#pragma once

#include <Arduino.h>
#include <AudioOutputBase.h>
#include <PWMAudio.h>






class AudioOutputPWM : public AudioOutputBase {
public:
  AudioOutputPWM() {}

  void SetPinout(int8_t left, int8_t right);
  void SetGain(float gain);
  void SetFrequency(int frequency) { _pendingFrequency = frequency; }
  void SetStereo(bool stereo = true) { _pendingStereo = stereo; }
  
  bool isRunning() { return _running; }

  bool setBuffers(size_t buffers, size_t bufferWords, int32_t silenceSample = 0) override;
  bool setBitsPerSample(int bits) override;
  bool setFrequency(int frequency) override;
  bool setStereo(bool stereo = true) override;
  bool begin() override;
  bool end() override;
  bool getUnderflow() override { return _pwm.getUnderflow(); }
  void onTransmit(void (*fn)(void *), void *data) override { _pwm.onTransmit(fn, data); }
  
  
  size_t write(const uint8_t *buffer, size_t size) override;
  int availableForWrite() override { return _pwm.availableForWrite(); }
  
  size_t write(uint8_t value) override { return write(&value, 1); }

private:
  PWMAudio _pwm;
  int16_t _frame[2] = {0, 0};
  float _gain = 1.0f;
  int _pendingFrequency = 44100;
  bool _pendingStereo = true;
  bool _configured = false;
  bool _running = false;
  int32_t _silenceSample = 0;
  
  
  
  
  
  
  
  
  
  size_t _bufferCount = 8;
  size_t _bufferWords = 64;
};
