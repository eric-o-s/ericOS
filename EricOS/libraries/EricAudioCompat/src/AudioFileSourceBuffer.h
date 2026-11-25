#pragma once

#include <Arduino.h>
#include "AudioFileSource.h"


class AudioFileSourceBuffer : public AudioFileSource {
public:
  AudioFileSourceBuffer(AudioFileSource *source, uint32_t bufferBytes);
  ~AudioFileSourceBuffer() override;

  void close() override;
  bool isOpen() override;
  int available() override;
  size_t readBytes(uint8_t *data, size_t length) override;
  bool isEndOfStream() override;

  
  
  void pump();

private:
  size_t freeSpace() const { return _capacity - _count; }

  AudioFileSource *_source = nullptr;
  uint8_t *_buffer = nullptr;
  size_t _capacity = 0;
  size_t _head = 0;   
  size_t _count = 0;  
};
