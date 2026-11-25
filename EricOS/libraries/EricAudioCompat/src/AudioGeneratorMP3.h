#pragma once

#include <Arduino.h>
#include <AudioOutputBase.h>
#include <BackgroundAudioMP3.h>
#include "AudioFileSource.h"




class AudioGeneratorMP3 {
public:
  AudioGeneratorMP3();

  bool begin(AudioFileSource *source, AudioOutputBase *output);
  
  bool loop();
  bool isRunning() { return _running; }
  void stop();
  
  
  uint32_t framesDecoded() { return _mp3.frames(); }
  
  
  const char *lastError() const { return _lastError; }

private:
  bool feed();

  BackgroundAudioMP3 _mp3;
  AudioFileSource *_source = nullptr;
  AudioOutputBase *_output = nullptr;
  uint8_t _scratch[1024] = {0};
  bool _running = false;
  bool _finished = false;
  bool _pausedForDrain = false;
  uint32_t _lastFrameCount = 0;
  uint32_t _lastProgressAt = 0;
  const char *_lastError = "";
};
