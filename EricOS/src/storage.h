#pragma once

#include <Arduino.h>



namespace EricStorage {
bool begin();
bool isReady();

// Watch the card-detect switch and mount or drop the card when it settles in a new
// position. Call often from loop(); returns true when isReady() changed.
bool serviceCardDetect();

bool saveText(const char *path, const char *data, size_t length);
bool loadText(const char *path, char *out, size_t capacity, size_t &length);
}
