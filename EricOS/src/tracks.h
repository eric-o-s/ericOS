#pragma once

#include <Arduino.h>

namespace EricTracks {
  void begin();
  void pump();
  void handleKey(uint8_t key);
  bool isPlaying();
  void stop();
} 

void drawTracksWorkspace();
void handleTracksKey(uint8_t key);
