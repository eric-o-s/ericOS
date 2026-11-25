#pragma once

#include <Arduino.h>




class AudioFileSource {
public:
  virtual ~AudioFileSource() {}
  virtual bool isOpen() = 0;
  virtual void close() = 0;
  virtual int available() = 0;
  virtual size_t readBytes(uint8_t *data, size_t length) = 0;
  
  virtual bool isEndOfStream() { return false; }
};
