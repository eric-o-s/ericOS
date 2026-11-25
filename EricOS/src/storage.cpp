#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "storage.h"
#include "eric_config.h"

namespace {
bool cardReady = false;
}

namespace EricStorage {
bool begin() {
  pinMode(EricConfig::SdDetect, INPUT_PULLUP);
  SPI.setRX(EricConfig::SdMiso);
  SPI.setTX(EricConfig::SdMosi);
  SPI.setSCK(EricConfig::SdSck);
  SPI.begin();
  
  cardReady = (digitalRead(EricConfig::SdDetect) == LOW) && SD.begin(EricConfig::SdCs, SPI);
  return cardReady;
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