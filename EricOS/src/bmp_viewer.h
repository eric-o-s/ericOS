#pragma once

#include <Arduino.h>




namespace EricBmp {
enum class Result : uint8_t { Ok, NoCard, OpenFailed, Unsupported, ReadFailed };

Result open(const char *path);
void close();
bool isOpen();
uint16_t width();
uint16_t height();

const char *formatLabel();


bool readScaledRow(uint16_t sourceY, uint16_t *destination, uint16_t destinationWidth);
const char *message(Result result);
}
