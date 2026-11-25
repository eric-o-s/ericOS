#include "AudioOutputPWM.h"

void AudioOutputPWM::SetPinout(int8_t left, int8_t right) {
  if (_running) return;
  
  
  if (left >= 0 && right >= 0 && (int)right != (int)left + 1) {
    Serial.printf("AudioOutputPWM: stereo pins %d/%d are not adjacent; using %d only\n", left, right, left);
  }
  _pwm.setPin((pin_size_t)left);
  _configured = true;
}

void AudioOutputPWM::SetGain(float gain) { _gain = gain < 0.0f ? 0.0f : gain; }

bool AudioOutputPWM::setBuffers(size_t buffers, size_t bufferWords, int32_t silenceSample) {
  if (_running) return false;
  
  
  
  
  _bufferCount = buffers;
  _bufferWords = bufferWords;
  _silenceSample = silenceSample;
  return true;
}

bool AudioOutputPWM::setBitsPerSample(int bits) { return bits == 16; }

bool AudioOutputPWM::setFrequency(int frequency) {
  if (_running) return _pwm.setFrequency(frequency);
  _pendingFrequency = frequency;
  return true;
}

bool AudioOutputPWM::setStereo(bool stereo) {
  if (_running) return _pwm.setStereo(stereo);
  _pendingStereo = stereo;
  return true;
}

bool AudioOutputPWM::begin() {
  if (_running) return true;
  if (!_configured) SetPinout(0, 1);
  if (!_pwm.setStereo(_pendingStereo)) return false;
  
  
  
  if (_bufferCount < 3 || _bufferWords < 8) {
    _bufferCount = 8;
    _bufferWords = 64;
  }
  if (!_pwm.setBuffers(_bufferCount, _bufferWords, _silenceSample)) return false;
  _running = _pwm.begin(_pendingFrequency);
  return _running;
}

bool AudioOutputPWM::end() {
  if (!_running) return true;
  _pwm.end();
  _running = false;
  return true;
}

size_t AudioOutputPWM::write(const uint8_t *buffer, size_t size) {
  if (!buffer || size == 0) return 0;
  if (_gain > 0.999f && _gain < 1.001f) return _pwm.write(buffer, size);

  
  
  size_t done = 0;
  while (size - done >= 4) {
    int16_t left, right;
    memcpy(&left, buffer + done, 2);
    memcpy(&right, buffer + done + 2, 2);
    left = (int16_t)(left * _gain);
    right = (int16_t)(right * _gain);
    memcpy(_frame, &left, 2);
    memcpy(_frame + 2, &right, 2);
    if (_pwm.write((const uint8_t *)_frame, 4) != 4) break;
    done += 4;
  }
  return done;
}
