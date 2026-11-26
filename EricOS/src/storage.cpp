#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "storage.h"
#include "eric_config.h"

namespace {
bool cardReady = false;

// Card-detect debounce. The socket switch chatters while a card slides in, so the
// pin must hold one level for DetectSettleMs before the card is mounted or dropped.
constexpr uint32_t DetectPollMs = 100;
constexpr uint32_t DetectSettleMs = 500;
bool pinPresent = false;      // last raw reading of the detect pin
bool actedPresent = false;    // pin level the mount state was last updated for
uint32_t pinChangedAt = 0;
uint32_t lastPollAt = 0;

// A card that is present but did not mount (most often at power-on, when it is
// powered up at the same moment as the Pico and is not ready for the first try)
// is retried once a second for a while after boot or insertion.
constexpr uint32_t RemountPeriodMs = 1000;
constexpr uint8_t RemountAttempts = 10;
uint8_t remountsLeft = 0;
uint32_t lastMountAttemptAt = 0;

bool readDetectPin() {
  return digitalRead(EricConfig::SdDetect) == LOW;
}

bool mountCard() {
  // end(false) clears SDFS's mounted flag without stopping the shared SPI bus, so
  // begin() mounts the newly inserted card instead of returning the stale mount.
  SD.end(false);
  return SD.begin(EricConfig::SdCs, SPI);
}
}

namespace EricStorage {
bool begin() {
  pinMode(EricConfig::SdDetect, INPUT_PULLUP);
  SPI.setRX(EricConfig::SdMiso);
  SPI.setTX(EricConfig::SdMosi);
  SPI.setSCK(EricConfig::SdSck);
  SPI.begin();
  
  // Let the detect pin's pull-up settle before trusting it.
  delay(2);
  pinPresent = actedPresent = readDetectPin();
  cardReady = false;
  if (pinPresent) {
    // Settings, notes and Wi-Fi details are read from the card right after this,
    // so give a card that is still powering up a few seconds here rather than
    // mounting it later. Costs nothing when the first attempt works.
    // Bounded by time, not attempts: one failed init can itself take a while.
    constexpr uint32_t BootMountWindowMs = 3000;
    const uint32_t started = millis();
    cardReady = mountCard();
    while (!cardReady && millis() - started < BootMountWindowMs) {
      delay(250);
      cardReady = mountCard();
    }
  }
  remountsLeft = (pinPresent && !cardReady) ? RemountAttempts : 0;
  pinChangedAt = lastPollAt = lastMountAttemptAt = millis();
  return cardReady;
}

bool serviceCardDetect() {
  const uint32_t now = millis();
  if (now - lastPollAt < DetectPollMs) return false;
  lastPollAt = now;

  const bool present = readDetectPin();
  if (present != pinPresent) {
    pinPresent = present;
    pinChangedAt = now;
    return false;
  }
  if (present == actedPresent) {
    if (!present || cardReady || !remountsLeft || now - lastMountAttemptAt < RemountPeriodMs) return false;
    --remountsLeft;
    lastMountAttemptAt = now;
    cardReady = mountCard();
    return cardReady;
  }
  if (now - pinChangedAt < DetectSettleMs) return false;

  // Act once per settled change; a card that fails to mount gets the limited
  // retries above, and re-inserting it starts them again.
  actedPresent = present;
  const bool wasReady = cardReady;
  if (present) {
    cardReady = mountCard();
    remountsLeft = cardReady ? 0 : RemountAttempts;
    lastMountAttemptAt = now;
  } else {
    remountsLeft = 0;
    cardReady = false;
    SD.end(false);
  }
  return cardReady != wasReady;
}

bool isReady() {
  return cardReady;
}

bool saveText(const char *path, const char *data, size_t length) {
  if (!cardReady || path == nullptr || data == nullptr) return false;
  if (!SD.exists("/ERIC") && !SD.mkdir("/ERIC")) return false;
  File file = SD.open(path, FILE_WRITE);
  if (!file) return false;
  file.seek(0);
  file.truncate(0);
  const size_t written = file.write(reinterpret_cast<const uint8_t *>(data), length);
  file.close();
  return written == length;
}

bool loadText(const char *path, char *out, size_t capacity, size_t &length) {
  length = 0;
  if (!cardReady || path == nullptr || out == nullptr || capacity == 0) return false;
  File file = SD.open(path, FILE_READ);
  if (!file) return false;
  while (file.available() && length + 1 < capacity) {
    const int value = file.read();
    if (value >= 0) out[length++] = static_cast<char>(value);
  }
  file.close();
  out[length] = '\0';
  return true;
}
}