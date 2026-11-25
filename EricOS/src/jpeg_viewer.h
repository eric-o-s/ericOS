#pragma once

#include <Arduino.h>



namespace EricJpeg {
enum class Result : uint8_t { Ok, NoCard, OpenFailed, Unsupported, DecodeFailed };
using OutputCallback = bool (*)(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *pixels);

Result open(const char *path);
void close();
bool isOpen();
uint16_t width();
uint16_t height();
const char *formatLabel();
bool draw(int16_t originX, int16_t originY, uint8_t scale, OutputCallback output);
const char *message(Result result);
}
