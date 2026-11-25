#include <Arduino.h>
#include <string.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputPWM.h>
#include "audio_player.h"
#include "audio_engine.h"
#include "eric_config.h"

namespace {
AudioFileSourceSD source;
AudioGeneratorMP3 &decoder = EricAudioEngine::decoder();
AudioOutputPWM &output = EricAudioEngine::output();
char currentPath[256] = "";
char currentMessage[36] = "IDLE";
bool playing = false;

void setMessage(const char *value) {
  strncpy(currentMessage, value, sizeof(currentMessage) - 1);
  currentMessage[sizeof(currentMessage) - 1] = '\0';
}
}

namespace EricAudio {
bool begin() { stop(); return EricAudioEngine::prepareOutput(); }

bool play(const char *requestedPath) {
  stop();
  if (requestedPath == nullptr || requestedPath[0] == '\0') {
    setMessage("NO AUDIO FILE SELECTED");
    return false;
  }
  if (!EricAudioEngine::prepareOutput()) {
    setMessage("AUDIO OUTPUT UNAVAILABLE");
    return false;
  }
  if (!source.open(requestedPath)) {
    setMessage("COULD NOT OPEN AUDIO FILE");
    return false;
  }
  strncpy(currentPath, requestedPath, sizeof(currentPath) - 1);
  currentPath[sizeof(currentPath) - 1] = '\0';
  EricAudioEngine::acquire(EricAudioEngine::Owner::LocalFile);
  if (!decoder.begin(&source, &output)) {
    source.close();
    currentPath[0] = '\0';
    setMessage(decoder.lastError()[0] != '\0' ? decoder.lastError() : "UNSUPPORTED MP3 FILE");
    return false;
  }
  
  
  
  if (!EricAudioEngine::outputRunning()) {
    EricAudio::stop();
    setMessage("AUDIO OUTPUT DID NOT START");
    return false;
  }
  playing = true;
  setMessage("PLAYING");
  return true;
}

void pump() {
  if (playing && !decoder.loop()) stop();
}

void stop() {
  EricAudioEngine::release(EricAudioEngine::Owner::LocalFile);
  if (source.isOpen()) source.close();
  playing = false;
  currentPath[0] = '\0';
  setMessage("STOPPED");
}

bool isPlaying() {
  return playing && EricAudioEngine::isOwner(EricAudioEngine::Owner::LocalFile) && decoder.isRunning();
}
const char *path() { return currentPath; }
const char *message() { return currentMessage; }
}