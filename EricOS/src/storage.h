#pragma once

#include <Arduino.h>



namespace EricStorage {
bool begin();
bool isReady();

bool saveText(const char *path, const char *data, size_t length);
bool loadText(const char *path, char *out, size_t capacity, size_t &length);
}
