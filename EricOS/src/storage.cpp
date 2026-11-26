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
  
  pinPresent = actedPresent = readDetectPin();
  pinChangedAt = lastPollAt = millis();
  cardReady = pinPresent && SD.begin(EricConfig::SdCs, SPI);
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
  if (present == actedPresent || now - pinChangedAt < DetectSettleMs) return false;

  // Act once per settled change, so a card that fails to mount is not retried on
  // every poll; re-inserting it tries again.
  actedPresent = present;
  const bool wasReady = cardReady;
  if (present) {
    cardReady = mountCard();
  } else {
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