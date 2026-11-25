#include <Arduino.h>
#include <SD.h>
#include <Arduino_GFX_Library.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include "gameboy.h"





#define PEANUT_GB_HIGH_LCD_ACCURACY 0



#define PEANUT_GB_12_COLOUR 0
#include "peanut_gb.h"

namespace {







constexpr uint16_t ROM_CACHE_SIZE = 16384;
constexpr uint8_t ROM_CACHE_SLOTS = 8;
constexpr uint8_t FIXED_ROM_SLOT = 0;
constexpr uint8_t FIRST_BANKED_ROM_SLOT = 1;
constexpr size_t MAX_SAVE_RAM = 32768;
constexpr uint32_t STATE_MAGIC = 0x45524742; 
constexpr uint16_t STATE_VERSION = 1;
constexpr uint8_t GB_W = 160;
constexpr uint8_t GB_H = 144;
constexpr int16_t LCD_X = 0;
constexpr int16_t LCD_Y = 16;
constexpr uint32_t GB_FRAME_US = 16743UL; 



constexpr uint32_t PRESENT_INTERVAL_US = 0UL;




constexpr uint8_t FRAME_BUFFER_COUNT = 2;

struct StateHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t gbBytes;
  uint32_t pathHash;
  uint32_t ramBytes;
};

Arduino_GFX *screen = nullptr;
File romFile;
gb_s gb;
bool running = false;
volatile bool core1Active = false;
volatile bool core1Running = false;
volatile bool paused = false;
volatile uint8_t requestedButtons = 0;
volatile uint32_t buttonHoldUntil = 0;





uint8_t gbFrames[FRAME_BUFFER_COUNT][GB_W * GB_H];
volatile int8_t readyFrame = -1;
volatile int8_t presentingFrame = -1;
uint8_t renderFrame = 0;
bool frameTouched = false;
uint32_t lastPresentAt = 0;


volatile uint32_t emulatedFrames = 0;
volatile uint32_t lateFrames = 0;
volatile uint32_t worstFrameUs = 0;
volatile uint32_t romCacheMisses = 0;
volatile uint32_t worstRomReadUs = 0;
volatile uint32_t lcdBands = 0;
volatile uint32_t worstLcdBandUs = 0;
uint32_t lcdBandStartedAt = 0;

char activePath[256] = "";
char gameTitle[17] = "";
struct RomCacheSlot {
  uint8_t bytes[ROM_CACHE_SIZE];
  uint32_t base;
  uint32_t lastUsed;
};
RomCacheSlot romCache[ROM_CACHE_SLOTS];



RomCacheSlot *lastRomCacheSlot = nullptr;
uint32_t lastRomCacheBase = 0xFFFFFFFFUL;
uint32_t romCacheUseSerial = 0;
uint32_t romLength = 0;
uint8_t cartRam[MAX_SAVE_RAM];
size_t cartRamSize = 0;
uint32_t identity = 0;







const uint32_t scaledPairWords[4] = {
  0x00000000UL, 
  0x80018001UL, 
  0xE003E003UL, 
  0xE007E007UL  
};










constexpr uint8_t PRESENT_SOURCE_ROWS = 24;
constexpr uint16_t PRESENT_WIDTH = GB_W * 2;
constexpr uint16_t PRESENT_ROWS = PRESENT_SOURCE_ROWS * 2;
constexpr size_t PRESENT_BYTES = static_cast<size_t>(PRESENT_WIDTH) * PRESENT_ROWS * 2;
uint8_t scaledBand[PRESENT_BYTES];





int lcdDmaChannel = -1;
bool lcdDmaReady = false;
bool lcdGamePixelFormat = false;



bool lcdPresentActive = false;
uint8_t lcdPresentSourceY = 0;
uint8_t lcdPresentRows = 0;
int8_t lcdPresentFrame = -1;




constexpr uint8_t LCD_SCK_PIN = 10;
constexpr uint8_t LCD_MOSI_PIN = 11;
constexpr uint8_t LCD_CS_PIN = 13;
constexpr uint8_t LCD_DC_PIN = 14;



constexpr uint32_t LCD_DMA_BAUD = 80000000UL;

void lcdWriteCommand(uint8_t command) {
  gpio_put(LCD_CS_PIN, 0);
  gpio_put(LCD_DC_PIN, 0);
  spi_write_blocking(spi1, &command, 1);
}

void lcdWriteData(const uint8_t *data, size_t length) {
  gpio_put(LCD_DC_PIN, 1);
  spi_write_blocking(spi1, data, length);
}

void lcdSetWindow(int16_t x, int16_t y, int16_t width, int16_t height) {
  const uint16_t x1 = static_cast<uint16_t>(x + width - 1);
  const uint16_t y1 = static_cast<uint16_t>(y + height - 1);
  const uint8_t columns[] = {static_cast<uint8_t>(x >> 8), static_cast<uint8_t>(x), static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1)};
  const uint8_t rows[] = {static_cast<uint8_t>(y >> 8), static_cast<uint8_t>(y), static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1)};
  lcdWriteCommand(0x2A);
  lcdWriteData(columns, sizeof(columns));
  lcdWriteCommand(0x2B);
  lcdWriteData(rows, sizeof(rows));
  lcdWriteCommand(0x2C);
  gpio_put(LCD_DC_PIN, 1);
}

void beginLcdDma() {
  if (lcdDmaReady) return;
  
  
  gpio_set_function(LCD_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);
  spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  spi_set_baudrate(spi1, LCD_DMA_BAUD);
  lcdDmaChannel = dma_claim_unused_channel(false);
  if (lcdDmaChannel < 0) return;
  dma_channel_config config = dma_channel_get_default_config(lcdDmaChannel);
  channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
  channel_config_set_read_increment(&config, true);
  channel_config_set_write_increment(&config, false);
  channel_config_set_dreq(&config, spi_get_dreq(spi1, true));
  dma_channel_configure(lcdDmaChannel, &config, &spi_get_hw(spi1)->dr, nullptr, 0, false);
  lcdDmaReady = true;
}

void setLcdGamePixelFormat(bool gameMode) {
  if (lcdGamePixelFormat == gameMode) return;
  const uint8_t format = gameMode ? 0x55 : 0x66; 
  lcdWriteCommand(0x3A);
  lcdWriteData(&format, 1);
  gpio_put(LCD_CS_PIN, 1);
  lcdGamePixelFormat = gameMode;
}

void lcdDmaStart(const uint8_t *data, size_t length) {
  dma_channel_set_read_addr(lcdDmaChannel, data, false);
  dma_channel_set_trans_count(lcdDmaChannel, length, true);
}

bool lcdDmaFinished() {
  
  
  return !dma_channel_is_busy(lcdDmaChannel) && !spi_is_busy(spi1);
}

void packScaledBand(const uint8_t *source, uint8_t sourceY, uint8_t sourceRows) {
  constexpr size_t OUTPUT_ROW_BYTES = static_cast<size_t>(PRESENT_WIDTH) * 2;
  
  
  
  static_assert((PRESENT_WIDTH * 2) % sizeof(uint32_t) == 0, "rows must be word aligned");
  uint8_t *destination = scaledBand;
  for (uint8_t row = 0; row < sourceRows; ++row) {
    const uint8_t *line = source + static_cast<size_t>(sourceY + row) * GB_W;
    uint32_t *firstOutputRow = reinterpret_cast<uint32_t *>(destination);
    for (uint8_t x = 0; x < GB_W; ++x) {
      firstOutputRow[x] = scaledPairWords[line[x] & 0x03];
    }
    destination += OUTPUT_ROW_BYTES;
    
    memcpy(destination, firstOutputRow, OUTPUT_ROW_BYTES);
    destination += OUTPUT_ROW_BYTES;
  }
}

uint32_t hashPath(const char *text) {
  uint32_t h = 2166136261UL;
  while (*text) { h ^= static_cast<uint8_t>(*text++); h *= 16777619UL; }
  return h;
}

void savePath(char *out, size_t size, const char *suffix) {
  snprintf(out, size, "/ERIC/SAVES/%08lX%s", static_cast<unsigned long>(identity), suffix);
}

bool ensureSaveFolders() {
  if (!SD.exists("/ERIC") && !SD.mkdir("/ERIC")) return false;
  if (!SD.exists("/ERIC/SAVES") && !SD.mkdir("/ERIC/SAVES")) return false;
  if (!SD.exists("/ERIC/SAVES/STATES") && !SD.mkdir("/ERIC/SAVES/STATES")) return false;
  return true;
}

uint8_t romRead(gb_s *, const uint_fast32_t address) {
  const uint32_t byteAddress = static_cast<uint32_t>(address);
  if (!romFile || byteAddress >= romLength) return 0xFF;
  const uint32_t base = byteAddress & ~(ROM_CACHE_SIZE - 1);

  
  
  
  RomCacheSlot *slot = lastRomCacheSlot;
  if (base != lastRomCacheBase || slot == nullptr) {
    slot = nullptr;
    if (base == 0) {
      
      
      slot = &romCache[FIXED_ROM_SLOT];
      if (slot->base != 0) {
        const uint32_t readStartedAt = micros();
        if (!romFile.seek(0)) return 0xFF;
        const size_t got = romFile.read(slot->bytes, ROM_CACHE_SIZE);
        const uint32_t readElapsed = micros() - readStartedAt;
        ++romCacheMisses;
        if (readElapsed > worstRomReadUs) worstRomReadUs = readElapsed;
        if (got < ROM_CACHE_SIZE) memset(slot->bytes + got, 0xFF, ROM_CACHE_SIZE - got);
        slot->base = 0;
      }
    } else {
      for (uint8_t i = FIRST_BANKED_ROM_SLOT; i < ROM_CACHE_SLOTS; ++i) {
        if (romCache[i].base == base) { slot = &romCache[i]; break; }
      }
      if (!slot) {
        
        
        uint8_t victim = FIRST_BANKED_ROM_SLOT;
        for (uint8_t i = FIRST_BANKED_ROM_SLOT + 1; i < ROM_CACHE_SLOTS; ++i) {
          if (romCache[i].lastUsed < romCache[victim].lastUsed) victim = i;
        }
        slot = &romCache[victim];
        const uint32_t readStartedAt = micros();
        if (!romFile.seek(base)) return 0xFF;
        const size_t got = romFile.read(slot->bytes, ROM_CACHE_SIZE);
        const uint32_t readElapsed = micros() - readStartedAt;
        ++romCacheMisses;
        if (readElapsed > worstRomReadUs) worstRomReadUs = readElapsed;
        if (got < ROM_CACHE_SIZE) memset(slot->bytes + got, 0xFF, ROM_CACHE_SIZE - got);
        slot->base = base;
      }
      slot->lastUsed = ++romCacheUseSerial;
    }
    lastRomCacheSlot = slot;
    lastRomCacheBase = base;
  }
  return slot->bytes[byteAddress - base];
}

uint8_t ramRead(gb_s *, const uint_fast32_t address) {
  return address < cartRamSize ? cartRam[address] : 0xFF;
}

void ramWrite(gb_s *, const uint_fast32_t address, const uint8_t value) {
  if (address < cartRamSize) cartRam[address] = value;
}

void gbError(gb_s *, const gb_error_e, const uint16_t) {
  running = false;
  core1Active = false;
}




void drawLine(gb_s *, const uint8_t *pixels, const uint_fast8_t line) {
  if (line >= GB_H) return;
  uint8_t *destination = &gbFrames[renderFrame][static_cast<size_t>(line) * GB_W];
  memcpy(destination, pixels, GB_W);
  frameTouched = true;
}

uint8_t chooseRenderFrame() {
  const int8_t heldForDisplay = presentingFrame;
  const int8_t queuedForDisplay = readyFrame;
  for (uint8_t candidate = 0; candidate < FRAME_BUFFER_COUNT; ++candidate) {
    if (candidate != heldForDisplay && candidate != queuedForDisplay) return candidate;
  }
  
  
  
  for (uint8_t candidate = 0; candidate < FRAME_BUFFER_COUNT; ++candidate) {
    if (candidate != heldForDisplay) return candidate;
  }
  return 0;
}

bool flushCartridgeRam() {
  if (!running || cartRamSize == 0 || !ensureSaveFolders()) return cartRamSize == 0;
  char path[40];
  savePath(path, sizeof(path), ".sav");
  SD.remove(path);
  File file = SD.open(path, FILE_WRITE);
  if (!file) return false;
  const bool ok = file.write(cartRam, cartRamSize) == cartRamSize;
  file.close();
  return ok;
}

void loadCartridgeRam() {
  if (cartRamSize == 0) return;
  memset(cartRam, 0xFF, cartRamSize);
  char path[40];
  savePath(path, sizeof(path), ".sav");
  File file = SD.open(path, FILE_READ);
  if (file) { file.read(cartRam, cartRamSize); file.close(); }
}

void waitUntil(uint32_t target) {
  const int32_t remaining = static_cast<int32_t>(target - micros());
  if (remaining <= 0) return;
  if (remaining >= 2000) delay(static_cast<uint32_t>(remaining / 1000));
  const int32_t tail = static_cast<int32_t>(target - micros());
  if (tail > 0) delayMicroseconds(static_cast<uint32_t>(tail));
}
} 

namespace EricGameBoy {
void begin(Arduino_GFX *target) { screen = target; }

StartResult start(const char *path) {
  stop();
  if (!path) return StartResult::NoCard;
  romFile = SD.open(path, FILE_READ);
  if (!romFile) return StartResult::OpenFailed;
  if (romFile.size() < 0x150) { romFile.close(); return StartResult::InvalidRom; }

  memset(&gb, 0, sizeof(gb));
  romLength = romFile.size();
  for (uint8_t i = 0; i < ROM_CACHE_SLOTS; ++i) {
    romCache[i].base = 0xFFFFFFFFUL;
    romCache[i].lastUsed = 0;
  }
  lastRomCacheSlot = nullptr;
  lastRomCacheBase = 0xFFFFFFFFUL;
  romCacheUseSerial = 0;
  const gb_init_error_e init = gb_init(&gb, romRead, ramRead, ramWrite, gbError, nullptr);
  if (init == GB_INIT_INVALID_CHECKSUM) { romFile.close(); return StartResult::InvalidRom; }
  if (init != GB_INIT_NO_ERROR) { romFile.close(); return StartResult::UnsupportedCart; }
  if (gb_get_save_size_s(&gb, &cartRamSize) != 0 || cartRamSize > MAX_SAVE_RAM) {
    romFile.close();
    return StartResult::SaveTooLarge;
  }

  identity = hashPath(path);
  strncpy(activePath, path, sizeof(activePath) - 1);
  activePath[sizeof(activePath) - 1] = '\0';
  gb_get_rom_name(&gb, gameTitle);
  if (!gameTitle[0]) strncpy(gameTitle, "UNTITLED", sizeof(gameTitle) - 1);
  loadCartridgeRam();
  gb_init_lcd(&gb, drawLine);
  
  
  
  
  
  gb.direct.frame_skip = false;
  gb.direct.interlace = false;

  requestedButtons = 0;
  buttonHoldUntil = 0;
  paused = false;
  readyFrame = -1;
  presentingFrame = -1;
  lcdPresentFrame = -1;
  lcdPresentActive = false;
  lcdPresentSourceY = 0;
  lcdPresentRows = 0;
  renderFrame = 0;
  lastPresentAt = 0;
  emulatedFrames = 0;
  lateFrames = 0;
  worstFrameUs = 0;
  romCacheMisses = 0;
  worstRomReadUs = 0;
  lcdBands = 0;
  worstLcdBandUs = 0;
  lcdBandStartedAt = 0;
  running = true;
  core1Active = true;
  if (screen) screen->fillScreen(0x0000);
  return StartResult::Ok;
}

void serviceCore1() {
  core1Running = true;
  uint32_t nextFrameAt = micros();
  while (core1Active) {
    if (!running || paused) { delay(1); nextFrameAt = micros(); continue; }

    
    
    
    
    renderFrame = chooseRenderFrame();
    frameTouched = false;
    
    
    
    if (requestedButtons && static_cast<int32_t>(millis() - buttonHoldUntil) >= 0) {
      requestedButtons = 0;
    }
    gb.direct.joypad = static_cast<uint8_t>(~requestedButtons);
    const uint32_t frameStartedAt = micros();
    gb_run_frame(&gb);
    const uint32_t frameElapsed = micros() - frameStartedAt;
    ++emulatedFrames;
    if (frameElapsed > worstFrameUs) worstFrameUs = frameElapsed;
    if (frameElapsed > GB_FRAME_US) ++lateFrames;

    if (frameTouched) {
      readyFrame = static_cast<int8_t>(renderFrame);
    }

    
    
    nextFrameAt += GB_FRAME_US;
    const int32_t late = static_cast<int32_t>(micros() - nextFrameAt);
    if (late > static_cast<int32_t>(GB_FRAME_US * 2)) nextFrameAt = micros();
    else waitUntil(nextFrameAt);
  }
  core1Running = false;
}

void presentFrame() {
  if (!running || !screen) return;

  beginLcdDma();
  
  if (!lcdDmaReady) return;
  setLcdGamePixelFormat(true);

  if (lcdPresentActive) {
    
    if (!lcdDmaFinished()) return;
    const uint32_t bandElapsed = micros() - lcdBandStartedAt;
    ++lcdBands;
    if (bandElapsed > worstLcdBandUs) worstLcdBandUs = bandElapsed;
    lcdPresentSourceY += lcdPresentRows;
    gpio_put(LCD_CS_PIN, 1);
  } else {
    const uint32_t now = micros();
    if (PRESENT_INTERVAL_US != 0 && now - lastPresentAt < PRESENT_INTERVAL_US) return;
    const int8_t candidate = readyFrame;
    if (candidate < 0) return;
    lcdPresentFrame = candidate;
    presentingFrame = candidate;
    lcdPresentSourceY = 0;
  }

  if (lcdPresentSourceY >= GB_H) {
    if (readyFrame == lcdPresentFrame) readyFrame = -1;
    presentingFrame = -1;
    lcdPresentFrame = -1;
    lcdPresentActive = false;
    lastPresentAt = micros();
    return;
  }

  
  
  
  const uint8_t remainingRows = GB_H - lcdPresentSourceY;
  lcdPresentRows = remainingRows < PRESENT_SOURCE_ROWS ? remainingRows : PRESENT_SOURCE_ROWS;
  lcdSetWindow(LCD_X, LCD_Y + lcdPresentSourceY * 2, PRESENT_WIDTH, lcdPresentRows * 2);
  packScaledBand(gbFrames[lcdPresentFrame], lcdPresentSourceY, lcdPresentRows);
  lcdBandStartedAt = micros();
  lcdDmaStart(scaledBand, static_cast<size_t>(lcdPresentRows) * PRESENT_WIDTH * 2 * 2);
  lcdPresentActive = true;
}

void yieldDisplayToSystem() {
  
  
  
  if (lcdPresentActive && lcdDmaReady) {
    dma_channel_wait_for_finish_blocking(lcdDmaChannel);
    while (spi_is_busy(spi1)) { }
    gpio_put(LCD_CS_PIN, 1);
    lcdPresentActive = false;
  }
  if (lcdDmaReady) setLcdGamePixelFormat(false);
}

void setButtons(uint8_t pressed, uint32_t holdMs) {
  requestedButtons = pressed;
  buttonHoldUntil = millis() + holdMs;
}
void setPaused(bool value) {
  paused = value;
  if (value) { requestedButtons = 0; buttonHoldUntil = 0; }
}

void stop() {
  core1Active = false;
  const uint32_t deadline = millis() + 300;
  while (core1Running && millis() < deadline) delay(1);
  if (running) flushCartridgeRam();
  running = false;
  paused = false;
  requestedButtons = 0;
  buttonHoldUntil = 0;
  
  
  if (lcdPresentActive && lcdDmaReady) {
    dma_channel_wait_for_finish_blocking(lcdDmaChannel);
    while (spi_is_busy(spi1)) { }
    gpio_put(LCD_CS_PIN, 1);
  }
  lcdPresentActive = false;
  
  
  if (lcdDmaReady) setLcdGamePixelFormat(false);
  lcdPresentFrame = -1;
  lcdPresentSourceY = 0;
  lcdPresentRows = 0;
  readyFrame = -1;
  presentingFrame = -1;
  if (romFile) romFile.close();
  romLength = 0;
  for (uint8_t i = 0; i < ROM_CACHE_SLOTS; ++i) {
    romCache[i].base = 0xFFFFFFFFUL;
    romCache[i].lastUsed = 0;
  }
  lastRomCacheSlot = nullptr;
  lastRomCacheBase = 0xFFFFFFFFUL;
  romCacheUseSerial = 0;
  cartRamSize = 0;
  activePath[0] = '\0';
}

bool isRunning() { return running; }
PerformanceStats performanceStats() {
  PerformanceStats stats;
  noInterrupts();
  stats.emulatedFrames = emulatedFrames;
  stats.lateFrames = lateFrames;
  stats.worstFrameUs = worstFrameUs;
  stats.romCacheMisses = romCacheMisses;
  stats.worstRomReadUs = worstRomReadUs;
  stats.lcdBands = lcdBands;
  stats.worstLcdBandUs = worstLcdBandUs;
  interrupts();
  return stats;
}
const char *title() { return gameTitle; }
const char *romPath() { return activePath; }

bool saveState() {
  if (!running || !ensureSaveFolders()) return false;
  char path[48];
  snprintf(path, sizeof(path), "/ERIC/SAVES/STATES/%08lX.state", static_cast<unsigned long>(identity));
  SD.remove(path);
  File file = SD.open(path, FILE_WRITE);
  if (!file) return false;
  StateHeader header = {STATE_MAGIC, STATE_VERSION, static_cast<uint16_t>(sizeof(gb)), identity, static_cast<uint32_t>(cartRamSize)};
  const bool ok = file.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header)) == sizeof(header) &&
    file.write(reinterpret_cast<const uint8_t *>(&gb), sizeof(gb)) == sizeof(gb) &&
    file.write(cartRam, cartRamSize) == cartRamSize;
  file.close();
  if (ok) flushCartridgeRam();
  return ok;
}

bool loadState() {
  if (!running) return false;
  char path[48];
  snprintf(path, sizeof(path), "/ERIC/SAVES/STATES/%08lX.state", static_cast<unsigned long>(identity));
  File file = SD.open(path, FILE_READ);
  if (!file) return false;
  StateHeader header;
  const bool valid = file.read(reinterpret_cast<uint8_t *>(&header), sizeof(header)) == sizeof(header) &&
    header.magic == STATE_MAGIC && header.version == STATE_VERSION &&
    header.gbBytes == sizeof(gb) && header.pathHash == identity && header.ramBytes == cartRamSize;
  if (!valid) { file.close(); return false; }
  const bool ok = file.read(reinterpret_cast<uint8_t *>(&gb), sizeof(gb)) == sizeof(gb) &&
    file.read(cartRam, cartRamSize) == cartRamSize;
  file.close();
  if (ok) {
    gb.gb_rom_read = romRead;
    gb.gb_cart_ram_read = ramRead;
    gb.gb_cart_ram_write = ramWrite;
    gb.gb_error = gbError;
    gb.direct.priv = nullptr;
    gb.display.lcd_draw_line = drawLine;
  }
  return ok;
}
} 


void setup1() { }
void loop1() { EricGameBoy::serviceCore1(); }
