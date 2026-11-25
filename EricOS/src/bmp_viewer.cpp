#include <Arduino.h>
#include <SD.h>
#include "bmp_viewer.h"
#include "storage.h"

namespace {
File image;
uint32_t pixelOffset = 0;
uint32_t rowBytes = 0;
uint16_t imageWidth = 0;
uint16_t imageHeight = 0;
uint16_t palette[256] = {};
uint16_t paletteCount = 0;
uint8_t bitsPerPixel = 0;
uint32_t compression = 0;
uint32_t redMask = 0;
uint32_t greenMask = 0;
uint32_t blueMask = 0;
bool bottomUp = true;
const char *sourceFormat = "BMP";

uint16_t read16(File &file) {
  const int lo = file.read();
  const int hi = file.read();
  if (lo < 0 || hi < 0) return 0;
  return static_cast<uint16_t>(lo | (hi << 8));
}

uint32_t read32(File &file) {
  const int b0 = file.read();
  const int b1 = file.read();
  const int b2 = file.read();
  const int b3 = file.read();
  if (b0 < 0 || b1 < 0 || b2 < 0 || b3 < 0) return 0;
  return static_cast<uint32_t>(b0) | (static_cast<uint32_t>(b1) << 8) |
         (static_cast<uint32_t>(b2) << 16) | (static_cast<uint32_t>(b3) << 24);
}

uint8_t maskShift(uint32_t mask) {
  uint8_t shift = 0;
  while (mask && !(mask & 1U)) { ++shift; mask >>= 1U; }
  return shift;
}

uint8_t expandTo8(uint32_t pixel, uint32_t mask) {
  if (!mask) return 0;
  const uint8_t shift = maskShift(mask);
  const uint32_t maximum = mask >> shift;
  const uint32_t value = (pixel & mask) >> shift;
  return static_cast<uint8_t>((value * 255UL + maximum / 2UL) / maximum);
}

uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
  return static_cast<uint16_t>(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
}

void reset() {
  if (image) image.close();
  pixelOffset = 0;
  rowBytes = 0;
  imageWidth = 0;
  imageHeight = 0;
  paletteCount = 0;
  bitsPerPixel = 0;
  compression = 0;
  redMask = greenMask = blueMask = 0;
  bottomUp = true;
  sourceFormat = "BMP";
}

bool readPalette(uint32_t paletteOffset, uint32_t requestedEntries) {
  paletteCount = requestedEntries > 256U ? 256U : static_cast<uint16_t>(requestedEntries);
  if (!image.seek(paletteOffset)) return false;
  for (uint16_t i = 0; i < paletteCount; ++i) {
    const int blue = image.read();
    const int green = image.read();
    const int red = image.read();
    if (blue < 0 || green < 0 || red < 0 || image.read() < 0) return false;
    palette[i] = rgb565(static_cast<uint8_t>(red), static_cast<uint8_t>(green), static_cast<uint8_t>(blue));
  }
  return true;
}

bool readPixel(uint32_t rowStart, uint16_t sourceX, uint16_t &pixel) {
  if (bitsPerPixel == 24) {
    if (!image.seek(rowStart + static_cast<uint32_t>(sourceX) * 3U)) return false;
    const int blue = image.read(), green = image.read(), red = image.read();
    if (blue < 0 || green < 0 || red < 0) return false;
    pixel = rgb565(red, green, blue);
    return true;
  }
  if (bitsPerPixel == 32) {
    if (!image.seek(rowStart + static_cast<uint32_t>(sourceX) * 4U)) return false;
    const uint32_t raw = read32(image);
    pixel = rgb565(expandTo8(raw, redMask), expandTo8(raw, greenMask), expandTo8(raw, blueMask));
    return true;
  }
  if (bitsPerPixel == 16) {
    if (!image.seek(rowStart + static_cast<uint32_t>(sourceX) * 2U)) return false;
    const uint32_t raw = read16(image);
    pixel = rgb565(expandTo8(raw, redMask), expandTo8(raw, greenMask), expandTo8(raw, blueMask));
    return true;
  }
  if (bitsPerPixel == 8 || bitsPerPixel == 4 || bitsPerPixel == 1) {
    const uint32_t bitOffset = static_cast<uint32_t>(sourceX) * bitsPerPixel;
    if (!image.seek(rowStart + bitOffset / 8U)) return false;
    const int packed = image.read();
    if (packed < 0) return false;
    const uint8_t shift = static_cast<uint8_t>(8U - bitsPerPixel - (bitOffset & 7U));
    const uint8_t index = static_cast<uint8_t>((packed >> shift) & ((1U << bitsPerPixel) - 1U));
    if (index >= paletteCount) return false;
    pixel = palette[index];
    return true;
  }
  return false;
}
}

namespace EricBmp {
Result open(const char *path) {
  reset();
  if (!EricStorage::isReady()) return Result::NoCard;
  if (path == nullptr) return Result::OpenFailed;
  image = SD.open(path, FILE_READ);
  if (!image) return Result::OpenFailed;

  if (read16(image) != 0x4D42) { reset(); return Result::Unsupported; }
  (void)read32(image);
  (void)read32(image);
  pixelOffset = read32(image);
  const uint32_t headerSize = read32(image);
  const int32_t signedWidth = static_cast<int32_t>(read32(image));
  const int32_t signedHeight = static_cast<int32_t>(read32(image));
  const uint16_t planes = read16(image);
  bitsPerPixel = static_cast<uint8_t>(read16(image));
  compression = read32(image);
  (void)read32(image); 
  (void)read32(image); 
  (void)read32(image); 
  const uint32_t colorsUsed = read32(image);
  (void)read32(image); 

  if (headerSize < 40 || signedWidth <= 0 || signedHeight == 0 || planes != 1 || signedWidth > 65535) {
    reset();
    return Result::Unsupported;
  }
  const int32_t positiveHeight = signedHeight > 0 ? signedHeight : -signedHeight;
  if (positiveHeight > 65535 || (compression != 0 && compression != 3) ||
      (bitsPerPixel != 1 && bitsPerPixel != 4 && bitsPerPixel != 8 && bitsPerPixel != 16 && bitsPerPixel != 24 && bitsPerPixel != 32)) {
    reset();
    return Result::Unsupported;
  }
  
  
  if ((bitsPerPixel <= 8 && compression != 0) || (bitsPerPixel == 24 && compression != 0)) {
    reset();
    return Result::Unsupported;
  }

  imageWidth = static_cast<uint16_t>(signedWidth);
  imageHeight = static_cast<uint16_t>(positiveHeight);
  bottomUp = signedHeight > 0;
  rowBytes = ((static_cast<uint32_t>(imageWidth) * bitsPerPixel + 31U) / 32U) * 4U;

  const uint32_t afterHeader = 14U + headerSize;
  if (bitsPerPixel == 16 || bitsPerPixel == 32) {
    if (compression == 3) {
      
      if (!image.seek(afterHeader)) { reset(); return Result::Unsupported; }
      redMask = read32(image); greenMask = read32(image); blueMask = read32(image);
      if (!redMask || !greenMask || !blueMask) { reset(); return Result::Unsupported; }
    } else if (bitsPerPixel == 16) {
      redMask = 0x7C00U; greenMask = 0x03E0U; blueMask = 0x001FU;
    } else {
      redMask = 0x00FF0000U; greenMask = 0x0000FF00U; blueMask = 0x000000FFU;
    }
  }
  if (bitsPerPixel <= 8) {
    const uint32_t defaultEntries = 1U << bitsPerPixel;
    const uint32_t entries = colorsUsed ? colorsUsed : defaultEntries;
    if (!entries || entries > 256U || !readPalette(afterHeader, entries)) { reset(); return Result::Unsupported; }
  }

  if (bitsPerPixel == 1) sourceFormat = "1-BIT BMP";
  else if (bitsPerPixel == 4) sourceFormat = "4-BIT BMP";
  else if (bitsPerPixel == 8) sourceFormat = "8-BIT BMP";
  else if (bitsPerPixel == 16) sourceFormat = compression == 3 ? "16-BIT BMP" : "16-BIT BMP (555)";
  else if (bitsPerPixel == 24) sourceFormat = "24-BIT BMP";
  else sourceFormat = compression == 3 ? "32-BIT BMP" : "32-BIT BMP (BGRA)";
  return Result::Ok;
}

void close() { reset(); }
bool isOpen() { return static_cast<bool>(image); }
uint16_t width() { return imageWidth; }
uint16_t height() { return imageHeight; }
const char *formatLabel() { return sourceFormat; }

bool readScaledRow(uint16_t sourceY, uint16_t *destination, uint16_t destinationWidth) {
  if (!image || destination == nullptr || destinationWidth == 0 || sourceY >= imageHeight) return false;
  const uint16_t storedRow = bottomUp ? imageHeight - 1U - sourceY : sourceY;
  const uint32_t rowStart = pixelOffset + static_cast<uint32_t>(storedRow) * rowBytes;
  for (uint16_t x = 0; x < destinationWidth; ++x) {
    const uint16_t sourceX = static_cast<uint16_t>((static_cast<uint32_t>(x) * imageWidth) / destinationWidth);
    if (!readPixel(rowStart, sourceX, destination[x])) return false;
  }
  return true;
}

const char *message(Result result) {
  switch (result) {
    case Result::Ok: return "IMAGE OPEN";
    case Result::NoCard: return "INSERT FAT32 SD CARD";
    case Result::OpenFailed: return "COULD NOT OPEN IMAGE";
    case Result::Unsupported: return "BMP FORMAT NOT SUPPORTED";
    case Result::ReadFailed: return "IMAGE READ FAILED";
  }
  return "IMAGE ERROR";
}
}