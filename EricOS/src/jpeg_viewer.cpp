#include <Arduino.h>
#include <SD.h>
#include <TJpg_Decoder.h>
#include "jpeg_viewer.h"
#include "storage.h"

namespace {
char imagePath[128] = {};
uint16_t imageWidth = 0;
uint16_t imageHeight = 0;
bool opened = false;
EricJpeg::OutputCallback drawCallback = nullptr;

bool jpegOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *pixels) {
  return drawCallback != nullptr && drawCallback(x, y, w, h, pixels);
}

void reset() {
  imagePath[0] = '\0';
  imageWidth = 0;
  imageHeight = 0;
  opened = false;
  drawCallback = nullptr;
}
}

namespace EricJpeg {
Result open(const char *path) {
  reset();
  if (!EricStorage::isReady()) return Result::NoCard;
  if (path == nullptr || path[0] == '\0' || strlen(path) >= sizeof(imagePath)) return Result::OpenFailed;
  File file = SD.open(path, FILE_READ);
  if (!file) return Result::OpenFailed;
  uint16_t width = 0, height = 0;
  const JRESULT result = TJpgDec.getSdJpgSize(&width, &height, file);
  file.close();
  if (result != JDR_OK || width == 0 || height == 0) return Result::Unsupported;
  strncpy(imagePath, path, sizeof(imagePath) - 1);
  imagePath[sizeof(imagePath) - 1] = '\0';
  imageWidth = width;
  imageHeight = height;
  opened = true;
  return Result::Ok;
}

void close() { reset(); }
bool isOpen() { return opened; }
uint16_t width() { return imageWidth; }
uint16_t height() { return imageHeight; }
const char *formatLabel() { return "JPEG"; }

bool draw(int16_t originX, int16_t originY, uint8_t scale, OutputCallback output) {
  if (!opened || output == nullptr || (scale != 1 && scale != 2 && scale != 4 && scale != 8)) return false;
  drawCallback = output;
  TJpgDec.setJpgScale(scale);
  
  TJpgDec.setSwapBytes(false);
  TJpgDec.setCallback(jpegOutput);
  File file = SD.open(imagePath, FILE_READ);
  if (!file) { drawCallback = nullptr; return false; }
  const JRESULT result = TJpgDec.drawSdJpg(originX, originY, file);
  file.close();
  drawCallback = nullptr;
  return result == JDR_OK;
}

const char *message(Result result) {
  switch (result) {
    case Result::Ok: return "IMAGE OPEN";
    case Result::NoCard: return "INSERT FAT32 SD CARD";
    case Result::OpenFailed: return "COULD NOT OPEN IMAGE";
    case Result::Unsupported: return "JPEG FORMAT NOT SUPPORTED";
    case Result::DecodeFailed: return "JPEG DECODE FAILED";
  }
  return "IMAGE ERROR";
}
}
