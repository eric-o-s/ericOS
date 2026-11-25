#include <Arduino.h>
#include <WiFi.h>
#include <string.h>
#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputPWM.h>
#include "radio_player.h"
#include "audio_engine.h"
#include "eric_config.h"

namespace {
constexpr size_t StreamBufferBytes = 12U * 1024U;


constexpr uint32_t CONNECT_WINDOW_MS = 15000UL;





const char *names[] = {"RAP", "R&B", "HOUSE", "ROCK"};
const char *genres[] = {"R&B / HIP-HOP", "R&B / SOUL", "HOUSE / TECHNO", "ROCK"};
const char *urls[] = {
  
  
  
  
  "http://bigrradio.cdnstream1.com/5105_128",
  "http://listen.181fm.com/181-rnb_128k.mp3",
  "http://stream.radioabf.com:8000/abf-sd.mp3",
  "http://stream.radioparadise.com/rock-192"
};
EricRadio::State currentState = EricRadio::State::Idle;
unsigned char currentStation = 0;
char currentMessage[40] = "SELECT A STATION";
AudioFileSourceICYStream *stream = nullptr;
AudioFileSourceBuffer *bufferedStream = nullptr;
AudioGeneratorMP3 &decoder = EricAudioEngine::decoder();
AudioOutputPWM &output = EricAudioEngine::output();
uint32_t startedAt = 0;
uint32_t framesAtStart = 0;

void setMessage(const char *value) {
  strncpy(currentMessage, value, sizeof(currentMessage) - 1);
  currentMessage[sizeof(currentMessage) - 1] = '\0';
}
}

namespace EricRadio {
bool begin() { stop(); return EricAudioEngine::prepareOutput(); }

bool play(unsigned char stationIndex) {
  stop();
  if (stationIndex >= StationCount || WiFi.status() != WL_CONNECTED) {
    setMessage("CONNECT W.I.F.I. BEFORE TUNING");
    return false;
  }
  if (!EricAudioEngine::prepareOutput()) {
    currentState = State::Failed;
    setMessage("AUDIO OUTPUT UNAVAILABLE");
    return false;
  }
  currentStation = stationIndex;
  stream = new AudioFileSourceICYStream(urls[stationIndex]);
  if (stream == nullptr || !stream->isOpen()) {
    
    
    
    char reason[40] = "STREAM CONNECTION FAILED";
    if (stream != nullptr) {
      const int status = stream->httpStatus();
      if (status > 0) snprintf(reason, sizeof(reason), "STREAM HTTP %d %s", status, stream->httpReason());
      else if (stream->httpReason()[0] != '\0') snprintf(reason, sizeof(reason), "STREAM %s", stream->httpReason());
    }
    if (stream) { delete stream; stream = nullptr; }
    currentState = State::Failed;
    setMessage(reason);
    return false;
  }
  bufferedStream = new AudioFileSourceBuffer(stream, StreamBufferBytes);
  EricAudioEngine::acquire(EricAudioEngine::Owner::Radio);
  if (bufferedStream == nullptr || !decoder.begin(bufferedStream, &output)) {
    stop();
    currentState = State::Failed;
    setMessage("STREAM DECODER FAILED");
    return false;
  }
  
  
  
  if (!EricAudioEngine::outputRunning()) {
    stop();
    currentState = State::Failed;
    setMessage("AUDIO OUTPUT DID NOT START");
    return false;
  }
  currentState = State::Connecting;
  startedAt = millis();
  framesAtStart = decoder.framesDecoded();
  setMessage("WAITING FOR STREAM RESPONSE...");
  return true;
}

bool start(unsigned char stationIndex) { return play(stationIndex); }

void pump() {
  if (currentState != State::Connecting && currentState != State::Playing) return;
  
  
  
  if (!EricAudioEngine::isOwner(EricAudioEngine::Owner::Radio)) {
    stop();
    currentState = State::Failed;
    setMessage("AUDIO CLAIMED ELSEWHERE");
    return;
  }
  if (!decoder.loop()) {
    stop();
    currentState = State::Failed;
    setMessage("STREAM ENDED OR TIMED OUT");
    return;
  }
  
  
  
  if (currentState == State::Connecting) {
    if (decoder.framesDecoded() > framesAtStart) {
      currentState = State::Playing;
      setMessage("ON AIR");
    } else if (millis() - startedAt > CONNECT_WINDOW_MS) {
      stop();
      currentState = State::Failed;
      setMessage("STREAM RESPONSE TIMED OUT");
    }
  }
}

void stop() {
  EricAudioEngine::release(EricAudioEngine::Owner::Radio);
  if (bufferedStream) { delete bufferedStream; bufferedStream = nullptr; }
  if (stream) { stream->close(); delete stream; stream = nullptr; }
  currentState = State::Idle;
  setMessage("SELECT A STATION");
}

State state() { return currentState; }
unsigned char station() { return currentStation; }
const char *stationName(unsigned char index) { return index < StationCount ? names[index] : ""; }
const char *stationGenre(unsigned char index) { return index < StationCount ? genres[index] : ""; }
const char *message() { return currentMessage; }
}