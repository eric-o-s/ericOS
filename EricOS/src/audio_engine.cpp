#include "audio_engine.h"
#include "eric_config.h"

namespace EricAudioEngine {
namespace {
AudioGeneratorMP3 sharedDecoder;
AudioOutputPWM sharedOutput;
AudioOutputPWM sharedSynthOutput;
Owner holder = Owner::None;
bool outputConfigured = false;
bool synthOutputConfigured = false;
}

AudioGeneratorMP3 &decoder() { return sharedDecoder; }
AudioOutputPWM &output() { return sharedOutput; }
AudioOutputPWM &synthOutput() { return sharedSynthOutput; }

bool prepareSynthOutput() {
  if (!synthOutputConfigured) {
    sharedSynthOutput.SetPinout(EricConfig::AudioLeft, EricConfig::AudioRight);
    sharedSynthOutput.SetGain(0.80f);
    
    sharedSynthOutput.setBuffers(8, 64);
    synthOutputConfigured = true;
  }
  return true;
}

bool prepareOutput() {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (!outputConfigured) {
    sharedOutput.SetPinout(EricConfig::AudioLeft, EricConfig::AudioRight);
    sharedOutput.SetGain(0.80f);
    outputConfigured = true;
  }
  return true;
}

bool outputRunning() { return sharedOutput.isRunning(); }
bool synthOutputRunning() { return sharedSynthOutput.isRunning(); }

void acquire(Owner who) {
  if (sharedDecoder.isRunning()) sharedDecoder.stop();
  holder = who;
}

void release(Owner who) {
  if (holder != who) return;
  if (sharedDecoder.isRunning()) sharedDecoder.stop();
  holder = Owner::None;
}

bool isOwner(Owner who) { return holder == who; }
}
