#pragma once

#include <Arduino.h>
class Arduino_GFX;




namespace EricGameBoy {
enum class StartResult : uint8_t { Ok, NoCard, OpenFailed, InvalidRom, UnsupportedCart, SaveTooLarge };
void begin(Arduino_GFX *screen);
StartResult start(const char *romPath);
void serviceCore1();
void presentFrame();



struct PerformanceStats {
  uint32_t emulatedFrames;
  uint32_t lateFrames;
  uint32_t worstFrameUs;
  uint32_t romCacheMisses;
  uint32_t worstRomReadUs;
  uint32_t lcdBands;
  uint32_t worstLcdBandUs;
};
PerformanceStats performanceStats();


void yieldDisplayToSystem();



void setButtons(uint8_t pressedMask, uint32_t holdMs);
void setPaused(bool paused);
void stop();
bool isRunning();
const char *title();
const char *romPath();
bool saveState();
bool loadState();


}
