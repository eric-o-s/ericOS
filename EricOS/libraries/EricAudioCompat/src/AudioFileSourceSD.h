#pragma once

#include <Arduino.h>
#include <FS.h>
#include "AudioFileSource.h"

class AudioFileSourceSD : public AudioFileSource {
public:
  bool open(const char *filename);
  void close() override;
  bool isOpen() override;
  int available() override;
  size_t readBytes(uint8_t *data, size_t length) override;
  bool isEndOfStream() override { return _eof; }

private:
  File _file;
  bool _open = false;
  bool _eof = false;
};
