#include "AudioGeneratorMP3.h"

namespace {

constexpr size_t PauseThreshold = 512;


constexpr uint32_t DrainTimeoutMs = 400;
}

AudioGeneratorMP3::AudioGeneratorMP3() {}

bool AudioGeneratorMP3::begin(AudioFileSource *source, AudioOutputBase *output) {
  stop();
  _lastError = "";
  if (source == nullptr || output == nullptr) {
    _lastError = "NO SOURCE OR OUTPUT";
    return false;
  }
  _source = source;
  _output = output;
  
  
  
  if (!_mp3.setDevice(_output)) {
    _source = nullptr;
    _output = nullptr;
    _lastError = "DECODER BUSY ELSEWHERE";
    return false;
  }
  if (!_mp3.begin()) {
    _source = nullptr;
    _output = nullptr;
    _lastError = "MP3 CORE REFUSED START";
    return false;
  }
  _running = true;
  _finished = false;
  _pausedForDrain = false;
  _lastFrameCount = 0;
  _lastProgressAt = millis();
  return true;
}

bool AudioGeneratorMP3::feed() {
  if (_source == nullptr) return false;
  size_t room = _mp3.availableForWrite();
  if (room == 0) return false;
  const size_t wanted = room < 1024 ? room : 1024;
  const size_t got = _source->readBytes(_scratch, wanted);
  if (got == 0) return false;
  return _mp3.write(_scratch, got) == got;
}

bool AudioGeneratorMP3::loop() {
  if (!_running) return false;
  if (_finished) {
    stop();
    return false;
  }

  const bool sourceDrained = (_source == nullptr) || !_source->isOpen() || _source->isEndOfStream();
  feed();

  if (sourceDrained) {
    
    
    if (_mp3.available() < PauseThreshold) {
      if (!_pausedForDrain) {
        _mp3.pause();
        _pausedForDrain = true;
      }
    } else if (_pausedForDrain) {
      _mp3.unpause();
      _pausedForDrain = false;
    }

    const uint32_t frames = _mp3.frames();
    if (frames != _lastFrameCount) {
      _lastFrameCount = frames;
      _lastProgressAt = millis();
    } else if (millis() - _lastProgressAt > DrainTimeoutMs) {
      _finished = true;
    }
  }
  return !_finished;
}

void AudioGeneratorMP3::stop() {
  if (_running) {
    if (_pausedForDrain) {
      _mp3.unpause();
      _pausedForDrain = false;
    }
    _mp3.end();
  }
  _running = false;
  _source = nullptr;
  _output = nullptr;
}
