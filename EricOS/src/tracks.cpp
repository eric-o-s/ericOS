#include "tracks.h"
#include "audio_engine.h"
#include "eric_config.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#ifndef KEY_LEFT
#define KEY_LEFT 0xB4
#endif
#ifndef KEY_RIGHT
#define KEY_RIGHT 0xB7
#endif
#ifndef KEY_UP
#define KEY_UP 0xB5
#endif
#ifndef KEY_DOWN
#define KEY_DOWN 0xB6
#endif
#ifndef KEY_ENTER
#define KEY_ENTER 0x0A
#endif
#ifndef KEY_BACK
#define KEY_BACK 0x08
#endif
#ifndef KEY_SPACE
#define KEY_SPACE 0x20
#endif

extern Arduino_ILI9488_18bit display;

namespace {
  static const char *NOTE_NAMES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  static const float NOTE_FREQS[6][12] = {
    {32.70, 34.65, 36.71, 38.89, 41.20, 43.65, 46.25, 49.00, 51.91, 55.00, 58.27, 61.74},
    {65.41, 69.30, 73.42, 77.78, 82.41, 87.31, 92.50, 98.00, 103.83, 110.00, 116.54, 123.47},
    {130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94},
    {261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88},
    {523.25, 554.37, 587.33, 622.25, 659.25, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77},
    {1046.5, 1108.73, 1174.66, 1244.51, 1318.51, 1396.91, 1479.98, 1567.98, 1661.22, 1760.00, 1864.66, 1975.53},
  };
  
  bool playing = false;
  uint32_t bpm = 120;
  uint32_t step = 0;
  uint32_t lastStepMs = 0;
  int currentTrack = 0;
  int cursorX = 0;
  const int numTracks = 6;
  const int numSteps = 16;

  
  
  
  
  constexpr int kScreenW = 320;
  constexpr int kScreenH = 320;
  constexpr int kCellW = 16;
  constexpr int kCellH = 9;
  constexpr int kGridW = numSteps * kCellW;
  constexpr int kGridH = numTracks * kCellH;
  constexpr int kGridX = (kScreenW - kGridW) / 2;
  constexpr int kGridY = (kScreenH - kGridH) / 2;
  
  struct Track {
    bool pattern[numSteps];
    int noteIdx;
    float freq;
    char noteName[4];
  };
  
  Track tracks[6] = {
    {{true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false}, 24, 261.63, "C4"},
    {{false,false,false,false,false,false,false,false,false,false,false,false,false,false,false}, 28, 329.63, "E4"},
    {{false,false,false,false,false,false,false,false,false,false,false,false,false,false,false}, 31, 392.00, "G4"},
    {{false,false,false,false,false,false,false,false,false,false,false,false,false,false,false}, 36, 523.25, "C5"},
    {{false,false,false,false,false,false,false,false,false,false,false,false,false,false,false}, 12, 130.81, "C3"},
    {{false,false,false,false,false,false,false,false,false,false,false,false,false,false,false}, 0, 32.70, "C1"},
  };
  
  int16_t phase = 0;
  int16_t sampleRate = 22050;
  volatile float currentFreq = 0.0;
  uint32_t stepDurMs() { return (60000UL / bpm) / 4UL; }
  int lastDrawnStep = -1;
  
  void updateTrackFreq(int t) {
    if (tracks[t].noteIdx < 0) tracks[t].noteIdx = 0;
    if (tracks[t].noteIdx > 71) tracks[t].noteIdx = 71;
    int oct = tracks[t].noteIdx / 12;
    int n = tracks[t].noteIdx % 12;
    if (oct > 5) oct = 5;
    if (oct < 0) oct = 0;
    tracks[t].freq = NOTE_FREQS[oct][n];
    snprintf(tracks[t].noteName, sizeof(tracks[t].noteName), "%s%d", NOTE_NAMES[n], oct + 1);
  }
}

namespace EricTracks {
  void begin() {
    for (int i = 0; i < 6; i++) updateTrackFreq(i);
    EricAudioEngine::prepareSynthOutput();
  }
  
  void stop() {
    if (playing) {
      if (EricAudioEngine::isOwner(EricAudioEngine::Owner::Synth)) {
        EricAudioEngine::release(EricAudioEngine::Owner::Synth);
      }
      playing = false;
    }
    if (EricAudioEngine::synthOutput().isRunning()) {
      EricAudioEngine::synthOutput().end();
    }
  }
  
  void pump() {
    if (!playing) return;
    if (!EricAudioEngine::synthOutput().isRunning()) {
      EricAudioEngine::synthOutput().setFrequency(sampleRate);
      EricAudioEngine::synthOutput().begin();
    }
    if (!EricAudioEngine::isOwner(EricAudioEngine::Owner::Synth)) {
      EricAudioEngine::acquire(EricAudioEngine::Owner::Synth);
    }
    uint32_t now = millis();
    if (now - lastStepMs >= stepDurMs()) {
      lastStepMs = now;
      step = (step + 1) % numSteps;
      currentFreq = 0.0;
      for (int t = 0; t < numTracks; t++) {
        if (tracks[t].pattern[step]) {
          currentFreq = tracks[t].freq;
          break;
        }
      }
      phase = 0;
      if (step != lastDrawnStep) {
        const int gridW = kGridW;
        const int gridH = kGridH;
        const int gridX = kGridX;
        const int gridY = kGridY;
        const int cellW = kCellW;
        const int cellH = kCellH;
        if (lastDrawnStep >= 0) {
          for (int t = 0; t < numTracks; t++) {
            int x = gridX + lastDrawnStep * cellW;
            int y = gridY + t * cellH;
            if (t == currentTrack && lastDrawnStep == cursorX) {
              display.drawRect(x, y, cellW, cellH, 0xFFFF);
              uint16_t color = (t == 0) ? 0xF800 : (t == 1) ? 0x07E0 : (t == 2) ? 0x001F : (t == 3) ? 0xFFE0 : (t == 4) ? 0xFD20 : 0xF81F;
              if (tracks[t].pattern[lastDrawnStep]) display.fillRect(x+1, y+1, cellW-2, cellH-2, color);
              else display.fillRect(x+1, y+1, cellW-2, cellH-2, 0x4208);
            } else if (tracks[t].pattern[lastDrawnStep]) {
              uint16_t color = (t == 0) ? 0xF800 : (t == 1) ? 0x07E0 : (t == 2) ? 0x001F : (t == 3) ? 0xFFE0 : (t == 4) ? 0xFD20 : 0xF81F;
              display.fillRect(x, y, cellW, cellH, color);
            } else {
              display.drawRect(x, y, cellW, cellH, 0x3186);
            }
          }
        }
        for (int t = 0; t < numTracks; t++) {
          int x = gridX + step * cellW;
          int y = gridY + t * cellH;
          display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
        }
        lastDrawnStep = step;
      }
    }
    AudioOutputPWM &out = EricAudioEngine::synthOutput();
    if (!out.isRunning() || currentFreq <= 0.0) return;
    int free = out.availableForWrite();
    if (free < 256) return;
    static int16_t buf[256];
    float period = (float)sampleRate / currentFreq;
    if (period < 1.0) period = 1.0;
    for (int i = 0; i < 128; i++) {
      phase++;
      if (phase >= (int)period) phase = 0;
      int16_t val = (phase < period / 2.0) ? 8000 : -8000;
      buf[i*2] = val;
      buf[i*2+1] = val;
    }
    out.write((uint8_t*)buf, 512);
  }
  
  void handleKey(uint8_t key) {
    if (key == ' ') {
      if (!playing) {
        EricAudioEngine::prepareSynthOutput();
        if (!EricAudioEngine::synthOutput().isRunning()) {
          EricAudioEngine::synthOutput().setFrequency(sampleRate);
          EricAudioEngine::synthOutput().begin();
        }
        EricAudioEngine::acquire(EricAudioEngine::Owner::Synth);
        playing = true;
        step = 0;
        lastStepMs = millis();
        currentFreq = 0.0;
        lastDrawnStep = -1;
        
        display.fillRect(120, 18, 50, 10, 0x0000);
        display.setTextColor(0xFFFF);
        display.setTextSize(1);
        display.setCursor(120, 18);
        display.print("PLAY");
        
        const int gridX = kGridX;
        const int gridY = kGridY;
        const int cellW = kCellW;
        const int cellH = kCellH;
        for (int t = 0; t < numTracks; t++) {
          int x = gridX + step * cellW;
          int y = gridY + t * cellH;
          display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
        }
        lastDrawnStep = step;
      } else {
        stop();
        
        if (lastDrawnStep >= 0) {
          const int gridX = kGridX;
          const int gridY = kGridY;
          const int cellW = kCellW;
          const int cellH = kCellH;
          for (int t = 0; t < numTracks; t++) {
            int x = gridX + lastDrawnStep * cellW;
            int y = gridY + t * cellH;
            if (t == currentTrack && lastDrawnStep == cursorX) {
              display.drawRect(x, y, cellW, cellH, 0xFFFF);
              uint16_t color = (t == 0) ? 0xF800 : (t == 1) ? 0x07E0 : (t == 2) ? 0x001F : (t == 3) ? 0xFFE0 : (t == 4) ? 0xFD20 : 0xF81F;
              if (tracks[t].pattern[lastDrawnStep]) display.fillRect(x+1, y+1, cellW-2, cellH-2, color);
              else display.fillRect(x+1, y+1, cellW-2, cellH-2, 0x4208);
            } else if (tracks[t].pattern[lastDrawnStep]) {
              uint16_t color = (t == 0) ? 0xF800 : (t == 1) ? 0x07E0 : (t == 2) ? 0x001F : (t == 3) ? 0xFFE0 : (t == 4) ? 0xFD20 : 0xF81F;
              display.fillRect(x, y, cellW, cellH, color);
            } else {
              display.drawRect(x, y, cellW, cellH, 0x3186);
            }
          }
        }
        display.fillRect(120, 18, 50, 10, 0x0000);
        display.setTextColor(0xFFFF);
        display.setTextSize(1);
        display.setCursor(120, 18);
        display.print("STOP");
        lastDrawnStep = -1;
      }
      return;
    }
    if (key == KEY_UP) {
      int oldTrack = currentTrack;
      currentTrack = (currentTrack == 0) ? numTracks - 1 : currentTrack - 1;
      
      {
        const int gridX = kGridX;
        const int gridY = kGridY;
        const int cellW = kCellW;
        const int cellH = kCellH;
        for (int s = 0; s < numSteps; s++) {
          int x = gridX + s * cellW;
          int y = gridY + oldTrack * cellH;
          if (tracks[oldTrack].pattern[s]) {
            uint16_t color = (oldTrack == 0) ? 0xF800 : (oldTrack == 1) ? 0x07E0 : (oldTrack == 2) ? 0x001F : (oldTrack == 3) ? 0xFFE0 : (oldTrack == 4) ? 0xFD20 : 0xF81F;
            display.fillRect(x, y, cellW, cellH, color);
          } else {
            display.drawRect(x, y, cellW, cellH, 0x3186);
          }
          if (playing && s == step) {
            display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
          }
        }
      }
      
      {
        const int gridX = kGridX;
        const int gridY = kGridY;
        const int cellW = kCellW;
        const int cellH = kCellH;
        for (int s = 0; s < numSteps; s++) {
          int x = gridX + s * cellW;
          int y = gridY + currentTrack * cellH;
          if (tracks[currentTrack].pattern[s]) {
            uint16_t color = (currentTrack == 0) ? 0xF800 : (currentTrack == 1) ? 0x07E0 : (currentTrack == 2) ? 0x001F : (currentTrack == 3) ? 0xFFE0 : (currentTrack == 4) ? 0xFD20 : 0xF81F;
            display.fillRect(x, y, cellW, cellH, color);
          } else {
            display.drawRect(x, y, cellW, cellH, 0x3186);
          }
          if (s == cursorX) {
            display.drawRect(x, y, cellW, cellH, 0xFFFF);
          }
          if (playing && s == step) {
            display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
          }
        }
      }
      
      display.fillRect(8 + 6*6, 18, 30, 10, 0x0000);
      display.setTextColor(0xFFFF);
      display.setTextSize(1);
      display.setCursor(8 + 6*6, 18);
      display.print(currentTrack + 1);
      display.fillRect(8 + 6*6 + 30 + 6*5, 18, 40, 10, 0x0000);
      display.setCursor(8 + 6*6 + 30 + 6*5, 18);
      display.print(tracks[currentTrack].noteName);
      return;
    }
    if (key == KEY_DOWN) {
      int oldTrack = currentTrack;
      currentTrack = (currentTrack + 1) % numTracks;
      {
        const int gridX = kGridX;
        const int gridY = kGridY;
        const int cellW = kCellW;
        const int cellH = kCellH;
        for (int s = 0; s < numSteps; s++) {
          int x = gridX + s * cellW;
          int y = gridY + oldTrack * cellH;
          if (tracks[oldTrack].pattern[s]) {
            uint16_t color = (oldTrack == 0) ? 0xF800 : (oldTrack == 1) ? 0x07E0 : (oldTrack == 2) ? 0x001F : (oldTrack == 3) ? 0xFFE0 : (oldTrack == 4) ? 0xFD20 : 0xF81F;
            display.fillRect(x, y, cellW, cellH, color);
          } else {
            display.drawRect(x, y, cellW, cellH, 0x3186);
          }
          if (playing && s == step) {
            display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
          }
        }
      }
      {
        const int gridX = kGridX;
        const int gridY = kGridY;
        const int cellW = kCellW;
        const int cellH = kCellH;
        for (int s = 0; s < numSteps; s++) {
          int x = gridX + s * cellW;
          int y = gridY + currentTrack * cellH;
          if (tracks[currentTrack].pattern[s]) {
            uint16_t color = (currentTrack == 0) ? 0xF800 : (currentTrack == 1) ? 0x07E0 : (currentTrack == 2) ? 0x001F : (currentTrack == 3) ? 0xFFE0 : (currentTrack == 4) ? 0xFD20 : 0xF81F;
            display.fillRect(x, y, cellW, cellH, color);
          } else {
            display.drawRect(x, y, cellW, cellH, 0x3186);
          }
          if (s == cursorX) {
            display.drawRect(x, y, cellW, cellH, 0xFFFF);
          }
          if (playing && s == step) {
            display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
          }
        }
      }
      display.fillRect(8 + 6*6, 18, 30, 10, 0x0000);
      display.setTextColor(0xFFFF);
      display.setTextSize(1);
      display.setCursor(8 + 6*6, 18);
      display.print(currentTrack + 1);
      display.fillRect(8 + 6*6 + 30 + 6*5, 18, 40, 10, 0x0000);
      display.setCursor(8 + 6*6 + 30 + 6*5, 18);
      display.print(tracks[currentTrack].noteName);
      return;
    }
    if (key == KEY_LEFT) {
      int oldX = cursorX;
      cursorX = (cursorX == 0) ? numSteps - 1 : cursorX - 1;
      const int gridX = kGridX;
      const int gridY = kGridY;
      const int cellW = kCellW;
      const int cellH = kCellH;
      
      {
        int x = gridX + oldX * cellW;
        int y = gridY + currentTrack * cellH;
        if (tracks[currentTrack].pattern[oldX]) {
          uint16_t color = (currentTrack == 0) ? 0xF800 : (currentTrack == 1) ? 0x07E0 : (currentTrack == 2) ? 0x001F : (currentTrack == 3) ? 0xFFE0 : (currentTrack == 4) ? 0xFD20 : 0xF81F;
          display.fillRect(x, y, cellW, cellH, color);
        } else {
          display.drawRect(x, y, cellW, cellH, 0x3186);
        }
        if (playing && oldX == step) {
          display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
        }
      }
      
      {
        int x = gridX + cursorX * cellW;
        int y = gridY + currentTrack * cellH;
        display.drawRect(x, y, cellW, cellH, 0xFFFF);
        if (playing && cursorX == step) {
          display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
        }
      }
      return;
    }
    if (key == KEY_RIGHT) {
      int oldX = cursorX;
      cursorX = (cursorX + 1) % numSteps;
      const int gridX = kGridX;
      const int gridY = kGridY;
      const int cellW = kCellW;
      const int cellH = kCellH;
      {
        int x = gridX + oldX * cellW;
        int y = gridY + currentTrack * cellH;
        if (tracks[currentTrack].pattern[oldX]) {
          uint16_t color = (currentTrack == 0) ? 0xF800 : (currentTrack == 1) ? 0x07E0 : (currentTrack == 2) ? 0x001F : (currentTrack == 3) ? 0xFFE0 : (currentTrack == 4) ? 0xFD20 : 0xF81F;
          display.fillRect(x, y, cellW, cellH, color);
        } else {
          display.drawRect(x, y, cellW, cellH, 0x3186);
        }
        if (playing && oldX == step) {
          display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
        }
      }
      {
        int x = gridX + cursorX * cellW;
        int y = gridY + currentTrack * cellH;
        display.drawRect(x, y, cellW, cellH, 0xFFFF);
        if (playing && cursorX == step) {
          display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
        }
      }
      return;
    }
    if (key == '+' || key == '=' || key == ']') {
      tracks[currentTrack].noteIdx++;
      updateTrackFreq(currentTrack);
      display.fillRect(8 + 6*6 + 30 + 6*5, 18, 40, 10, 0x0000);
      display.setTextColor(0xFFFF);
      display.setTextSize(1);
      display.setCursor(8 + 6*6 + 30 + 6*5, 18);
      display.print(tracks[currentTrack].noteName);
      return;
    }
    if (key == '-' || key == '_' || key == '[') {
      tracks[currentTrack].noteIdx--;
      updateTrackFreq(currentTrack);
      display.fillRect(8 + 6*6 + 30 + 6*5, 18, 40, 10, 0x0000);
      display.setTextColor(0xFFFF);
      display.setTextSize(1);
      display.setCursor(8 + 6*6 + 30 + 6*5, 18);
      display.print(tracks[currentTrack].noteName);
      return;
    }
    if (key == '>' || key == '.') {
      bpm = bpm < 200 ? bpm + 5 : 200;
      display.fillRect(200, 18, 60, 10, 0x0000);
      display.setTextColor(0xFFFF);
      display.setTextSize(1);
      display.setCursor(200, 18);
      char bpmbuf[12];
      snprintf(bpmbuf, sizeof(bpmbuf), "BPM:%d", (int)bpm);
      display.print(bpmbuf);
      return;
    }
    if (key == '<' || key == ',') {
      bpm = bpm > 40 ? bpm - 5 : 40;
      display.fillRect(200, 18, 60, 10, 0x0000);
      display.setTextColor(0xFFFF);
      display.setTextSize(1);
      display.setCursor(200, 18);
      char bpmbuf[12];
      snprintf(bpmbuf, sizeof(bpmbuf), "BPM:%d", (int)bpm);
      display.print(bpmbuf);
      return;
    }
    if (key == KEY_BACK) {
      stop();
      return;
    }
    if (key == 0x0D || key == '\n' || key == '\r') {
      tracks[currentTrack].pattern[cursorX] = !tracks[currentTrack].pattern[cursorX];
      const int gridX = kGridX;
      const int gridY = kGridY;
      const int cellW = kCellW;
      const int cellH = kCellH;
      int x = gridX + cursorX * cellW;
      int y = gridY + currentTrack * cellH;
      if (tracks[currentTrack].pattern[cursorX]) {
        uint16_t color = (currentTrack == 0) ? 0xF800 : (currentTrack == 1) ? 0x07E0 : (currentTrack == 2) ? 0x001F : (currentTrack == 3) ? 0xFFE0 : (currentTrack == 4) ? 0xFD20 : 0xF81F;
        display.fillRect(x, y, cellW, cellH, color);
      } else {
        display.fillRect(x, y, cellW, cellH, 0x0000);
        display.drawRect(x, y, cellW, cellH, 0x3186);
      }
      display.drawRect(x, y, cellW, cellH, 0xFFFF);
      if (playing && cursorX == step) {
        display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
      }
      return;
    }
  }
  
  bool isPlaying() { return playing; }
} 


void drawTracksWorkspace() {
  display.fillScreen(0x0000);
  display.setTextColor(0xFFFF);
  display.setTextSize(1);
  display.setCursor(8, 4);
  display.print("T.R.A.C.K.S - DAW");
  
  
  display.setCursor(8, 18);
  display.print("Track:");
  display.print(currentTrack + 1);
  display.print(" Note:");
  display.print(tracks[currentTrack].noteName);
  display.print(playing ? " PLAY" : " STOP");
  display.setCursor(200, 18);
  char bpmbuf[12];
  snprintf(bpmbuf, sizeof(bpmbuf), "BPM:%d", (int)bpm);
  display.print(bpmbuf);
  
  
  const int gridW = kGridW;
  const int gridH = kGridH;
  const int gridX = kGridX;
  const int gridY = kGridY;
  const int cellW = kCellW;
  const int cellH = kCellH;
  for (int t = 0; t < numTracks; t++) {
    for (int s = 0; s < numSteps; s++) {
      int x = gridX + s * cellW;
      int y = gridY + t * cellH;
      if (tracks[t].pattern[s]) {
        uint16_t color = (t == 0) ? 0xF800 : (t == 1) ? 0x07E0 : (t == 2) ? 0x001F : (t == 3) ? 0xFFE0 : (t == 4) ? 0xFD20 : 0xF81F;
        display.fillRect(x, y, cellW, cellH, color);
      } else {
        display.drawRect(x, y, cellW, cellH, 0x3186);
      }
      if (t == currentTrack && s == cursorX) {
        display.drawRect(x, y, cellW, cellH, 0xFFFF);
      }
      if (playing && s == step) {
        display.drawRect(x-1, y-1, cellW+2, cellH+2, 0xFFFF);
      }
    }
  }
  
  
  display.setCursor(8, 280);
  display.print("SPC:Play/Stop  ENT:Toggle  +/-:Note  ,/.:BPM");
  display.setCursor(8, 292);
  display.print("UP/DN:Track L/R:Move  BK:Back");
  lastDrawnStep = playing ? step : -1;
}

void handleTracksKey(uint8_t key) {
  EricTracks::handleKey(key);
}
