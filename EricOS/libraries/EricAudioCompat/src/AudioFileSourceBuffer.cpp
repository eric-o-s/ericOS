#include "AudioFileSourceBuffer.h"

namespace {


constexpr size_t ChunkBytes = 1024;
}

AudioFileSourceBuffer::AudioFileSourceBuffer(AudioFileSource *source, uint32_t bufferBytes)
  : _source(source) {
  if (_source == nullptr || bufferBytes < 2) return;
  _capacity = bufferBytes;
  _buffer = new (std::nothrow) uint8_t[_capacity];
}

AudioFileSourceBuffer::~AudioFileSourceBuffer() { delete[] _buffer; }

void AudioFileSourceBuffer::close() {
  _head = 0;
  _count = 0;
}

bool AudioFileSourceBuffer::isOpen() { return _buffer != nullptr && _source != nullptr && _source->isOpen(); }

bool AudioFileSourceBuffer::isEndOfStream() {
  return _count == 0 && (_source == nullptr || !_source->isOpen() || _source->isEndOfStream());
}

int AudioFileSourceBuffer::available() {
  pump();
  return (int)_count;
}

void AudioFileSourceBuffer::pump() {
  if (_buffer == nullptr || _source == nullptr || !_source->isOpen()) return;
  while (freeSpace() > 0) {
    const size_t room = freeSpace() < ChunkBytes ? freeSpace() : ChunkBytes;
    const size_t produced = _source->readBytes(_buffer + ((_head + _count) % _capacity), room);
    if (produced == 0) return;
    _count += produced;
  }
}

size_t AudioFileSourceBuffer::readBytes(uint8_t *data, size_t length) {
  if (data == nullptr || length == 0 || _count == 0) {
    pump();
    if (_count == 0) return 0;
  }
  const size_t take = _count < length ? _count : length;
  for (size_t i = 0; i < take; i++) data[i] = _buffer[(_head + i) % _capacity];
  _head = (_head + take) % _capacity;
  _count -= take;
  return take;
}
