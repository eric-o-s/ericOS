#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <hardware/clocks.h>
#include <Arduino_GFX_Library.h>
#include "eric_config.h"
#include "keyboard.h"
#include "storage.h"
#include "file_types.h"
#include "audio_player.h"
#include "radio_player.h"
#include "gameboy.h"
#include "bmp_viewer.h"
#include "jpeg_viewer.h"
#include "file_transfer.h"
#include "weather_api.h"
#include "weather_data.h"
#include "audio_engine.h"
#include "tracks.h"




enum class AppId : uint8_t { Home, Shell, SystemInfo, Scratchpad, Explorer, ImageViewer, AudioPlayer, Radio, GameBoy, Network, Calculator, Weather, Tracks }; 

enum class HomeTile : uint8_t { Shell = 0, Explorer = 1, AudioPlayer = 2, Radio = 3, Scratchpad = 4, GameBoy = 5, System = 6, Network = 7, Calculator = 8, Weather = 9, Tracks = 10 };



enum class WeatherMode : uint8_t { Search, Results, Now };

enum class NetworkMode : uint8_t { Survey, Password, Status, Transfer };

enum class GameMode : uint8_t { Browser, Playing, Menu, Notice };

enum class GameMenuItem : uint8_t { Resume = 0, SaveState = 1, LoadState = 2, ExitHome = 3 };

enum class QuickAction : uint8_t { NewNote = 0, SaveNote = 1, Dashboard = 2, Back = 3, Close = 4 };

enum class ScratchpadModal : uint8_t { None, ExitConfirm };

enum class ExplorerMode : uint8_t { Browse, Actions, NameEntry, DeleteConfirm, Result };

enum class ExplorerAction : uint8_t { Open = 0, Edit = 1, Rename = 2, Delete = 3, NewFile = 4, NewFolder = 5, Cancel = 6 };

enum class SoundMode : uint8_t { FolderSelect, Playlist, NowPlaying };

enum class CalcView : uint8_t { Graph, Convert };

struct CalcUnit { const char *name; float baseFactor; };
struct CalcConversionFamily { const CalcUnit *units; uint8_t count; };








































void openBlueExchangeWorkspace();
void drawBlueExchangeStatus();
void drawBlueControls();
void drawBlueControlRow(uint8_t index);
void drawBlueControlSelection(uint8_t previous);
void drawBlueNotice();
void drawBlueTransferMenu();
void drawBlueTransferRow(uint8_t index);
void drawBlueTransferSelection(uint8_t previous);
void drawBluePairing();
void drawBlueExchangeWorkspace();
void handleBlueExchangeKey(uint8_t key);

bool isBmpFile(const char *name);
bool isJpegFile(const char *name);

void formatCalculatorMeasurement(float value, uint8_t decimals, char *out, size_t outSize);

void drawCalculatorUnitPicker();

void formatCalculatorConversionValue(float value, char *out, size_t outSize);

bool calcReadConversionInput(float &value);

void calcUseOppositePane();

void calcSkipSpaces();
float calcPrimary();
float calcPower();
float calcTerm();
bool evaluateCalculator(float x, float &value);

uint8_t homeCarouselIndex(HomeTile tile);

void oldRadioWorkspace();

void openNetworkWorkspace();
void drawNetworkWorkspace();
void openWeatherWorkspace();
void drawWeatherWorkspace();
void handleWeatherKey(uint8_t key);
void serviceWeather();
void drawBrowserWorkspace();
void handleBrowserKey(uint8_t key);
void serviceBrowser();
void drawNetworkPasswordInput();
void drawNetworkRow(uint8_t index);
void drawNetworkSelection(uint8_t previous);
void handleNetworkKey(uint8_t key);
void scanNearbyNetworks();
void startSelectedNetwork();
void serviceNetworkConnection();
void drawNetworkTransfer();
void openRadioWorkspace();
void drawRadioWorkspace();
void drawRadioRow(uint8_t index);
void drawRadioSelection(uint8_t previous);
void handleRadioKey(uint8_t key);

bool isGameRom(const char *name);

void clippedText(char *out, size_t outSize, const char *value, uint8_t maxChars);

void drawQuickMenu();

void drawPixelLetter(const uint8_t rows[7], int16_t x, int16_t y, uint8_t scale);

void text(uint16_t color, uint8_t size, int16_t x, int16_t y, const char *value);
void drawRoundedPanel(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t fill, uint16_t edge);
void ditherRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint8_t spacing = 4);
void ditherGradientRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t dark, uint16_t light);
void drawTopBar(const char *title, const char *subtitle);
void drawWirelessStatusIcon();
void serviceWirelessStatusIcon();
void drawFooter(const char *value);
void drawBootScreen();
void drawHomeTile(HomeTile tile, int16_t x, int16_t y, int16_t w, int16_t h, const char *icon, const char *name, const char *detail, const char *detailLine2, bool focused);
bool saveNote();
void loadNote();
bool saveNoteToSD();
bool loadNoteFromSD();
void handleShellKey(uint8_t key);
void handleScratchpadKey(uint8_t key);
void handleExplorerKey(uint8_t key);
void drawAudioPlayer();
void handleAudioPlayerKey(uint8_t key);
void openSoundWorkspace();
void refreshSoundFolders();
void scanSoundFolder();
void drawSoundFolderSelect();
void drawSoundPlaylist();
void drawSoundFolderRow(uint8_t index);
void drawSoundFolderSelection(uint8_t previous);
bool startSoundTrack(uint8_t index);
void openSelectedTile();
void handleHomeKey(uint8_t key);
void drawCalculator();
void drawCalculatorInput();
void drawCalculatorGraph();
void drawCalculatorConverter();
void drawCalculatorConversionValue();
float calculatorConvertedValue(float input, bool fromLeft);
void handleCalculatorKey(uint8_t key);

constexpr uint8_t LCD_SCK = 10;
constexpr uint8_t LCD_MOSI = 11;
constexpr uint8_t LCD_MISO = 12;
constexpr uint8_t LCD_CS = 13;
constexpr uint8_t LCD_DC = 14;
constexpr uint8_t LCD_RST = 15;






constexpr uint8_t KEY_BACK = EricConfig::KeyBack;
constexpr uint8_t KEY_ENTER = EricConfig::KeyEnter;
constexpr uint8_t KEY_ESC = EricConfig::KeyEsc;
constexpr uint8_t KEY_LEFT = EricConfig::KeyLeft;
constexpr uint8_t KEY_UP = EricConfig::KeyUp;
constexpr uint8_t KEY_DOWN = EricConfig::KeyDown;
constexpr uint8_t KEY_RIGHT = EricConfig::KeyRight;
constexpr uint8_t KEY_DELETE = EricConfig::KeyDelete;
constexpr uint8_t KEY_F1 = EricConfig::KeyF1;
constexpr uint8_t KEY_JOY_UP = EricConfig::KeyJoyUp;
constexpr uint8_t KEY_JOY_DOWN = EricConfig::KeyJoyDown;
constexpr uint8_t KEY_JOY_LEFT = EricConfig::KeyJoyLeft;
constexpr uint8_t KEY_JOY_RIGHT = EricConfig::KeyJoyRight;
constexpr uint8_t KEY_JOY_CENTER = EricConfig::KeyJoyCenter;


constexpr int16_t SCREEN_W = 320;
constexpr int16_t SCREEN_H = 320;
constexpr int16_t SAFE_LEFT = 8;
constexpr int16_t SAFE_RIGHT = 312;
constexpr int16_t SAFE_WIDTH = SAFE_RIGHT - SAFE_LEFT;





constexpr int16_t HEADER_BRAND_X = SAFE_LEFT + 4;
constexpr int16_t HEADER_BRAND_Y = 8;
constexpr int16_t HEADER_TITLE_X = SAFE_LEFT + 4;
constexpr int16_t HEADER_TITLE_Y = 22;
constexpr int16_t HEADER_SUBTITLE_X = SAFE_LEFT + 116;
constexpr int16_t HEADER_SUBTITLE_Y = 8;
constexpr int16_t HEADER_DIVIDER_Y = 50;

Arduino_RPiPicoSPI lcdBus(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI, LCD_MISO, spi1);



Arduino_ILI9488_18bit display(&lcdBus, LCD_RST, 0, false);







constexpr uint16_t COLOR_BLACK = 0x0000;
constexpr uint16_t COLOR_BG = COLOR_BLACK;
constexpr uint16_t COLOR_SURFACE = 0x0080;
constexpr uint16_t COLOR_SURFACE_2 = 0x0180;
constexpr uint16_t COLOR_TEXT = 0x07E0;
constexpr uint16_t COLOR_MUTED = 0x03E0;
constexpr uint16_t COLOR_ACCENT = 0x07E0;
constexpr uint16_t COLOR_ACCENT_DARK = 0x01E0;
constexpr uint16_t COLOR_GREEN = 0x05E0;
constexpr uint16_t COLOR_HIGHLIGHT = 0x07E0;

constexpr uint8_t MAX_COMMAND = 39;
constexpr uint8_t HISTORY_LINES = 10;

constexpr uint8_t FIRST_LINE_Y = 104;
constexpr uint8_t LINE_HEIGHT = 18;

AppId activeApp = AppId::Home;
AppId previousApp = AppId::Home;


HomeTile selectedTile = HomeTile::Network;
char commandLine[MAX_COMMAND + 1] = "";


constexpr uint16_t NOTE_CAPACITY = EricConfig::ScratchpadCapacity;
char noteBuffer[NOTE_CAPACITY + 1] = "";
uint16_t noteLength = 0;
char history[HISTORY_LINES][MAX_COMMAND + 8] = {};
uint8_t commandLength = 0;
uint8_t historyCount = 0;
uint32_t lastPoll = 0;

uint32_t lastTransferRedraw = 0;
bool quickMenuVisible = false;
QuickAction quickAction = QuickAction::NewNote;
bool noteDirty = false;
bool sdReady = false;

ScratchpadModal scratchpadModal = ScratchpadModal::None;
AppId pendingScratchDestination = AppId::Home;
bool pendingNewScratchpad = false;




constexpr uint8_t EXPLORER_ITEMS = EricConfig::ExplorerPageItems;
constexpr uint8_t EXPLORER_NAME_LEN = EricConfig::ExplorerNameLength;



constexpr size_t EXPLORER_PATH_CAP = 128;
char explorerPath[EXPLORER_PATH_CAP] = "/";
char explorerNames[EXPLORER_ITEMS][EXPLORER_NAME_LEN + 1] = {};




char explorerPaths[EXPLORER_ITEMS][EXPLORER_PATH_CAP] = {};
bool explorerDirs[EXPLORER_ITEMS] = {};
uint8_t explorerCount = 0;
uint8_t explorerSelection = 0;


uint16_t explorerOffset = 0;
uint16_t explorerTotal = 0;
bool explorerMore = false;
char explorerStatus[32] = "SD CARD";
ExplorerMode explorerMode = ExplorerMode::Browse;
ExplorerAction explorerAction = ExplorerAction::Open;
constexpr uint8_t EXPLORER_INPUT_LEN = EricConfig::ExplorerInputLength;
char explorerInput[EXPLORER_INPUT_LEN + 1] = "";
uint8_t explorerInputLength = 0;


uint8_t explorerCursor = 0;
char notePath[EXPLORER_PATH_CAP] = "/ERIC/SCRATCH.TXT";
char explorerResult[40] = "";


char explorerNameError[40] = "";
uint32_t lastKeyAt = 0;
uint8_t lastKeyCode = 0;



bool keyboardAwaitingRelease = false;



constexpr uint8_t SOUND_FOLDER_ITEMS = 8;
constexpr uint8_t SOUND_PLAYLIST_ITEMS = 12;
constexpr uint8_t SOUND_NAME_LEN = 31;



constexpr size_t SOUND_PATH_LEN = 256;
char soundFolder[SOUND_PATH_LEN] = "/";
char soundFolders[SOUND_FOLDER_ITEMS][EXPLORER_NAME_LEN + 1] = {};
char soundFolderPaths[SOUND_FOLDER_ITEMS][SOUND_PATH_LEN] = {};
bool soundFolderIsDirectory[SOUND_FOLDER_ITEMS] = {};
uint8_t soundFolderCount = 0;
uint8_t soundFolderSelection = 0;
char soundTracks[SOUND_PLAYLIST_ITEMS][SOUND_PATH_LEN] = {};
uint8_t soundTrackCount = 0;
uint8_t soundTrackSelection = 0;
bool soundPlaylistTruncated = false;
bool soundReturnToExplorer = false;
SoundMode soundMode = SoundMode::FolderSelect;



uint8_t radioSelection = 0;
EricRadio::State lastRadioState = EricRadio::State::Idle;



constexpr uint8_t NETWORK_ITEMS = 8;
constexpr uint8_t NETWORK_NAME_LEN = 32;
constexpr uint8_t NETWORK_PASSWORD_LEN = 63;
char networkNames[NETWORK_ITEMS][NETWORK_NAME_LEN + 1] = {};
int16_t networkRssi[NETWORK_ITEMS] = {};
bool networkSecure[NETWORK_ITEMS] = {};
uint8_t networkCount = 0;
uint8_t networkSelection = 0;
char networkPassword[NETWORK_PASSWORD_LEN + 1] = "";
uint8_t networkPasswordLength = 0;

bool networkPasswordRevealed = false;
NetworkMode networkMode = NetworkMode::Survey;
uint32_t networkConnectStartedAt = 0;




bool networkJoinPending = false;
char networkStatus[40] = "PRESS ENTER TO SCAN";


int8_t lastWirelessIconState = -1;
uint32_t lastWirelessIconCheckAt = 0;



constexpr uint8_t CALC_EXPR_LEN = 40;
char calcExpression[CALC_EXPR_LEN + 1] = "x*x";
uint8_t calcExpressionLength = 3;


uint8_t calcCursor = 3;


float calcCenterX = 0.0f;
float calcProbeX = 0.0f;
float calcCenterY = 0.0f;
float calcRange = 10.0f;
char calcStatus[34] = "ENTER DRAWS GRAPH";
const char *calcParseText = nullptr;
uint8_t calcParseAt = 0;
float calcParseX = 0.0f;
bool calcParseOk = true;







constexpr CalcUnit CALC_TEMPERATURE_UNITS[] = {{"C", 1.0f}, {"F", 1.0f}, {"K", 1.0f}};
constexpr CalcUnit CALC_DISTANCE_UNITS[] = {{"KM", 1000.0f}, {"M", 1.0f}, {"MI", 1609.344f}, {"FT", 0.3048f}, {"YD", 0.9144f}, {"IN", 0.0254f}, {"CM", 0.01f}, {"MM", 0.001f}};
constexpr CalcUnit CALC_MASS_UNITS[] = {{"KG", 1.0f}, {"G", 0.001f}, {"OZ", 0.028349523f}, {"TON", 1000.0f}, {"LB", 0.45359237f}};
constexpr uint8_t CALC_CONVERSION_COUNT = 3;
const CalcConversionFamily calcConversionFamilies[CALC_CONVERSION_COUNT] = {
  {CALC_TEMPERATURE_UNITS, 3}, {CALC_DISTANCE_UNITS, 8}, {CALC_MASS_UNITS, 5}
};
CalcView calcView = CalcView::Graph;
uint8_t calcConversionSelection = 0;
uint8_t calcLeftUnit = 0;
uint8_t calcRightUnit = 1;
constexpr uint8_t CALC_VALUE_LEN = 16;
char calcValueInput[CALC_VALUE_LEN + 1] = "";
uint8_t calcValueLength = 0;
bool calcValueFromLeft = true;
bool calcUnitPickerVisible = false;
bool calcUnitPickerForLeft = true;
uint8_t calcUnitPickerSelection = 0;
char calcConversionStatus[36] = "TYPE IN EITHER PANE";



constexpr uint8_t GAME_BROWSER_ITEMS = 8;
constexpr size_t GAME_PATH_LEN = 256;
char gameFolder[GAME_PATH_LEN] = "/";


char gameEntries[GAME_BROWSER_ITEMS][EXPLORER_NAME_LEN + 1] = {};
char gameEntryPaths[GAME_BROWSER_ITEMS][GAME_PATH_LEN] = {};
bool gameEntryIsDirectory[GAME_BROWSER_ITEMS] = {};
uint8_t gameEntryCount = 0;
uint8_t gameSelection = 0;
GameMode gameMode = GameMode::Browser;
GameMenuItem gameMenuItem = GameMenuItem::Resume;
char gameNotice[48] = "";

void openGameWorkspace();
void refreshGameFolder();
void drawGameBrowser();
void drawGameBrowserRow(uint8_t index);
void drawGameBrowserSelection(uint8_t previous);
void drawGameMenu();
void drawGameNotice(const char *message);
void handleGameKey(uint8_t key);
void runGameFrame();
void switchApp(AppId next);
void goBack();
void drawHome();
void drawShell();
void drawSystemInfo();
void drawScratchpad();
void drawScratchpadBody();
void drawScratchpadExitConfirm();
void beginNewScratchpad();
void requestNewScratchpad();
void requestScratchpadExit(AppId destination);
void drawExplorer();
void drawExplorerSelection();
void drawExplorerPreview();
void refreshExplorer();
void openExplorerSelection();
void drawExplorerActionMenu();
void drawExplorerNameEntry();
void drawExplorerNameInput();
void drawExplorerDeleteConfirm();
void drawExplorerResult();
void showExplorerResult(const char *message);
void openAudioFile(const char *path);
void openBmpViewer(const char *path);
void openJpegViewer(const char *path);
void drawBmpViewer();
void drawJpegViewer();
void handleBmpViewerKey(uint8_t key);
bool drawJpegBlock(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t *pixels);
void beginExplorerNameEntry(ExplorerAction action);
void commitExplorerNameEntry();
bool selectedExplorerPath(char *out, size_t outSize);
bool selectedExplorerName(char *out, size_t outSize);
bool isTextFile(const char *name);
const char *fileOpenHint(const char *name);
bool removeExplorerTree(const char *path);
void drawHomeSelection();
void drawPrompt();
void runCommand();
void addHistory(const char *line);
int readKeyboardEvent();

void text(uint16_t color, uint8_t size, int16_t x, int16_t y, const char *value) {
  display.setTextSize(size);
  display.setTextColor(color);
  display.setCursor(x, y);
  display.print(value);
}



void clippedText(char *out, size_t outSize, const char *value, uint8_t maxChars) {
  if (maxChars < 4 || strlen(value) <= maxChars) {
    strncpy(out, value, outSize - 1);
    out[outSize - 1] = '\0';
    return;
  }
  const uint8_t kept = maxChars - 3;
  strncpy(out, value, kept);
  out[kept] = '\0';
  strncat(out, "...", outSize - strlen(out) - 1);
}

void drawRoundedPanel(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t fill, uint16_t edge) {
  display.fillRoundRect(x, y, w, h, 8, fill);
  display.drawRoundRect(x, y, w, h, 8, edge);
}


void ditherRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint8_t spacing) {
  for (int16_t row = y; row < y + h; row += spacing) {
    for (int16_t col = x + ((row / spacing) & 1 ? spacing / 2 : 0); col < x + w; col += spacing) {
      display.drawPixel(col, row, color);
    }
  }
}


void ditherGradientRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t dark, uint16_t light) {
  for (int16_t row = 0; row < h; ++row) {
    const uint8_t level = (static_cast<uint32_t>(row) * 4) / h;
    for (int16_t col = 0; col < w; ++col) {
      const uint8_t threshold = ((col & 1) << 1) | (row & 1);
      display.drawPixel(x + col, y + row, threshold <= level ? light : dark);
    }
  }
}



void drawPixelLetter(const uint8_t rows[7], int16_t x, int16_t y, uint8_t scale) {
  for (uint8_t row = 0; row < 7; ++row) {
    for (uint8_t col = 0; col < 5; ++col) {
      if (rows[row] & (1 << (4 - col))) {
        display.fillRect(x + col * scale, y + row * scale, scale - 1, scale - 1, COLOR_ACCENT);
      }
    }
  }
}

void drawBootScreen() {
  static const uint8_t LETTER_E[7] = {0x1F, 0x10, 0x1E, 0x10, 0x10, 0x10, 0x1F};
  static const uint8_t LETTER_R[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
  static const uint8_t LETTER_I[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F};
  static const uint8_t LETTER_C[7] = {0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F};
  constexpr uint8_t SCALE = 8;

  display.fillScreen(COLOR_BLACK);
  ditherGradientRect(8, 20, 304, 250, COLOR_BLACK, COLOR_SURFACE);
  display.drawRect(8, 20, 304, 250, COLOR_ACCENT_DARK);
  
  
  
  
  text(COLOR_MUTED, 1, 70, 36, "D E L A C R U Z  S Y S T E M S");

  
  
  
  drawPixelLetter(LETTER_E, 48, 68, SCALE);
  
  display.fillRect(92, 116, SCALE - 1, SCALE - 1, COLOR_ACCENT);
  drawPixelLetter(LETTER_R, 104, 68, SCALE);
  display.fillRect(152, 116, SCALE - 1, SCALE - 1, COLOR_ACCENT);
  drawPixelLetter(LETTER_I, 168, 68, SCALE);
  display.fillRect(216, 116, SCALE - 1, SCALE - 1, COLOR_ACCENT);
  drawPixelLetter(LETTER_C, 232, 68, SCALE);

  display.drawFastHLine(24, 140, 272, COLOR_ACCENT);
  
  
  text(COLOR_TEXT, 1, 40, 158, "[ Embedded RP2350 Interactive Computer ]");
  
  text(COLOR_TEXT, 1, 82, 174, "E.R.I.C by DeLaCruz Systems");
  
  text(COLOR_MUTED, 1, 142, 190, "v1.0.0");
  text(COLOR_MUTED, 1, 112, 232, "INITIALIZING...");
  
  delay(5000);
}



void drawWirelessStatusIcon() {
  constexpr int16_t ICON_X = 286;
  constexpr int16_t ICON_BASELINE = 18;
  const bool connected = WiFi.status() == WL_CONNECTED;
  uint8_t bars = 0;
  if (connected) {
    const int32_t rssi = WiFi.RSSI();
    bars = rssi >= -55 ? 4 : (rssi >= -67 ? 3 : (rssi >= -75 ? 2 : 1));
  }
  for (uint8_t i = 0; i < 4; ++i) {
    const int16_t height = 3 + i * 3;
    const uint16_t color = (i < bars) ? COLOR_ACCENT : COLOR_ACCENT_DARK;
    display.fillRect(ICON_X + i * 5, ICON_BASELINE - height, 3, height, color);
  }
}

void serviceWirelessStatusIcon() {
  if (millis() - lastWirelessIconCheckAt < 1000UL) return;
  lastWirelessIconCheckAt = millis();
  uint8_t bars = 0;
  if (WiFi.status() == WL_CONNECTED) {
    const int32_t rssi = WiFi.RSSI();
    bars = rssi >= -55 ? 4 : (rssi >= -67 ? 3 : (rssi >= -75 ? 2 : 1));
  }
  
  if (static_cast<int8_t>(bars) == lastWirelessIconState) return;
  lastWirelessIconState = static_cast<int8_t>(bars);
  
  display.fillRect(284, 2, 28, 18, COLOR_BLACK);
  drawWirelessStatusIcon();
}



void drawTopBar(const char *title, const char *subtitle) {
  display.fillRect(0, 0, SCREEN_W, 52, COLOR_BLACK);
  display.fillRect(SAFE_LEFT, HEADER_DIVIDER_Y, SAFE_WIDTH, 2, COLOR_ACCENT);
  text(COLOR_ACCENT, 1, HEADER_BRAND_X, HEADER_BRAND_Y, "// E.R.I.C");
  text(COLOR_TEXT, 2, HEADER_TITLE_X, HEADER_TITLE_Y, title);
  
  if (subtitle != nullptr && subtitle[0] != '\0') {
    text(COLOR_MUTED, 1, HEADER_SUBTITLE_X, HEADER_SUBTITLE_Y, subtitle);
  }
  drawWirelessStatusIcon();
}

void drawFooter(const char *value) {
  display.fillRect(0, 286, SCREEN_W, 34, COLOR_BLACK);
  display.drawFastHLine(SAFE_LEFT, 286, SAFE_WIDTH, COLOR_ACCENT_DARK);
  text(COLOR_MUTED, 1, SAFE_LEFT + 4, 300, value);
}

void drawQuickMenu() {
  drawRoundedPanel(40, 66, 240, 186, COLOR_SURFACE, COLOR_ACCENT);
  ditherRect(46, 72, 228, 174, COLOR_ACCENT_DARK, 6);

  
  
  
  text(COLOR_ACCENT, 1, 58, 84, "E.R.I.C QUICK MENU");
  const char *menuItems[] = {"New Scratchpad", "Save Scratchpad", "Dashboard", "Previous screen", "Close menu"};
  for (uint8_t i = 0; i < 5; ++i) {
    const int16_t y = 108 + i * 20;
    const bool selected = i == static_cast<uint8_t>(quickAction);
    if (selected) display.fillRoundRect(54, y - 2, 212, 16, 4, COLOR_SURFACE_2);
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 62, y, selected ? ">" : " ");
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 76, y, menuItems[i]);
  }
  text(COLOR_MUTED, 1, 58, 222, "UP/DOWN choose  ENTER run");
}


void drawScratchpadExitConfirm() {
  drawRoundedPanel(28, 92, 264, 142, COLOR_SURFACE, COLOR_ACCENT);
  ditherRect(34, 98, 252, 130, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 46, 110, "UNSAVED CHANGES");
  text(COLOR_TEXT, 1, 46, 134, "Save before leaving Scratchpad?");
  text(COLOR_MUTED, 1, 46, 158, "ENTER SAVE + CONTINUE");
  text(COLOR_MUTED, 1, 46, 176, "DEL DISCARD CHANGES");
  text(COLOR_MUTED, 1, 46, 194, "BACK OR ESC CANCEL");
}

void drawHomeTile(HomeTile tile, int16_t x, int16_t y, int16_t w, int16_t h, const char *icon, const char *name, const char *detail, const char *detailLine2, bool focused) {
  const uint16_t fill = focused ? COLOR_SURFACE_2 : COLOR_SURFACE;
  const uint16_t edge = focused ? COLOR_ACCENT : COLOR_ACCENT_DARK;
  drawRoundedPanel(x, y, w, h, fill, edge);
  if (focused) ditherRect(x + 4, y + 4, w - 8, h - 8, COLOR_ACCENT_DARK, 6);
  const int16_t iconX = x + (w - 28) / 2;
  display.drawCircle(iconX + 14, y + 25, 14, focused ? COLOR_ACCENT : COLOR_MUTED);
  text(focused ? COLOR_ACCENT : COLOR_MUTED, 1, iconX + 8, y + 19, icon);
  if (w >= 150) {
    text(focused ? COLOR_TEXT : COLOR_MUTED, 2, x + 18, y + 48, name);
    text(COLOR_MUTED, 1, x + 18, y + 69, detail);
    if (detailLine2 != nullptr && detailLine2[0] != '\0') text(COLOR_MUTED, 1, x + 18, y + 82, detailLine2);
  }
}



constexpr uint8_t HOME_TILE_COUNT = 11;
static const HomeTile HOME_CAROUSEL_ORDER[HOME_TILE_COUNT] = {
  HomeTile::Network, HomeTile::Radio, HomeTile::AudioPlayer, HomeTile::GameBoy,
  HomeTile::Calculator, HomeTile::Scratchpad, HomeTile::Explorer,
  HomeTile::System, HomeTile::Shell, HomeTile::Weather, HomeTile::Tracks
};

uint8_t homeCarouselIndex(HomeTile tile) {
  for (uint8_t i = 0; i < HOME_TILE_COUNT; ++i) {
    if (HOME_CAROUSEL_ORDER[i] == tile) return i;
  }
  return 0;
}












constexpr int16_t HOME_CONTENT_TOP = 52;      
constexpr int16_t HOME_CONTENT_BOTTOM = 286;   
constexpr int16_t HOME_CAPTION_H = 10;         
constexpr int16_t HOME_CAPTION_GAP = 6;        
constexpr int16_t HOME_TILE_H = 108;           
constexpr int16_t HOME_SIDE_TILE_DY = 20;      
constexpr int16_t HOME_SIDE_TILE_H = 78;
constexpr int16_t HOME_DOTS_DY = 118;          
constexpr int16_t HOME_DOTS_R = 3;

constexpr int16_t HOME_CONTENT_H = HOME_CAPTION_H + HOME_CAPTION_GAP + HOME_TILE_H;
constexpr int16_t HOME_CAPTION_Y = HOME_CONTENT_TOP + (HOME_CONTENT_BOTTOM - HOME_CONTENT_TOP - HOME_CONTENT_H) / 2;
constexpr int16_t HOME_TILE_Y = HOME_CAPTION_Y + HOME_CAPTION_H + HOME_CAPTION_GAP;

void drawHomeSelection() {
  static const char *icons[] = {">_", "[]", ">))", "()", "*", "GB", "i", "~", "f(x)", "ST", "TR", "</>"};
  static const char *names[] = {"S.H.E.L.L.", "F.I.L.E.S.", "S.O.U.N.D.", "R.A.D.I.O.", "M.E.M.O.", "G.A.M.E.", "S.Y.S.T.E.M.", "W.I.F.I.", "A.D.D.", "S.T.O.R.M.", "T.R.A.C.K.S"};
  static const char *details[] = {"Somehow Handling Every", "Folder Inspection & Location", "System Operated Universal", "Really Awesome Digital", "Mistakes Eventually", "Generating Absolutely Minimal", "Super Young System That", "Wacky Internet", "Arithmetic Decoder", "Standard Temperature &", "Tones Rhythms And"};
  static const char *detailLines2[] = {"Line Literally", "Execution Scanner", "Noise Dispenser", "Internet Output", "Manifest Otherwise", "Emulation", "Electronically Manages", "Frequencing Invention", "Device", "Overcast Radar Module", "Classic Keys Synth"};
  const uint8_t carouselCurrent = homeCarouselIndex(selectedTile);
  const uint8_t carouselBefore = (carouselCurrent + HOME_TILE_COUNT - 1) % HOME_TILE_COUNT;
  const uint8_t carouselAfter = (carouselCurrent + 1) % HOME_TILE_COUNT;
  const uint8_t before = static_cast<uint8_t>(HOME_CAROUSEL_ORDER[carouselBefore]);
  const uint8_t current = static_cast<uint8_t>(HOME_CAROUSEL_ORDER[carouselCurrent]);
  const uint8_t after = static_cast<uint8_t>(HOME_CAROUSEL_ORDER[carouselAfter]);
  
  
  
  display.fillRect(SAFE_LEFT, HOME_TILE_Y, SAFE_WIDTH, HOME_TILE_H + 1, COLOR_BLACK);
  drawHomeTile(HOME_CAROUSEL_ORDER[carouselBefore], -58, HOME_TILE_Y + HOME_SIDE_TILE_DY, 112, HOME_SIDE_TILE_H, icons[before], names[before], details[before], detailLines2[before], false);
  drawHomeTile(HOME_CAROUSEL_ORDER[carouselCurrent], 58, HOME_TILE_Y, 204, HOME_TILE_H, icons[current], names[current], details[current], detailLines2[current], true);
  drawHomeTile(HOME_CAROUSEL_ORDER[carouselAfter], 266, HOME_TILE_Y + HOME_SIDE_TILE_DY, 112, HOME_SIDE_TILE_H, icons[after], names[after], details[after], detailLines2[after], false);
}

void drawHome() {
  display.fillScreen(COLOR_BG);
  drawTopBar("HOME", "");
  text(COLOR_ACCENT, 1, 14, HOME_CAPTION_Y, "[ WORKSPACES ]");
  drawHomeSelection();
  drawFooter("LEFT/RIGHT BROWSE  ENTER OPEN  ESC MENU");
}

void drawPrompt() {
  constexpr int16_t y = 286;
  display.fillRect(0, y, 320, 34, COLOR_BLACK);
  display.drawFastHLine(0, y, 320, COLOR_SURFACE_2);
  
  text(COLOR_ACCENT, 1, 9, y + 10, "eric> ");
  display.setTextColor(COLOR_TEXT);
  display.setTextSize(1);
  display.print(commandLine);
  display.fillRect(45 + commandLength * 6, y + 9, 5, 9, COLOR_ACCENT);
}

void drawShell() {
  display.fillScreen(COLOR_BG);
  drawTopBar("S.H.E.L.L.", "");
  
  
  text(COLOR_ACCENT, 1, SAFE_LEFT + 4, 60, "SOMEHOW HANDLING EVERY LINE");
  text(COLOR_ACCENT, 1, SAFE_LEFT + 4, 72, "LITERALLY");
  display.setTextSize(1);
  for (uint8_t i = 0; i < historyCount; ++i) {
    display.setCursor(8, FIRST_LINE_Y + i * LINE_HEIGHT);
    display.setTextColor(history[i][0] == '>' ? COLOR_ACCENT : COLOR_TEXT);
    display.print(history[i]);
  }
  drawPrompt();
}

void drawSystemInfo() {
  display.fillScreen(COLOR_BG);
  drawTopBar("S.Y.S.T.E.M.", "");
  drawRoundedPanel(12, 66, 296, 43, COLOR_SURFACE, COLOR_GREEN);
  text(COLOR_GREEN, 1, 25, 78, "SYSTEM READY");
  text(COLOR_TEXT, 1, 25, 94, "Pico 2 W / display + keyboard online");

  
  
  display.drawFastHLine(SAFE_LEFT, 118, SAFE_WIDTH, COLOR_ACCENT_DARK);
  text(COLOR_MUTED, 1, 20, 124, "DELACRUZ SYSTEMS -- OPERATIONAL SPECIFICATIONS");
  display.drawFastHLine(SAFE_LEFT, 136, SAFE_WIDTH, COLOR_ACCENT_DARK);

  text(COLOR_TEXT, 1, 20, 144, "CORE HARDWARE:  E.R.I.C.");
  text(COLOR_MUTED, 1, 20, 155, "[ Embedded RP2350 Interactive Computer ]");
  text(COLOR_TEXT, 1, 20, 166, "OPERATING SYS:  S.Y.S.T.E.M. v1.0.0");
  text(COLOR_MUTED, 1, 20, 177, "[ Super Young System That Electronically Manages ]");
  text(COLOR_TEXT, 1, 20, 188, "COMMAND SHELL:  S.H.E.L.L.");
  text(COLOR_MUTED, 1, 20, 199, "[ Somehow Handling Every Line Literally ]");
  text(COLOR_TEXT, 1, 20, 210, "FILE EXPLORER:  F.I.L.E.S.");
  text(COLOR_MUTED, 1, 20, 221, "[ Folder Inspection & Location");
  text(COLOR_MUTED, 1, 20, 232, "  Execution Scanner ]");
  text(COLOR_TEXT, 1, 20, 243, "AUDIO ENGINE:   S.O.U.N.D.");
  text(COLOR_MUTED, 1, 20, 254, "[ System Operated Universal Noise Dispenser ]");
  text(COLOR_TEXT, 1, 20, 265, "SCRATCHPAD:     M.E.M.O.");
  text(COLOR_MUTED, 1, 20, 276, "[ Mistakes Eventually Manifest Otherwise ]");
  display.drawFastHLine(SAFE_LEFT, 284, SAFE_WIDTH, COLOR_ACCENT_DARK);
  drawFooter("BACK RETURN   ESC MENU");
}

void drawScratchpadBody() {
  display.fillRect(12, 68, 296, 210, COLOR_BLACK);
  display.drawRect(12, 68, 296, 210, COLOR_ACCENT_DARK);
  constexpr uint8_t COLUMNS = 46;
  constexpr uint8_t ROWS = 15;
  for (uint16_t i = 0; i < noteLength && i < COLUMNS * ROWS; ++i) {
    const uint8_t row = i / COLUMNS;
    const uint8_t col = i % COLUMNS;
    const char value = noteBuffer[i] == '\n' ? ' ' : noteBuffer[i];
    char glyph[2] = {value, '\0'};
    text(COLOR_TEXT, 1, 18 + col * 6, 78 + row * 13, glyph);
  }
  const uint16_t cursor = noteLength < COLUMNS * ROWS ? noteLength : COLUMNS * ROWS - 1;
  const uint8_t cursorRow = cursor / COLUMNS;
  const uint8_t cursorCol = cursor % COLUMNS;
  display.fillRect(18 + cursorCol * 6, 78 + cursorRow * 13, 4, 9, COLOR_ACCENT);
}

void drawScratchpad() {
  display.fillScreen(COLOR_BG);
  
  drawTopBar("M.E.M.O.", "");
  drawScratchpadBody();
  char shortPath[35];
  clippedText(shortPath, sizeof(shortPath), notePath, 31);
  text(COLOR_MUTED, 1, 14, 266, sdReady ? shortPath : "No SD card: session only.");
  drawFooter(sdReady ? "TYPE WRITE  CTRL+S SAVE  ESC MENU" : "TYPE WRITE  INSERT SD TO SAVE  ESC MENU");
}

void beginNewScratchpad() {
  noteBuffer[0] = '\0';
  noteLength = 0;
  noteDirty = false;
  
  if (sdReady) {
    uint16_t number = 1;
    do {
      snprintf(notePath, sizeof(notePath), "/ERIC/NOTE%03u.TXT", number++);
    } while (number <= 999 && SD.exists(notePath));
  } else {
    strncpy(notePath, "/ERIC/UNTITLED.TXT", sizeof(notePath) - 1);
    notePath[sizeof(notePath) - 1] = '\0';
  }
  scratchpadModal = ScratchpadModal::None;
  pendingNewScratchpad = false;
  switchApp(AppId::Scratchpad);
}

void requestNewScratchpad() {
  if (activeApp == AppId::Scratchpad && noteDirty) {
    pendingNewScratchpad = true;
    scratchpadModal = ScratchpadModal::ExitConfirm;
    drawScratchpadExitConfirm();
  } else {
    beginNewScratchpad();
  }
}

void requestScratchpadExit(AppId destination) {
  if (activeApp == AppId::Scratchpad && noteDirty) {
    pendingScratchDestination = destination;
    pendingNewScratchpad = false;
    scratchpadModal = ScratchpadModal::ExitConfirm;
    drawScratchpadExitConfirm();
  } else {
    switchApp(destination);
  }
}

void refreshExplorer() {
  explorerCount = 0;
  explorerSelection = 0;
  explorerMore = false;
  explorerTotal = 0;
  if (!sdReady) {
    strncpy(explorerStatus, "NO SD CARD", sizeof(explorerStatus));
    return;
  }
  File directory = SD.open(explorerPath);
  if (!directory || !directory.isDirectory()) {
    strncpy(explorerStatus, "PATH UNAVAILABLE", sizeof(explorerStatus));
    if (directory) directory.close();
    return;
  }
  strncpy(explorerStatus, "SD CARD", sizeof(explorerStatus));
  while (true) {
    File entry = directory.openNextFile();
    if (!entry) break;
    
    if (explorerTotal >= explorerOffset && explorerCount < EXPLORER_ITEMS) {
      const char *entryName = entry.name();
      const char *baseName = strrchr(entryName, '/');
      baseName = baseName ? baseName + 1 : entryName;
      strncpy(explorerNames[explorerCount], baseName, EXPLORER_NAME_LEN);
      explorerNames[explorerCount][EXPLORER_NAME_LEN] = '\0';
      
      
      
      if (entryName[0] == '/') {
        strncpy(explorerPaths[explorerCount], entryName, sizeof(explorerPaths[0]) - 1);
        explorerPaths[explorerCount][sizeof(explorerPaths[0]) - 1] = '\0';
      } else if (strcmp(explorerPath, "/") == 0) {
        snprintf(explorerPaths[explorerCount], sizeof(explorerPaths[0]), "/%s", entryName);
      } else {
        snprintf(explorerPaths[explorerCount], sizeof(explorerPaths[0]), "%s/%s", explorerPath, entryName);
      }
      explorerDirs[explorerCount] = entry.isDirectory();
      explorerCount++;
    }
    ++explorerTotal;
    entry.close();
  }
  directory.close();
  
  if (explorerCount == 0 && explorerOffset > 0 && explorerTotal > 0) {
    explorerOffset = ((explorerTotal - 1) / EXPLORER_ITEMS) * EXPLORER_ITEMS;
    refreshExplorer();
    return;
  }
  explorerMore = explorerOffset + explorerCount < explorerTotal;
}

void drawExplorerSelection() {
  display.fillRect(12, 68, 138, 210, COLOR_BLACK);
  display.drawRect(12, 68, 138, 210, COLOR_ACCENT_DARK);
  char clippedPath[24];
  clippedText(clippedPath, sizeof(clippedPath), explorerPath, 20);
  text(COLOR_ACCENT, 1, 20, 76, clippedPath);
  if (explorerCount == 0) {
    text(COLOR_MUTED, 1, 20, 104, sdReady ? "Folder is empty." : "Insert FAT32 SD card.");
    return;
  }
  for (uint8_t i = 0; i < explorerCount; ++i) {
    const int16_t y = 94 + i * 21;
    const bool selected = i == explorerSelection;
    if (selected) display.fillRoundRect(17, y - 2, 128, 18, 4, COLOR_SURFACE_2);
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 22, y + 2, explorerDirs[i] ? "[D]" : "[F]");
    
    char clippedName[18];
    clippedText(clippedName, sizeof(clippedName), explorerNames[i], 16);
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 46, y + 2, clippedName);
  }
  if (explorerTotal > EXPLORER_ITEMS) {
    char pageLabel[22];
    snprintf(pageLabel, sizeof(pageLabel), "%u-%u / %u", explorerOffset + 1, explorerOffset + explorerCount, explorerTotal);
    text(COLOR_MUTED, 1, 20, 258, pageLabel);
  }
  if (explorerMore) text(COLOR_ACCENT, 1, 20, 270, "DOWN: next page");
  else if (explorerOffset > 0) text(COLOR_ACCENT, 1, 20, 270, "UP: previous page");
}

bool selectedExplorerPath(char *out, size_t outSize) {
  if (explorerCount == 0 || explorerSelection >= explorerCount) return false;
  const size_t pathLength = strlen(explorerPaths[explorerSelection]);
  if (pathLength == 0 || pathLength >= outSize) return false;
  memcpy(out, explorerPaths[explorerSelection], pathLength + 1);
  return true;
}



bool selectedExplorerName(char *out, size_t outSize) {
  char fullPath[sizeof(explorerPaths[0])];
  if (!selectedExplorerPath(fullPath, sizeof(fullPath))) return false;
  const char *name = strrchr(fullPath, '/');
  name = name ? name + 1 : fullPath;
  const size_t nameLength = strlen(name);
  if (nameLength == 0 || nameLength >= outSize) return false;
  memcpy(out, name, nameLength + 1);
  return true;
}

bool isTextFile(const char *name) {
  return EricFileTypes::isEditableText(name);
}

bool isBmpFile(const char *name) {
  return EricFileTypes::isBmp(name);
}

bool isJpegFile(const char *name) {
  return EricFileTypes::isJpeg(name);
}

const char *fileOpenHint(const char *name) {
  return EricFileTypes::openHint(name);
}



bool removeExplorerTree(const char *path) {
  File item = SD.open(path);
  if (!item) return false;
  const bool isFolder = item.isDirectory();
  item.close();
  if (!isFolder) return SD.remove(path);

  File folder = SD.open(path);
  if (!folder) return false;
  bool ok = true;
  while (ok) {
    File child = folder.openNextFile();
    if (!child) break;
    const char *fullName = child.name();
    char childPath[EXPLORER_PATH_CAP];
    strncpy(childPath, fullName, sizeof(childPath) - 1);
    childPath[sizeof(childPath) - 1] = '\0';
    child.close();
    
    if (!removeExplorerTree(childPath)) ok = false;
  }
  folder.close();
  return ok && SD.rmdir(path);
}

void drawExplorerActionMenu() {
  drawRoundedPanel(32, 80, 256, 162, COLOR_SURFACE, COLOR_ACCENT);
  ditherRect(38, 86, 244, 150, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 48, 96, "FILE ACTIONS");
  const char *items[] = {"Open", "Edit text", "Rename", "Delete", "New file", "New folder", "Cancel"};
  for (uint8_t i = 0; i < 7; ++i) {
    const bool selected = i == static_cast<uint8_t>(explorerAction);
    const int16_t y = 116 + i * 16;
    if (selected) display.fillRoundRect(46, y - 1, 226, 14, 3, COLOR_SURFACE_2);
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 54, y, selected ? ">" : " ");
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 70, y, items[i]);
  }
  text(COLOR_MUTED, 1, 48, 220, "UP/DOWN  ENTER  BACK CLOSE");
}

void drawExplorerNameEntry() {
  display.fillScreen(COLOR_BG);
  drawTopBar(explorerAction == ExplorerAction::Rename ? "Rename" : (explorerAction == ExplorerAction::NewFolder ? "New Folder" : "New File"), "SD card files");
  drawRoundedPanel(18, 92, 284, 88, COLOR_SURFACE, COLOR_ACCENT);
  text(COLOR_MUTED, 1, 30, 108, explorerAction == ExplorerAction::Rename ? "TYPE A NEW NAME" : "TYPE A NAME");
  drawExplorerNameInput();
  text(COLOR_MUTED, 1, 30, 180, "LEFT/UP MOVE  RIGHT/DOWN MOVE");
  drawFooter("ENTER OR CTRL+S SAVE  BACK CANCEL  DEL ERASE");
}



void drawExplorerNameInput() {
  display.fillRect(30, 130, 260, 24, COLOR_BLACK);
  
  
  constexpr uint8_t VISIBLE_CHARS = 42;
  uint8_t firstChar = 0;
  if (explorerCursor >= VISIBLE_CHARS) firstChar = explorerCursor - (VISIBLE_CHARS - 1);
  char visibleName[VISIBLE_CHARS + 1];
  strncpy(visibleName, explorerInput + firstChar, VISIBLE_CHARS);
  visibleName[VISIBLE_CHARS] = '\0';
  text(COLOR_TEXT, 1, 34, 138, visibleName);
  const int16_t cursorX = 34 + (explorerCursor - firstChar) * 6;
  display.fillRect(cursorX, 137, 4, 9, COLOR_ACCENT);
  display.fillRect(30, 162, 260, 12, COLOR_SURFACE);
  char position[28];
  snprintf(position, sizeof(position), "CURSOR %u / %u", explorerCursor + 1, explorerInputLength + 1);
  text(COLOR_MUTED, 1, 30, 166, explorerNameError[0] ? explorerNameError : position);
}

void drawExplorerDeleteConfirm() {
  
  
  drawRoundedPanel(32, 92, 256, 136, COLOR_SURFACE, COLOR_ACCENT);
  const bool isFolder = explorerCount > 0 && explorerDirs[explorerSelection];
  text(COLOR_ACCENT, 1, 48, 108, isFolder ? "DELETE FOLDER?" : "DELETE FILE?");
  if (explorerCount > 0) text(COLOR_TEXT, 1, 48, 132, explorerNames[explorerSelection]);
  if (isFolder) {
    text(COLOR_MUTED, 1, 48, 156, "All contents will be deleted.");
    text(COLOR_MUTED, 1, 48, 174, "This cannot be undone.");
  } else {
    text(COLOR_MUTED, 1, 48, 156, "This cannot be undone.");
  }
  text(COLOR_MUTED, 1, 48, 202, "ENTER DELETE  BACK CANCEL");
}

void drawExplorerResult() {
  drawRoundedPanel(32, 108, 256, 104, COLOR_SURFACE, COLOR_ACCENT);
  ditherRect(38, 114, 244, 92, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 48, 126, "EXPLORER");
  text(COLOR_TEXT, 1, 48, 152, explorerResult);
  text(COLOR_MUTED, 1, 48, 184, "ENTER OR BACK TO CONTINUE");
}

void showExplorerResult(const char *message) {
  strncpy(explorerResult, message, sizeof(explorerResult) - 1);
  explorerResult[sizeof(explorerResult) - 1] = '\0';
  explorerMode = ExplorerMode::Result;
  drawExplorerResult();
}

void beginExplorerNameEntry(ExplorerAction action) {
  explorerAction = action;
  explorerInputLength = 0;
  explorerCursor = 0;
  explorerInput[0] = '\0';
  explorerNameError[0] = '\0';
  if (action == ExplorerAction::Rename && explorerCount > 0) {
    
    
    if (!selectedExplorerName(explorerInput, sizeof(explorerInput))) {
      strncpy(explorerNameError, "Could not read the full filename.", sizeof(explorerNameError) - 1);
    }
    explorerInputLength = strlen(explorerInput);
    explorerCursor = explorerInputLength;
  }
  explorerMode = ExplorerMode::NameEntry;
  drawExplorerNameEntry();
}

void commitExplorerNameEntry() {
  explorerNameError[0] = '\0';
  if (!sdReady) { strncpy(explorerNameError, "SD card is unavailable.", sizeof(explorerNameError) - 1); drawExplorerNameInput(); return; }
  if (explorerInputLength == 0) { strncpy(explorerNameError, "Name cannot be empty.", sizeof(explorerNameError) - 1); drawExplorerNameInput(); return; }
  for (uint8_t i = 0; i < explorerInputLength; ++i) {
    const char c = explorerInput[i];
    if (!(isalnum(c) || c == '.' || c == '_' || c == '-')) {
      strncpy(explorerNameError, "Use letters, numbers, . _ - only.", sizeof(explorerNameError) - 1);
      drawExplorerNameInput();
      return;
    }
  }
  char destination[160];
  const int written = strcmp(explorerPath, "/") == 0
    ? snprintf(destination, sizeof(destination), "/%s", explorerInput)
    : snprintf(destination, sizeof(destination), "%s/%s", explorerPath, explorerInput);
  if (written <= 0 || static_cast<size_t>(written) >= sizeof(destination)) {
    strncpy(explorerNameError, "That path is too long.", sizeof(explorerNameError) - 1);
    drawExplorerNameInput();
    return;
  }
  bool completed = false;
  if (SD.exists(destination)) {
    strncpy(explorerNameError, "That name already exists.", sizeof(explorerNameError) - 1);
    drawExplorerNameInput();
    return;
  }
  if (explorerAction == ExplorerAction::NewFile) {
    File file = SD.open(destination, FILE_WRITE);
    completed = static_cast<bool>(file);
    if (file) file.close();
  } else if (explorerAction == ExplorerAction::NewFolder) {
    completed = SD.mkdir(destination);
  } else if (explorerAction == ExplorerAction::Rename) {
    char source[sizeof(explorerPaths[0])];
    if (selectedExplorerPath(source, sizeof(source))) completed = SD.rename(source, destination);
  }
  if (completed) showExplorerResult(explorerAction == ExplorerAction::Rename ? "Rename complete." : "Item created.");
  else {
    strncpy(explorerNameError, "Save failed. Try another name.", sizeof(explorerNameError) - 1);
    drawExplorerNameInput();
  }
}

void drawExplorerPreview() {
  display.fillRect(158, 68, 150, 210, COLOR_BLACK);
  display.drawRect(158, 68, 150, 210, COLOR_ACCENT_DARK);
  
  
  text(COLOR_ACCENT, 1, 168, 76, "FILE INSPECTION &");
  text(COLOR_ACCENT, 1, 168, 88, "LOCATION EXAMINATION");
  text(COLOR_ACCENT, 1, 168, 100, "SCANNER");
  if (!sdReady) {
    text(COLOR_TEXT, 1, 168, 124, "Files stay on");
    text(COLOR_TEXT, 1, 168, 140, "your SD card.");
    text(COLOR_MUTED, 1, 168, 172, "No flash writes.");
    return;
  }
  if (explorerCount == 0) return;
  
  char clippedName[24];
  clippedText(clippedName, sizeof(clippedName), explorerNames[explorerSelection], 22);
  text(COLOR_TEXT, 1, 168, 124, clippedName);
  if (explorerDirs[explorerSelection]) {
    text(COLOR_MUTED, 1, 168, 148, "Folder");
    text(COLOR_MUTED, 1, 168, 166, "Enter to open");
  } else {
    text(COLOR_MUTED, 1, 168, 148, fileOpenHint(explorerNames[explorerSelection]));
    text(COLOR_MUTED, 1, 168, 166, isTextFile(explorerNames[explorerSelection]) ? "Enter to edit" : ((isBmpFile(explorerNames[explorerSelection]) || isJpegFile(explorerNames[explorerSelection])) ? "Enter to view" : "Del for actions"));
  }
}

void refreshSoundFolders() {
  soundFolderCount = 0;
  if (!sdReady) return;
  File directory = SD.open(soundFolder);
  if (!directory || !directory.isDirectory()) {
    if (directory) directory.close();
    return;
  }
  
  
  while (soundFolderCount < SOUND_FOLDER_ITEMS) {
    File entry = directory.openNextFile();
    if (!entry) break;
    const bool isFolder = entry.isDirectory();
    if (isFolder || EricFileTypes::isAudio(entry.name())) {
      const char *base = strrchr(entry.name(), '/');
      base = base ? base + 1 : entry.name();
      strncpy(soundFolders[soundFolderCount], base, EXPLORER_NAME_LEN);
      soundFolders[soundFolderCount][EXPLORER_NAME_LEN] = '\0';
      
      
      if (entry.name()[0] == '/') {
        strncpy(soundFolderPaths[soundFolderCount], entry.name(), SOUND_PATH_LEN - 1);
        soundFolderPaths[soundFolderCount][SOUND_PATH_LEN - 1] = '\0';
      } else if (strcmp(soundFolder, "/") == 0) {
        snprintf(soundFolderPaths[soundFolderCount], SOUND_PATH_LEN, "/%s", entry.name());
      } else {
        snprintf(soundFolderPaths[soundFolderCount], SOUND_PATH_LEN, "%s/%s", soundFolder, entry.name());
      }
      soundFolderIsDirectory[soundFolderCount] = isFolder;
      ++soundFolderCount;
    }
    entry.close();
  }
  directory.close();
  if (soundFolderCount == 0) soundFolderSelection = 0;
  else if (soundFolderSelection >= soundFolderCount) soundFolderSelection = soundFolderCount - 1;
}

void scanSoundFolder() {
  soundTrackCount = 0;
  soundTrackSelection = 0;
  soundPlaylistTruncated = false;
  if (!sdReady) return;
  File directory = SD.open(soundFolder);
  if (!directory || !directory.isDirectory()) {
    if (directory) directory.close();
    return;
  }
  while (true) {
    File entry = directory.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory() && EricFileTypes::isAudio(entry.name())) {
      if (soundTrackCount < SOUND_PLAYLIST_ITEMS) {
        
        
        
        
        
        
        
        char *slot = soundTracks[soundTrackCount];
        if (entry.name()[0] == '/') {
          strncpy(slot, entry.name(), SOUND_PATH_LEN - 1);
          slot[SOUND_PATH_LEN - 1] = '\0';
        } else if (strcmp(soundFolder, "/") == 0) {
          snprintf(slot, SOUND_PATH_LEN, "/%s", entry.name());
        } else {
          snprintf(slot, SOUND_PATH_LEN, "%s/%s", soundFolder, entry.name());
        }
        ++soundTrackCount;
      } else {
        soundPlaylistTruncated = true;
      }
    }
    entry.close();
  }
  directory.close();
  soundMode = SoundMode::Playlist;
}

void drawSoundFolderSelect() {
  display.fillScreen(COLOR_BG);
  drawTopBar("S.O.U.N.D.", "");
  drawRoundedPanel(12, 66, 296, 48, COLOR_SURFACE, COLOR_ACCENT_DARK);
  ditherRect(18, 72, 284, 36, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 22, 76, "SYSTEM OPERATED UNIVERSAL NOISE DISPENSER");
  char folderLabel[32];
  clippedText(folderLabel, sizeof(folderLabel), soundFolder, 28);
  text(COLOR_TEXT, 1, 22, 94, folderLabel);
  display.drawRect(12, 124, 296, 150, COLOR_ACCENT_DARK);
  if (!sdReady) {
    text(COLOR_TEXT, 1, 26, 154, "Insert a FAT32 SD card.");
  } else if (soundFolderCount == 0) {
    text(COLOR_MUTED, 1, 26, 154, "No folders or MP3 files here.");
  }
  for (uint8_t i = 0; i < soundFolderCount; ++i) drawSoundFolderRow(i);
  drawFooter("UP/DOWN BROWSE  ENTER OPEN/PLAY  LEFT UP");
}



void drawSoundFolderRow(uint8_t index) {
  if (index >= soundFolderCount) return;
  const int16_t y = 136 + index * 16;
  const bool selected = soundFolderSelection == index;
  display.fillRect(18, y - 1, 284, 14, COLOR_BLACK);
  if (selected) display.fillRoundRect(18, y - 1, 284, 14, 3, COLOR_SURFACE_2);
  char name[28];
  clippedText(name, sizeof(name), soundFolders[index], 25);
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 26, y, selected ? ">" : " ");
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 40, y, soundFolderIsDirectory[index] ? "[D]" : "[A]");
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 64, y, name);
}

void drawSoundFolderSelection(uint8_t previous) {
  drawSoundFolderRow(previous);
  drawSoundFolderRow(soundFolderSelection);
}

void drawSoundPlaylist() {
  display.fillScreen(COLOR_BG);
  drawTopBar("S.O.U.N.D.", "");
  drawRoundedPanel(12, 66, 296, 48, COLOR_SURFACE, COLOR_ACCENT_DARK);
  ditherRect(18, 72, 284, 36, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 22, 76, "PLAYLIST // DIRECT FOLDER SCAN");
  char folderLabel[32];
  clippedText(folderLabel, sizeof(folderLabel), soundFolder, 28);
  text(COLOR_MUTED, 1, 22, 94, folderLabel);
  display.drawRect(12, 124, 296, 150, COLOR_ACCENT_DARK);
  if (soundTrackCount == 0) {
    text(COLOR_TEXT, 1, 26, 154, "No MP3 files found.");
    text(COLOR_MUTED, 1, 26, 174, "BACK chooses another folder.");
  }
  for (uint8_t i = 0; i < soundTrackCount; ++i) {
    const int16_t y = 136 + i * 11;
    const bool selected = i == soundTrackSelection;
    if (selected) display.fillRoundRect(18, y - 1, 284, 11, 3, COLOR_SURFACE_2);
    const char *base = strrchr(soundTracks[i], '/');
    char name[30];
    clippedText(name, sizeof(name), base ? base + 1 : soundTracks[i], 26);
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 26, y, selected ? ">" : " ");
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 40, y, name);
  }
  if (soundPlaylistTruncated) text(COLOR_ACCENT, 1, 22, 262, "First 12 tracks shown; refine folder.");
  drawFooter("UP/DOWN CHOOSE  ENTER PLAY  BACK FOLDERS");
}

void drawAudioPlayer() {
  if (soundMode == SoundMode::FolderSelect) { drawSoundFolderSelect(); return; }
  if (soundMode == SoundMode::Playlist) { drawSoundPlaylist(); return; }
  display.fillScreen(COLOR_BG);
  drawTopBar("S.O.U.N.D.", "");
  drawRoundedPanel(18, 82, 284, 120, COLOR_SURFACE, COLOR_ACCENT);
  ditherRect(24, 88, 272, 108, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 34, 96, "SYSTEM OPERATED UNIVERSAL");
  text(COLOR_ACCENT, 1, 34, 108, "NOISE DISPENSER");
  char name[28];
  const char *track = EricAudio::path();
  const char *base = strrchr(track, '/');
  clippedText(name, sizeof(name), base ? base + 1 : track, 25);
  text(COLOR_TEXT, 1, 34, 134, name);
  text(COLOR_TEXT, 1, 34, 158, EricAudio::isPlaying() ? "PLAYING" : "STOPPED");
  text(COLOR_MUTED, 1, 34, 180, "ENTER STOP/PLAY  BACK RETURNS TO BROWSER");
  drawFooter("F AUDIO BROWSER  BACK STOP + BROWSER  ESC MENU");
}

void openSoundWorkspace() {
  soundFolder[0] = '/'; soundFolder[1] = '\0';
  soundFolderSelection = 0;
  soundMode = SoundMode::FolderSelect;
  soundReturnToExplorer = false;
  refreshSoundFolders();
  switchApp(AppId::AudioPlayer);
}

bool startSoundTrack(uint8_t index) {
  if (index >= soundTrackCount) return false;
  soundTrackSelection = index;
  soundReturnToExplorer = false;
  if (!EricAudio::play(soundTracks[index])) return false;
  soundMode = SoundMode::NowPlaying;
  drawAudioPlayer();
  return true;
}

void handleAudioPlayerKey(uint8_t key) {
  if (soundMode == SoundMode::NowPlaying) {
    if (key == KEY_LEFT || key == KEY_JOY_LEFT) {
      if (soundTrackCount) {
        const uint8_t previous = soundTrackSelection == 0 ? soundTrackCount - 1 : soundTrackSelection - 1;
        startSoundTrack(previous);
      }
    } else if (key == KEY_RIGHT || key == KEY_JOY_RIGHT) {
      if (soundTrackCount) startSoundTrack((soundTrackSelection + 1) % soundTrackCount);
    } else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER || key == KEY_DELETE) {
      
      if (EricAudio::isPlaying()) EricAudio::stop();
      else startSoundTrack(soundTrackSelection);
      drawAudioPlayer();
    } else if (key == KEY_BACK) {
      EricAudio::stop();
      if (soundReturnToExplorer) switchApp(AppId::Explorer);
      else { soundMode = SoundMode::FolderSelect; refreshSoundFolders(); drawSoundFolderSelect(); }
    }
    return;
  }
  if (soundMode == SoundMode::Playlist) {
    if (key == KEY_UP || key == KEY_JOY_UP) {
      if (soundTrackCount && soundTrackSelection > 0) --soundTrackSelection;
      drawSoundPlaylist();
    } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
      if (soundTrackCount && soundTrackSelection + 1 < soundTrackCount) ++soundTrackSelection;
      drawSoundPlaylist();
    } else if ((key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) && soundTrackCount) {
      
      if (!startSoundTrack(soundTrackSelection)) {
        
        
        
        
        drawSoundPlaylist();
        char reason[38];
        clippedText(reason, sizeof(reason), EricAudio::message(), 36);
        text(COLOR_ACCENT, 1, 26, 250, reason);
      }
    } else if (key == KEY_BACK) {
      soundMode = SoundMode::FolderSelect; refreshSoundFolders(); drawSoundFolderSelect();
    }
    return;
  }
  if (key == KEY_UP || key == KEY_JOY_UP) {
    const uint8_t previous = soundFolderSelection;
    if (soundFolderCount && soundFolderSelection > 0) --soundFolderSelection;
    drawSoundFolderSelection(previous);
  } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
    const uint8_t previous = soundFolderSelection;
    if (soundFolderCount && soundFolderSelection + 1 < soundFolderCount) ++soundFolderSelection;
    drawSoundFolderSelection(previous);
  } else if (key == KEY_LEFT || key == KEY_JOY_LEFT) {
    
    if (strcmp(soundFolder, "/") != 0) {
      char *slash = strrchr(soundFolder, '/');
      if (slash == soundFolder) soundFolder[1] = '\0'; else *slash = '\0';
      soundFolderSelection = 0; refreshSoundFolders(); drawSoundFolderSelect();
    }
  } else if ((key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) && soundFolderCount) {
    const char *selectedPath = soundFolderPaths[soundFolderSelection];
    if (soundFolderIsDirectory[soundFolderSelection]) {
      
      if (selectedPath[0] != '\0' && strlen(selectedPath) < sizeof(soundFolder)) {
        strncpy(soundFolder, selectedPath, sizeof(soundFolder) - 1);
        soundFolder[sizeof(soundFolder) - 1] = '\0';
        soundFolderSelection = 0;
        refreshSoundFolders();
        drawSoundFolderSelect();
      }
    } else if (!EricAudio::play(selectedPath)) {
      drawSoundFolderSelect();
      
      
      
      char reason[38];
      clippedText(reason, sizeof(reason), EricAudio::message(), 36);
      text(COLOR_ACCENT, 1, 26, 250, reason);
    } else {
      
      strncpy(soundTracks[0], selectedPath, sizeof(soundTracks[0]) - 1);
      soundTracks[0][sizeof(soundTracks[0]) - 1] = '\0';
      soundTrackCount = 1;
      soundTrackSelection = 0;
      soundReturnToExplorer = false;
      soundMode = SoundMode::NowPlaying;
      drawAudioPlayer();
    }
  } else if (key == KEY_BACK) {
    goBack();
  }
}

void openBmpViewer(const char *path) {
  const EricBmp::Result result = EricBmp::open(path);
  if (result != EricBmp::Result::Ok) {
    showExplorerResult(EricBmp::message(result));
    return;
  }
  switchApp(AppId::ImageViewer);
}

void drawBmpViewer() {
  constexpr int16_t AREA_X = 12, AREA_Y = 62, AREA_W = 296, AREA_H = 216;
  display.fillScreen(COLOR_BG);
  drawTopBar("I.M.A.G.E.", "BMP VIEWER");
  display.fillRect(AREA_X, AREA_Y, AREA_W, AREA_H, COLOR_BLACK);
  display.drawRect(AREA_X, AREA_Y, AREA_W, AREA_H, COLOR_ACCENT_DARK);
  if (!EricBmp::isOpen()) {
    text(COLOR_ACCENT, 1, 24, 88, "IMAGE IS NO LONGER AVAILABLE");
    drawFooter("BACK RETURN TO F.I.L.E.S.");
    return;
  }
  const uint16_t sourceW = EricBmp::width();
  const uint16_t sourceH = EricBmp::height();
  uint16_t drawW = AREA_W;
  uint16_t drawH = static_cast<uint16_t>((static_cast<uint32_t>(sourceH) * drawW) / sourceW);
  if (drawH > AREA_H) {
    drawH = AREA_H;
    drawW = static_cast<uint16_t>((static_cast<uint32_t>(sourceW) * drawH) / sourceH);
  }
  const int16_t originX = AREA_X + (AREA_W - drawW) / 2;
  const int16_t originY = AREA_Y + (AREA_H - drawH) / 2;
  uint16_t row[320];
  bool readOk = true;
  
  
  
  
  
  for (uint16_t destinationY = 0; destinationY < drawH && readOk; ++destinationY) {
    const uint16_t sourceY = static_cast<uint16_t>((static_cast<uint32_t>(destinationY) * sourceH) / drawH);
    
    
    if (!EricBmp::readScaledRow(sourceY, row, drawW)) { readOk = false; break; }
    for (uint16_t destinationX = 0; destinationX < drawW; ++destinationX) {
      display.drawPixel(originX + destinationX, originY + destinationY, row[destinationX]);
    }
  }
  if (!readOk) text(COLOR_ACCENT, 1, 24, 264, "IMAGE READ FAILED");
  else {
    char dimensions[38];
    snprintf(dimensions, sizeof(dimensions), "%u x %u // %s", sourceW, sourceH, EricBmp::formatLabel());
    text(COLOR_MUTED, 1, 20, 264, dimensions);
  }
  drawFooter("ENTER REDRAW  BACK RETURN TO F.I.L.E.S.");
}

void handleBmpViewerKey(uint8_t key) {
  if (key == KEY_BACK) {
    EricBmp::close();
    EricJpeg::close();
    switchApp(AppId::Explorer);
  } else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
    if (EricBmp::isOpen()) drawBmpViewer();
    else drawJpegViewer();
  }
}

void openJpegViewer(const char *path) {
  const EricJpeg::Result result = EricJpeg::open(path);
  if (result != EricJpeg::Result::Ok) {
    showExplorerResult(EricJpeg::message(result));
    return;
  }
  switchApp(AppId::ImageViewer);
}





uint16_t jpegDecodedW = 0;
uint16_t jpegDecodedH = 0;
uint16_t jpegDisplayW = 0;
uint16_t jpegDisplayH = 0;
int16_t jpegDisplayX = 0;
int16_t jpegDisplayY = 0;














void fitSpan(uint32_t index, uint16_t sourceSize, uint16_t destinationSize,
             uint16_t &start, uint16_t &end) {
  start = static_cast<uint16_t>((index * destinationSize) / sourceSize);
  end = static_cast<uint16_t>(((index + 1U) * destinationSize) / sourceSize);
  
  
  if (index + 1U >= sourceSize) end = destinationSize;
  if (end > destinationSize) end = destinationSize;
  
  
  
}

bool drawJpegBlock(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t *pixels) {
  if (pixels == nullptr || jpegDecodedW == 0 || jpegDecodedH == 0) return false;
  for (uint16_t row = 0; row < height; ++row) {
    const uint32_t sourceY = static_cast<uint32_t>(y) + row;
    if (sourceY >= jpegDecodedH) continue;
    uint16_t destinationY = 0, destinationYEnd = 0;
    fitSpan(sourceY, jpegDecodedH, jpegDisplayH, destinationY, destinationYEnd);
    if (destinationY >= jpegDisplayH) continue;
    for (uint16_t column = 0; column < width; ++column) {
      const uint32_t sourceX = static_cast<uint32_t>(x) + column;
      if (sourceX >= jpegDecodedW) continue;
      uint16_t destinationX = 0, destinationXEnd = 0;
      fitSpan(sourceX, jpegDecodedW, jpegDisplayW, destinationX, destinationXEnd);
      if (destinationX >= jpegDisplayW) continue;
      const uint16_t colour = pixels[static_cast<uint32_t>(row) * width + column];
      for (uint16_t dy = destinationY; dy < destinationYEnd; ++dy) {
        
        
        display.drawFastHLine(jpegDisplayX + destinationX, jpegDisplayY + dy,
                              destinationXEnd - destinationX, colour);
      }
    }
  }
  return true;
}

void drawJpegViewer() {
  constexpr int16_t AREA_X = 12, AREA_Y = 62, AREA_W = 296, AREA_H = 216;
  display.fillScreen(COLOR_BG);
  drawTopBar("I.M.A.G.E.", "JPEG VIEWER");
  display.fillRect(AREA_X, AREA_Y, AREA_W, AREA_H, COLOR_BLACK);
  display.drawRect(AREA_X, AREA_Y, AREA_W, AREA_H, COLOR_ACCENT_DARK);
  if (!EricJpeg::isOpen()) {
    text(COLOR_ACCENT, 1, 24, 88, "IMAGE IS NO LONGER AVAILABLE");
    drawFooter("BACK RETURN TO F.I.L.E.S.");
    return;
  }
  const uint16_t sourceW = EricJpeg::width();
  const uint16_t sourceH = EricJpeg::height();
  
  
  
  
  
  
  
  
  uint8_t decoderScale = 1;
  for (uint8_t candidate = 2; candidate <= 8; candidate <<= 1) {
    const uint32_t decodedW = (sourceW + candidate - 1U) / candidate;
    const uint32_t decodedH = (sourceH + candidate - 1U) / candidate;
    if (decodedW >= static_cast<uint32_t>(AREA_W) && decodedH >= static_cast<uint32_t>(AREA_H)) {
      decoderScale = candidate;
    }
  }
  jpegDecodedW = static_cast<uint16_t>((sourceW + decoderScale - 1U) / decoderScale);
  jpegDecodedH = static_cast<uint16_t>((sourceH + decoderScale - 1U) / decoderScale);
  jpegDisplayW = AREA_W;
  jpegDisplayH = static_cast<uint16_t>((static_cast<uint32_t>(jpegDecodedH) * jpegDisplayW) / jpegDecodedW);
  if (jpegDisplayH > AREA_H) {
    jpegDisplayH = AREA_H;
    jpegDisplayW = static_cast<uint16_t>((static_cast<uint32_t>(jpegDecodedW) * jpegDisplayH) / jpegDecodedH);
  }
  jpegDisplayX = AREA_X + (AREA_W - jpegDisplayW) / 2;
  jpegDisplayY = AREA_Y + (AREA_H - jpegDisplayH) / 2;
  text(COLOR_MUTED, 1, 20, 264, "DECODING...");
  const bool decoded = EricJpeg::draw(0, 0, decoderScale, drawJpegBlock);
  if (!decoded) text(COLOR_ACCENT, 1, 20, 264, "JPEG DECODE FAILED");
  else {
    char dimensions[38];
    snprintf(dimensions, sizeof(dimensions), "%u x %u // JPEG 1/%u", sourceW, sourceH, decoderScale);
    display.fillRect(18, 260, 284, 14, COLOR_BLACK);
    text(COLOR_MUTED, 1, 20, 264, dimensions);
  }
  drawFooter("ENTER REDRAW  BACK RETURN TO F.I.L.E.S.");
}

void openAudioFile(const char *path) {
  if (!EricAudio::play(path)) {
    if (soundReturnToExplorer) {
      
      
      char reason[38];
      clippedText(reason, sizeof(reason), EricAudio::message(), 36);
      showExplorerResult(reason);
    }
    return;
  }
  soundMode = SoundMode::NowPlaying;
  
  switchApp(AppId::AudioPlayer);
}

void drawExplorer() {
  refreshExplorer();
  display.fillScreen(COLOR_BG);
  drawTopBar("F.I.L.E.S.", "");
  drawExplorerSelection();
  drawExplorerPreview();
  drawFooter("UP/DOWN BROWSE  ENTER OPEN  DEL ACTIONS");
}

void openExplorerSelection() {
  if (!sdReady || explorerCount == 0) return;
  if (explorerDirs[explorerSelection]) {
    char nextPath[sizeof(explorerPath)];
    
    
    
    int needed;
    if (strcmp(explorerPath, "/") == 0) needed = snprintf(nextPath, sizeof(nextPath), "/%s", explorerNames[explorerSelection]);
    else needed = snprintf(nextPath, sizeof(nextPath), "%s/%s", explorerPath, explorerNames[explorerSelection]);
    if (needed < 0 || static_cast<size_t>(needed) >= sizeof(explorerPath)) {
      showExplorerResult("That folder is too deep to open.");
      return;
    }
    explorerOffset = 0;
    strncpy(explorerPath, nextPath, sizeof(explorerPath) - 1);
    explorerPath[sizeof(explorerPath) - 1] = '\0';
    drawExplorer();
  } else if (isTextFile(explorerNames[explorerSelection])) {
    if (selectedExplorerPath(notePath, sizeof(notePath))) {
      loadNoteFromSD();
      switchApp(AppId::Scratchpad);
    }
  } else if (EricFileTypes::isAudio(explorerNames[explorerSelection])) {
    char audioPath[EXPLORER_PATH_CAP];
    
    
    soundReturnToExplorer = true;
    if (selectedExplorerPath(audioPath, sizeof(audioPath))) openAudioFile(audioPath);
  } else if (isBmpFile(explorerNames[explorerSelection])) {
    char imagePath[sizeof(explorerPaths[0])];
    if (selectedExplorerPath(imagePath, sizeof(imagePath))) openBmpViewer(imagePath);
  } else if (isJpegFile(explorerNames[explorerSelection])) {
    char imagePath[sizeof(explorerPaths[0])];
    if (selectedExplorerPath(imagePath, sizeof(imagePath))) openJpegViewer(imagePath);
  } else {
    showExplorerResult("No viewer installed for this type.");
  }
}

void handleExplorerKey(uint8_t key) {
  
  
  if (explorerMode == ExplorerMode::Result) {
    if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER || key == KEY_BACK) {
      explorerMode = ExplorerMode::Browse;
      drawExplorer();
    }
    return;
  }
  if (explorerMode == ExplorerMode::Actions) {
    if (key == KEY_UP || key == KEY_JOY_UP) {
      explorerAction = static_cast<ExplorerAction>((static_cast<uint8_t>(explorerAction) + 6) % 7);
      drawExplorerActionMenu();
    } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
      explorerAction = static_cast<ExplorerAction>((static_cast<uint8_t>(explorerAction) + 1) % 7);
      drawExplorerActionMenu();
    } else if (key == KEY_BACK) {
      explorerMode = ExplorerMode::Browse; drawExplorer();
    } else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      if (explorerAction == ExplorerAction::Open) {
        if (explorerCount == 0) showExplorerResult("Choose an item first.");
        else { explorerMode = ExplorerMode::Browse; openExplorerSelection(); }
      } else if (explorerAction == ExplorerAction::Edit) {
        if (explorerCount > 0 && !explorerDirs[explorerSelection] && isTextFile(explorerNames[explorerSelection])) {
          explorerMode = ExplorerMode::Browse; openExplorerSelection();
        } else showExplorerResult("Edit works with .TXT files.");
      } else if (explorerAction == ExplorerAction::Rename || explorerAction == ExplorerAction::NewFile || explorerAction == ExplorerAction::NewFolder) {
        if (explorerAction == ExplorerAction::Rename && explorerCount == 0) showExplorerResult("Choose an item first.");
        else beginExplorerNameEntry(explorerAction);
      } else if (explorerAction == ExplorerAction::Delete && explorerCount > 0) {
        explorerMode = ExplorerMode::DeleteConfirm; drawExplorerDeleteConfirm();
      } else if (explorerAction == ExplorerAction::Delete) showExplorerResult("Choose an item first.");
      else { explorerMode = ExplorerMode::Browse; drawExplorer(); }
    }
    return;
  }
  if (explorerMode == ExplorerMode::NameEntry) {
    if (key == KEY_BACK) {
      explorerMode = ExplorerMode::Browse;
      drawExplorer();
    } else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER || key == 0x13) {
      
      
      commitExplorerNameEntry();
    } else if (key == KEY_LEFT || key == KEY_JOY_LEFT || key == KEY_UP || key == KEY_JOY_UP) {
      
      
      if (explorerCursor > 0) --explorerCursor;
      drawExplorerNameInput();
    } else if (key == KEY_RIGHT || key == KEY_JOY_RIGHT || key == KEY_DOWN || key == KEY_JOY_DOWN) {
      
      if (explorerCursor < explorerInputLength) ++explorerCursor;
      drawExplorerNameInput();
    } else if (key == 0x7F || key == KEY_DELETE) {
      
      
      if (explorerCursor > 0) {
        memmove(&explorerInput[explorerCursor - 1], &explorerInput[explorerCursor], explorerInputLength - explorerCursor + 1);
        --explorerCursor;
        --explorerInputLength;
      }
      drawExplorerNameInput();
    } else if (key >= 32 && key <= 126 && explorerInputLength < EXPLORER_INPUT_LEN) {
      
      
      memmove(&explorerInput[explorerCursor + 1], &explorerInput[explorerCursor], explorerInputLength - explorerCursor + 1);
      explorerInput[explorerCursor++] = static_cast<char>(key);
      ++explorerInputLength;
      drawExplorerNameInput();
    }
    return;
  }
  if (explorerMode == ExplorerMode::DeleteConfirm) {
    if (key == KEY_BACK) { explorerMode = ExplorerMode::Browse; drawExplorer(); }
    else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      char target[EXPLORER_PATH_CAP];
      bool removed = false;
      if (selectedExplorerPath(target, sizeof(target))) {
        
        
        removed = explorerDirs[explorerSelection] ? removeExplorerTree(target) : SD.remove(target);
      }
      showExplorerResult(removed ? "Delete complete." : "Delete failed. Check SD card.");
    }
    return;
  }
  if (key == KEY_UP || key == KEY_JOY_UP) {
    if (explorerCount > 0 && explorerSelection > 0) {
      --explorerSelection;
    } else if (explorerOffset >= EXPLORER_ITEMS) {
      
      explorerOffset -= EXPLORER_ITEMS;
      drawExplorer();
      explorerSelection = explorerCount > 0 ? explorerCount - 1 : 0;
      drawExplorerSelection();
      drawExplorerPreview();
      return;
    }
  } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
    if (explorerCount > 0 && explorerSelection + 1 < explorerCount) {
      ++explorerSelection;
    } else if (explorerMore) {
      
      explorerOffset += EXPLORER_ITEMS;
      drawExplorer();
      return;
    }
  } else if (key == KEY_LEFT || key == KEY_JOY_LEFT) {
    if (strcmp(explorerPath, "/") != 0) {
      char *lastSlash = strrchr(explorerPath, '/');
      if (lastSlash == explorerPath) explorerPath[1] = '\0';
      else if (lastSlash) *lastSlash = '\0';
      explorerOffset = 0;
      drawExplorer();
      return;
    }
  } else if (key == KEY_BACK) {
    goBack();
    return;
  } else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
    openExplorerSelection();
    return;
  } else {
    return;
  }
  drawExplorerSelection();
  drawExplorerPreview();
}

bool saveNoteToSD() {
  return sdReady && EricStorage::saveText(notePath, noteBuffer, noteLength);
}

bool loadNoteFromSD() {
  size_t loadedLength = 0;
  if (!sdReady || !EricStorage::loadText(notePath, noteBuffer, sizeof(noteBuffer), loadedLength)) return false;
  noteLength = static_cast<uint16_t>(loadedLength);
  noteDirty = false;
  return true;
}

bool saveNote() {
  
  
  if (!sdReady) return false;
  const bool savedToCard = saveNoteToSD();
  if (savedToCard) noteDirty = false;
  return savedToCard;
}

void loadNote() {
  
  loadNoteFromSD();
}

void handleScratchpadKey(uint8_t key) {
  if (scratchpadModal == ScratchpadModal::ExitConfirm) {
    if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      if (!saveNote()) return;
      noteDirty = false;
      scratchpadModal = ScratchpadModal::None;
      if (pendingNewScratchpad) beginNewScratchpad();
      else switchApp(pendingScratchDestination);
    } else if (key == KEY_DELETE || key == 0x7F) {
      noteDirty = false;
      scratchpadModal = ScratchpadModal::None;
      if (pendingNewScratchpad) beginNewScratchpad();
      else switchApp(pendingScratchDestination);
    } else if (key == KEY_BACK || key == KEY_ESC || key == 0x1B) {
      scratchpadModal = ScratchpadModal::None;
      pendingNewScratchpad = false;
      drawScratchpad();
    }
    return;
  }
  if (key == 0x13) { saveNote(); drawScratchpad(); return; }
  
  if (key == 0x7F || key == KEY_DELETE) {
    if (noteLength > 0) noteBuffer[--noteLength] = '\0';
  } else if (key == 0x0A || key == 0x0D) {
    if (noteLength < NOTE_CAPACITY) {
      noteBuffer[noteLength++] = '\n';
      noteBuffer[noteLength] = '\0';
    }
  } else if (key >= 32 && key <= 126 && noteLength < NOTE_CAPACITY) {
    noteBuffer[noteLength++] = static_cast<char>(key);
    noteBuffer[noteLength] = '\0';
  } else {
    return;
  }
  noteDirty = true;
  drawScratchpadBody();
}

void addHistory(const char *line) {
  if (historyCount == HISTORY_LINES) {
    for (uint8_t i = 1; i < HISTORY_LINES; ++i) {
      strncpy(history[i - 1], history[i], sizeof(history[0]));
    }
    historyCount--;
  }
  strncpy(history[historyCount], line, sizeof(history[0]) - 1);
  history[historyCount][sizeof(history[0]) - 1] = '\0';
  historyCount++;
}

void runCommand() {
  char echo[MAX_COMMAND + 4];
  snprintf(echo, sizeof(echo), "> %s", commandLine);
  addHistory(echo);
  for (uint8_t i = 0; commandLine[i]; ++i) commandLine[i] = toupper(commandLine[i]);

  if (strcmp(commandLine, "HELP") == 0) {
    addHistory("HELP APPS HOME SYS NOTES CLEAR ECHO <text>");
  } else if (strcmp(commandLine, "APPS") == 0) {
    addHistory("Shell, System, and Scratchpad are live.");
  } else if (strcmp(commandLine, "HOME") == 0) {
    commandLine[0] = '\0'; commandLength = 0; switchApp(AppId::Home); return;
  } else if (strcmp(commandLine, "SYS") == 0 || strcmp(commandLine, "SYSTEM") == 0) {
    commandLine[0] = '\0'; commandLength = 0; switchApp(AppId::SystemInfo); return;
  } else if (strcmp(commandLine, "NOTES") == 0 || strcmp(commandLine, "PAD") == 0) {
    commandLine[0] = '\0'; commandLength = 0; switchApp(AppId::Scratchpad); return;
  } else if (strcmp(commandLine, "CLEAR") == 0) {
    historyCount = 0;
  } else if (strncmp(commandLine, "ECHO ", 5) == 0) {
    addHistory(commandLine + 5);
  } else if (strcmp(commandLine, "MMBASIC") == 0 || strcmp(commandLine, "C") == 0 ||
             strcmp(commandLine, "AUDIO") == 0 || strcmp(commandLine, "AGENT") == 0) {
    addHistory("That native subsystem is planned, not simulated.");
  } else if (commandLine[0] != '\0') {
    addHistory("Unknown command. Type HELP.");
  }
  commandLine[0] = '\0';
  commandLength = 0;
  drawShell();
}

void handleShellKey(uint8_t key) {
  
  if (key == 0x7F || key == KEY_DELETE) {
    if (commandLength > 0) {
      commandLine[--commandLength] = '\0';
      drawPrompt();
    }
  } else if (key == KEY_ENTER || key == 0x0D) {
    runCommand();
  } else if (key >= 32 && key <= 126 && commandLength < MAX_COMMAND) {
    commandLine[commandLength++] = static_cast<char>(key);
    commandLine[commandLength] = '\0';
    drawPrompt();
  }
}

bool isGameRom(const char *name) {
  const size_t length = strlen(name);
  if (length < 4) return false;
  const char *ext = name + length - 3;
  return ext[0] == '.' && (ext[1] == 'g' || ext[1] == 'G') && (ext[2] == 'b' || ext[2] == 'B');
}

void refreshGameFolder() {
  gameEntryCount = 0;
  if (!sdReady) return;
  File directory = SD.open(gameFolder);
  if (!directory || !directory.isDirectory()) { if (directory) directory.close(); return; }
  while (gameEntryCount < GAME_BROWSER_ITEMS) {
    File entry = directory.openNextFile();
    if (!entry) break;
    const bool directoryEntry = entry.isDirectory();
    if (directoryEntry || isGameRom(entry.name())) {
      const char *base = strrchr(entry.name(), '/');
      base = base ? base + 1 : entry.name();
      strncpy(gameEntries[gameEntryCount], base, EXPLORER_NAME_LEN);
      gameEntries[gameEntryCount][EXPLORER_NAME_LEN] = '\0';
      if (entry.name()[0] == '/') {
        strncpy(gameEntryPaths[gameEntryCount], entry.name(), GAME_PATH_LEN - 1);
        gameEntryPaths[gameEntryCount][GAME_PATH_LEN - 1] = '\0';
      } else if (strcmp(gameFolder, "/") == 0) {
        snprintf(gameEntryPaths[gameEntryCount], GAME_PATH_LEN, "/%s", entry.name());
      } else {
        snprintf(gameEntryPaths[gameEntryCount], GAME_PATH_LEN, "%s/%s", gameFolder, entry.name());
      }
      gameEntryIsDirectory[gameEntryCount] = directoryEntry;
      ++gameEntryCount;
    }
    entry.close();
  }
  directory.close();
  if (gameEntryCount == 0) gameSelection = 0;
  else if (gameSelection >= gameEntryCount) gameSelection = gameEntryCount - 1;
}

void drawGameBrowser() {
  display.fillScreen(COLOR_BG);
  drawTopBar("G.A.M.E.", "");
  drawRoundedPanel(12, 66, 296, 48, COLOR_SURFACE, COLOR_ACCENT_DARK);
  text(COLOR_ACCENT, 1, 22, 76, "GENERATING ABSOLUTELY MINIMAL EMULATION");
  char location[32];
  clippedText(location, sizeof(location), gameFolder, 28);
  text(COLOR_TEXT, 1, 22, 94, location);
  display.drawRect(12, 124, 296, 150, COLOR_ACCENT_DARK);
  if (!sdReady) text(COLOR_TEXT, 1, 26, 152, "Insert a FAT32 SD card.");
  else if (gameEntryCount == 0) text(COLOR_MUTED, 1, 26, 152, "No folders or .GB files here.");
  for (uint8_t i = 0; i < gameEntryCount; ++i) drawGameBrowserRow(i);
  drawFooter("UP/DOWN BROWSE  ENTER OPEN  LEFT UP  BACK HOME");
}



void drawGameBrowserRow(uint8_t index) {
  if (index >= gameEntryCount) return;
  const int16_t y = 136 + index * 16;
  const bool selected = index == gameSelection;
  display.fillRect(18, y - 1, 284, 14, COLOR_BLACK);
  if (selected) display.fillRoundRect(18, y - 1, 284, 14, 3, COLOR_SURFACE_2);
  char label[29];
  clippedText(label, sizeof(label), gameEntries[index], 25);
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 26, y, selected ? ">" : " ");
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 40, y, gameEntryIsDirectory[index] ? "[D]" : "[GB]");
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 70, y, label);
}

void drawGameBrowserSelection(uint8_t previous) {
  drawGameBrowserRow(previous);
  drawGameBrowserRow(gameSelection);
}

void drawGameNotice(const char *message) {
  gameMode = GameMode::Notice;
  strncpy(gameNotice, message, sizeof(gameNotice) - 1);
  gameNotice[sizeof(gameNotice) - 1] = '\0';
  drawGameBrowser();
  drawRoundedPanel(34, 112, 252, 94, COLOR_SURFACE, COLOR_ACCENT);
  text(COLOR_ACCENT, 1, 50, 128, "G.A.M.E.");
  text(COLOR_TEXT, 1, 50, 154, gameNotice);
  text(COLOR_MUTED, 1, 50, 182, "ENTER OR BACK TO CONTINUE");
}

void drawGameMenu() {
  
  drawRoundedPanel(42, 78, 236, 168, COLOR_SURFACE, COLOR_ACCENT);
  ditherRect(48, 84, 224, 156, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 58, 94, "G.A.M.E. MENU");
  const char *items[] = {"Resume", "Save State", "Load State", "Exit to Home"};
  for (uint8_t i = 0; i < 4; ++i) {
    const int16_t y = 120 + i * 23;
    const bool selected = i == static_cast<uint8_t>(gameMenuItem);
    if (selected) display.fillRoundRect(54, y - 2, 210, 17, 4, COLOR_SURFACE_2);
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 62, y, selected ? ">" : " ");
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 78, y, items[i]);
  }
  const EricGameBoy::PerformanceStats stats = EricGameBoy::performanceStats();
  char performanceLine[48];
  
  
  snprintf(performanceLine, sizeof(performanceLine), "LATE:%lu/%lu ROM:%lums LCD:%lums",
           static_cast<unsigned long>(stats.lateFrames),
           static_cast<unsigned long>(stats.emulatedFrames),
           static_cast<unsigned long>(stats.worstRomReadUs / 1000UL),
           static_cast<unsigned long>(stats.worstLcdBandUs / 1000UL));
  text(COLOR_MUTED, 1, 58, 210, performanceLine);
  text(COLOR_MUTED, 1, 58, 226, "F1 OR BACK RESUME");
}




void calcSkipSpaces() { while (calcParseText[calcParseAt] == ' ') ++calcParseAt; }
float calcExpressionValue();
float calcPrimary() {
  calcSkipSpaces();
  const char c = calcParseText[calcParseAt];
  if (c == '(') { ++calcParseAt; const float value = calcExpressionValue(); calcSkipSpaces(); if (calcParseText[calcParseAt] == ')') ++calcParseAt; else calcParseOk = false; return value; }
  if (c == 'x' || c == 'X') { ++calcParseAt; return calcParseX; }
  const char *word = calcParseText + calcParseAt;
  float (*function)(float) = nullptr;
  uint8_t length = 0;
  if (strncmp(word, "sin", 3) == 0) { function = sinf; length = 3; }
  else if (strncmp(word, "cos", 3) == 0) { function = cosf; length = 3; }
  else if (strncmp(word, "tan", 3) == 0) { function = tanf; length = 3; }
  else if (strncmp(word, "abs", 3) == 0) { function = fabsf; length = 3; }
  else if (strncmp(word, "sqrt", 4) == 0) { function = sqrtf; length = 4; }
  if (function != nullptr) {
    calcParseAt += length; calcSkipSpaces();
    if (calcParseText[calcParseAt] != '(') { calcParseOk = false; return 0.0f; }
    ++calcParseAt; const float value = calcExpressionValue(); calcSkipSpaces();
    if (calcParseText[calcParseAt] == ')') ++calcParseAt; else calcParseOk = false;
    return function(value);
  }
  char *end = nullptr;
  const float value = strtof(word, &end);
  if (end == word) { calcParseOk = false; return 0.0f; }
  calcParseAt += static_cast<uint8_t>(end - word);
  return value;
}
float calcPower() { float value = calcPrimary(); calcSkipSpaces(); if (calcParseText[calcParseAt] == '^') { ++calcParseAt; value = powf(value, calcPower()); } return value; }
float calcTerm() { float value = calcPower(); while (calcParseOk) { calcSkipSpaces(); const char op = calcParseText[calcParseAt]; if (op != '*' && op != '/') break; ++calcParseAt; const float rhs = calcPower(); if (op == '*') value *= rhs; else if (rhs == 0.0f) calcParseOk = false; else value /= rhs; } return value; }
float calcExpressionValue() { calcSkipSpaces(); bool negate = false; if (calcParseText[calcParseAt] == '+' || calcParseText[calcParseAt] == '-') negate = calcParseText[calcParseAt++] == '-'; float value = calcTerm(); if (negate) value = -value; while (calcParseOk) { calcSkipSpaces(); const char op = calcParseText[calcParseAt]; if (op != '+' && op != '-') break; ++calcParseAt; const float rhs = calcTerm(); value = op == '+' ? value + rhs : value - rhs; } return value; }
bool evaluateCalculator(float x, float &value) { calcParseText = calcExpression; calcParseAt = 0; calcParseX = x; calcParseOk = true; value = calcExpressionValue(); calcSkipSpaces(); return calcParseOk && calcParseText[calcParseAt] == '\0' && isfinite(value); }

bool calcGraphClipLine(float &x0, float &y0, float &x1, float &y1, float left, float top, float right, float bottom) {
  if (!isfinite(x0) || !isfinite(y0) || !isfinite(x1) || !isfinite(y1)) return false;
  const float dx = x1 - x0;
  const float dy = y1 - y0;
  float t0 = 0.0f;
  float t1 = 1.0f;
  const float p[4] = {-dx, dx, -dy, dy};
  const float q[4] = {x0 - left, right - x0, y0 - top, bottom - y0};
  for (uint8_t i = 0; i < 4; ++i) {
    if (p[i] == 0.0f) {
      if (q[i] < 0.0f) return false;
      continue;
    }
    const float r = q[i] / p[i];
    if (p[i] < 0.0f) {
      if (r > t1) return false;
      if (r > t0) t0 = r;
    } else {
      if (r < t0) return false;
      if (r < t1) t1 = r;
    }
  }
  float ax = x0 + t0 * dx;
  float ay = y0 + t0 * dy;
  float bx = x0 + t1 * dx;
  float by = y0 + t1 * dy;
  if (ax < left) ax = left; else if (ax > right) ax = right;
  if (bx < left) bx = left; else if (bx > right) bx = right;
  if (ay < top) ay = top; else if (ay > bottom) ay = bottom;
  if (by < top) by = top; else if (by > bottom) by = bottom;
  x0 = ax; y0 = ay; x1 = bx; y1 = by;
  return true;
}

void calcGraphDrawLine(float x0, float y0, float x1, float y1, uint16_t color, int16_t left, int16_t top, int16_t right, int16_t bottom) {
  if (!calcGraphClipLine(x0, y0, x1, y1, left, top, right, bottom)) return;
  const int16_t ax = static_cast<int16_t>(floorf(x0 + 0.5f));
  const int16_t ay = static_cast<int16_t>(floorf(y0 + 0.5f));
  const int16_t bx = static_cast<int16_t>(floorf(x1 + 0.5f));
  const int16_t by = static_cast<int16_t>(floorf(y1 + 0.5f));
  display.drawLine(ax, ay, bx, by, color);
}

void calcGraphPlotPixelClipped(int16_t x, int16_t y, uint16_t color, int16_t left, int16_t top, int16_t right, int16_t bottom) {
  if (x >= left && x <= right && y >= top && y <= bottom) display.drawPixel(x, y, color);
}

void calcGraphDrawFilledCircleClipped(int16_t cx, int16_t cy, int16_t radius, uint16_t color, int16_t left, int16_t top, int16_t right, int16_t bottom) {
  if (radius <= 0) return;
  const int16_t r2 = radius * radius;
  for (int16_t dy = -radius; dy <= radius; ++dy) {
    const int16_t span = static_cast<int16_t>(sqrtf(static_cast<float>(r2 - dy * dy)));
    int16_t x0 = cx - span;
    int16_t x1 = cx + span;
    const int16_t y = cy + dy;
    if (x0 < left) x0 = left;
    if (x1 > right) x1 = right;
    if (y >= top && y <= bottom && x0 <= x1) display.drawFastHLine(x0, y, static_cast<int16_t>(x1 - x0 + 1), color);
  }
}

void calcGraphDrawCircleClipped(int16_t cx, int16_t cy, int16_t radius, uint16_t color, int16_t left, int16_t top, int16_t right, int16_t bottom) {
  if (radius <= 0) return;
  const int16_t r2 = radius * radius;
  for (int16_t dy = -radius; dy <= radius; ++dy) {
    const int16_t span = static_cast<int16_t>(sqrtf(static_cast<float>(r2 - dy * dy)));
    calcGraphPlotPixelClipped(cx + span, cy + dy, color, left, top, right, bottom);
    calcGraphPlotPixelClipped(cx - span, cy + dy, color, left, top, right, bottom);
    calcGraphPlotPixelClipped(cx + dy, cy + span, color, left, top, right, bottom);
    calcGraphPlotPixelClipped(cx - dy, cy + span, color, left, top, right, bottom);
    calcGraphPlotPixelClipped(cx + dy, cy - span, color, left, top, right, bottom);
    calcGraphPlotPixelClipped(cx - dy, cy - span, color, left, top, right, bottom);
  }
}

void drawCalculatorGraph() {
  constexpr int16_t GX = 12, GY = 106, GW = 296, GH = 166;
  constexpr int16_t GRAPH_LEFT = GX, GRAPH_TOP = GY;
  constexpr int16_t GRAPH_RIGHT = GX + GW - 1, GRAPH_BOTTOM = GY + GH - 1;
  display.fillRect(GX, GY, GW, GH, COLOR_BLACK);
  display.drawRect(GX, GY, GW, GH, COLOR_ACCENT_DARK);
  const float xMin = calcCenterX - calcRange;
  const float yRange = calcRange * (static_cast<float>(GH) / GW);
  const float yMin = calcCenterY - yRange;
  const float axisXf = GX + (0.0f - xMin) * GW / (2.0f * calcRange);
  const float axisYf = GY + GH - (0.0f - yMin) * GH / (2.0f * yRange);
  if (isfinite(axisXf) && axisXf >= GX && axisXf < GX + GW) display.drawFastVLine(static_cast<int16_t>(axisXf), GY + 1, GH - 2, COLOR_SURFACE_2);
  if (isfinite(axisYf) && axisYf >= GY && axisYf < GY + GH) display.drawFastHLine(GX + 1, static_cast<int16_t>(axisYf), GW - 2, COLOR_SURFACE_2);
  bool previousValid = false; int16_t previousY = 0;
  for (int16_t pixel = 0; pixel < GW; ++pixel) {
    const float x = xMin + (static_cast<float>(pixel) * 2.0f * calcRange / (GW - 1));
    float y = 0.0f;
    const bool valid = evaluateCalculator(x, y) && y >= yMin - yRange && y <= yMin + 3.0f * yRange;
    if (valid) {
      const float plotYf = GY + GH - 1 - (y - yMin) * GH / (2.0f * yRange);
      if (isfinite(plotYf) && plotYf >= GY && plotYf < GY + GH) {
        const int16_t plotY = static_cast<int16_t>(plotYf);
        if (previousValid && abs(plotY - previousY) < 45) calcGraphDrawLine(GX + pixel - 1, previousY, GX + pixel, plotY, COLOR_ACCENT, GRAPH_LEFT, GRAPH_TOP, GRAPH_RIGHT, GRAPH_BOTTOM);
        else calcGraphPlotPixelClipped(GX + pixel, plotY, COLOR_ACCENT, GRAPH_LEFT, GRAPH_TOP, GRAPH_RIGHT, GRAPH_BOTTOM);
        previousY = plotY; previousValid = true;
      } else previousValid = false;
    } else previousValid = false;
  }
  float value = 0.0f, left = 0.0f, right = 0.0f;
  const bool pointOk = evaluateCalculator(calcProbeX, value);
  const float step = calcRange / 100.0f;
  const bool slopeOk = evaluateCalculator(calcProbeX - step, left) && evaluateCalculator(calcProbeX + step, right);
  const float slope = slopeOk ? (right - left) / (2.0f * step) : 0.0f;
  const float probePixelXf = GX + (calcProbeX - xMin) * (GW - 1) / (2.0f * calcRange);
  if (isfinite(probePixelXf) && probePixelXf >= GX && probePixelXf < GX + GW) {
    const int16_t probePixelX = static_cast<int16_t>(probePixelXf);
    display.drawFastVLine(probePixelX, GY + 1, GH - 2, COLOR_MUTED);
    if (pointOk) {
      const float probePixelYf = GY + GH - 1 - (value - yMin) * GH / (2.0f * yRange);
      if (isfinite(probePixelYf) && probePixelYf >= GY && probePixelYf < GY + GH) {
        const int16_t probePixelY = static_cast<int16_t>(probePixelYf);
        const float pixelsPerUnitX = (GW - 1) / (2.0f * calcRange);
        const float pixelsPerUnitY = GH / (2.0f * yRange);
        const float tangentHalfWidth = 18.0f;
        if (isfinite(slope)) {
          const float tangentRise = slope * tangentHalfWidth * pixelsPerUnitY / pixelsPerUnitX;
          calcGraphDrawLine(probePixelX - tangentHalfWidth, probePixelY + tangentRise,
                            probePixelX + tangentHalfWidth, probePixelY - tangentRise,
                            COLOR_MUTED, GRAPH_LEFT, GRAPH_TOP, GRAPH_RIGHT, GRAPH_BOTTOM);
        }
        calcGraphDrawFilledCircleClipped(probePixelX, probePixelY, 3, COLOR_ACCENT, GRAPH_LEFT, GRAPH_TOP, GRAPH_RIGHT, GRAPH_BOTTOM);
        calcGraphDrawCircleClipped(probePixelX, probePixelY, 4, COLOR_TEXT, GRAPH_LEFT, GRAPH_TOP, GRAPH_RIGHT, GRAPH_BOTTOM);
      }
    }
  }
  display.fillRect(12, 273, 296, 12, COLOR_BLACK);
  if (pointOk) {
    char xText[12], yText[14], slopeText[14], readout[58];
    formatCalculatorMeasurement(calcProbeX, 2, xText, sizeof(xText));
    formatCalculatorMeasurement(value, 3, yText, sizeof(yText));
    formatCalculatorMeasurement(slope, 3, slopeText, sizeof(slopeText));
    snprintf(readout, sizeof(readout), "X:%s Y:%s S:%s", xText, yText, slopeText);
    text(COLOR_TEXT, 1, 18, 275, readout);
  } else {
    text(COLOR_ACCENT, 1, 18, 275, "EQUATION NEEDS CORRECTION");
  }
}



void drawCalculatorInput() {
  display.fillRect(13, 77, 294, 20, COLOR_BLACK);
  text(COLOR_TEXT, 1, 18, 83, "y=");
  constexpr uint8_t VISIBLE_CHARS = 43;
  uint8_t firstChar = 0;
  if (calcCursor >= VISIBLE_CHARS) firstChar = calcCursor - (VISIBLE_CHARS - 1);
  char visible[VISIBLE_CHARS + 1];
  strncpy(visible, calcExpression + firstChar, VISIBLE_CHARS);
  visible[VISIBLE_CHARS] = '\0';
  text(COLOR_TEXT, 1, 36, 83, visible);
  const int16_t cursorX = 36 + (calcCursor - firstChar) * 6;
  display.fillRect(cursorX, 82, 4, 10, COLOR_ACCENT);
}

const CalcConversionFamily &calculatorFamily() { return calcConversionFamilies[calcConversionSelection]; }

float calculatorConvertedValue(float input, bool fromLeft) {
  const uint8_t sourceUnit = fromLeft ? calcLeftUnit : calcRightUnit;
  const uint8_t targetUnit = fromLeft ? calcRightUnit : calcLeftUnit;
  if (calcConversionSelection == 0) {
    
    const float celsius = sourceUnit == 0 ? input : (sourceUnit == 1 ? (input - 32.0f) * 5.0f / 9.0f : input - 273.15f);
    return targetUnit == 0 ? celsius : (targetUnit == 1 ? celsius * 9.0f / 5.0f + 32.0f : celsius + 273.15f);
  }
  const CalcConversionFamily &family = calculatorFamily();
  return input * family.units[sourceUnit].baseFactor / family.units[targetUnit].baseFactor;
}



bool calcReadConversionInput(float &value) {
  if (calcValueLength == 0) return false;
  char *end = nullptr;
  value = strtof(calcValueInput, &end);
  return end != calcValueInput && *end == '\0' && isfinite(value);
}




void formatCalculatorConversionValue(float value, char *out, size_t outSize) {
  const bool negative = value < 0.0f;
  const float magnitude = negative ? -value : value;
  const long hundredths = static_cast<long>(magnitude * 100.0f + 0.5f);
  const long whole = hundredths / 100L;
  const long fraction = hundredths % 100L;
  if (fraction == 0) snprintf(out, outSize, "%s%ld", negative ? "-" : "", whole);
  else if ((fraction % 10L) == 0) snprintf(out, outSize, "%s%ld.%ld", negative ? "-" : "", whole, fraction / 10L);
  else snprintf(out, outSize, "%s%ld.%02ld", negative ? "-" : "", whole, fraction);
}




void formatCalculatorMeasurement(float value, uint8_t decimals, char *out, size_t outSize) {
  if (!isfinite(value)) {
    strncpy(out, "--", outSize - 1);
    out[outSize - 1] = '\0';
    return;
  }
  bool negative = value < 0.0f;
  float magnitude = negative ? -value : value;
  long scale = 1;
  for (uint8_t i = 0; i < decimals; ++i) scale *= 10L;
  const long scaled = static_cast<long>(magnitude * static_cast<float>(scale) + 0.5f);
  const long whole = scaled / scale;
  const long fraction = scaled % scale;
  if (decimals == 0) snprintf(out, outSize, "%s%ld", negative ? "-" : "+", whole);
  else snprintf(out, outSize, "%s%ld.%0*ld", negative ? "-" : "+", whole, decimals, fraction);
}

void calcUseOppositePane() {
  float current = 0.0f;
  if (calcReadConversionInput(current)) {
    const float converted = calculatorConvertedValue(current, calcValueFromLeft);
    formatCalculatorConversionValue(converted, calcValueInput, sizeof(calcValueInput));
    calcValueLength = strlen(calcValueInput);
  }
  calcValueFromLeft = !calcValueFromLeft;
}



void drawCalculatorConversionValue() {
  constexpr int16_t LEFT_X = 28, RIGHT_X = 166, BOX_Y = 154, BOX_W = 126, BOX_H = 70;
  const CalcConversionFamily &family = calculatorFamily();
  const char *leftUnit = family.units[calcLeftUnit].name;
  const char *rightUnit = family.units[calcRightUnit].name;
  float source = 0.0f;
  const bool hasSource = calcReadConversionInput(source);
  char convertedText[CALC_VALUE_LEN + 1] = "";
  if (hasSource) {
    const float converted = calculatorConvertedValue(source, calcValueFromLeft);
    formatCalculatorConversionValue(converted, convertedText, sizeof(convertedText));
  }
  
  
  const char *leftText = calcValueFromLeft ? calcValueInput : convertedText;
  const char *rightText = calcValueFromLeft ? convertedText : calcValueInput;
  display.fillRect(LEFT_X, BOX_Y, BOX_W, BOX_H, COLOR_BLACK);
  display.fillRect(RIGHT_X, BOX_Y, BOX_W, BOX_H, COLOR_BLACK);
  drawRoundedPanel(LEFT_X, BOX_Y, BOX_W, BOX_H, calcValueFromLeft ? COLOR_SURFACE_2 : COLOR_SURFACE, calcValueFromLeft ? COLOR_ACCENT : COLOR_ACCENT_DARK);
  drawRoundedPanel(RIGHT_X, BOX_Y, BOX_W, BOX_H, calcValueFromLeft ? COLOR_SURFACE : COLOR_SURFACE_2, calcValueFromLeft ? COLOR_ACCENT_DARK : COLOR_ACCENT);
  text(calcValueFromLeft ? COLOR_ACCENT : COLOR_MUTED, 1, LEFT_X + 10, BOX_Y + 9, leftUnit);
  text(calcValueFromLeft ? COLOR_MUTED : COLOR_ACCENT, 1, RIGHT_X + 10, BOX_Y + 9, rightUnit);
  text(COLOR_TEXT, 2, LEFT_X + 10, BOX_Y + 29, leftText[0] ? leftText : "-");
  text(COLOR_TEXT, 2, RIGHT_X + 10, BOX_Y + 29, rightText[0] ? rightText : "-");
  const int16_t activeX = (calcValueFromLeft ? LEFT_X : RIGHT_X) + 10 + strlen(calcValueInput) * 12;
  display.fillRect(activeX, BOX_Y + 28, 4, 16, COLOR_ACCENT);
  text(COLOR_MUTED, 1, LEFT_X + 10, BOX_Y + 55, calcValueFromLeft ? "EDITING" : "LIVE RESULT");
  text(COLOR_MUTED, 1, RIGHT_X + 10, BOX_Y + 55, calcValueFromLeft ? "LIVE RESULT" : "EDITING");
}

void drawCalculatorUnitPicker() {
  const CalcConversionFamily &family = calculatorFamily();
  drawRoundedPanel(42, 72, 236, 182, COLOR_SURFACE, COLOR_ACCENT);
  ditherRect(48, 78, 224, 170, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 58, 88, calcUnitPickerForLeft ? "LEFT PANE UNIT" : "RIGHT PANE UNIT");
  for (uint8_t i = 0; i < family.count; ++i) {
    const int16_t y = 110 + i * 16;
    const bool selected = i == calcUnitPickerSelection;
    if (selected) display.fillRoundRect(54, y - 2, 210, 14, 3, COLOR_SURFACE_2);
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 62, y, selected ? ">" : " ");
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 78, y, family.units[i].name);
  }
  text(COLOR_MUTED, 1, 58, 232, "UP/DOWN CHOOSE  ENTER SET  BACK CANCEL");
}

void drawCalculatorConverter() {
  display.fillScreen(COLOR_BG);
  drawTopBar("A.D.D.", "CONVERT");
  text(COLOR_ACCENT, 1, 16, 64, "ARITHMETIC DECODER DEVICE");
  drawRoundedPanel(16, 82, 288, 162, COLOR_SURFACE, COLOR_ACCENT_DARK);
  text(COLOR_MUTED, 1, 30, 96, "TWO-WAY UNIT CONVERTER");
  const CalcConversionFamily &family = calculatorFamily();
  char route[30];
  snprintf(route, sizeof(route), "%s  < >  %s", family.units[calcLeftUnit].name, family.units[calcRightUnit].name);
  text(COLOR_ACCENT, 2, 30, 118, route);
  text(COLOR_MUTED, 1, 30, 140, "L/R PANE  ENTER CHOOSES THAT UNIT");
  drawCalculatorConversionValue();
  if (calcUnitPickerVisible) drawCalculatorUnitPicker();
  char preset[34];
  snprintf(preset, sizeof(preset), "%u/3  UP/DOWN CHANGE TYPE", calcConversionSelection + 1);
  text(COLOR_MUTED, 1, 30, 232, preset);
  drawFooter("TYPE VALUE  L/R PANE  ENTER UNITS  U/D TYPE");
}

void drawCalculator() {
  if (calcView == CalcView::Convert) { drawCalculatorConverter(); return; }
  display.fillScreen(COLOR_BG);
  drawTopBar("A.D.D.", "GRAPHING");
  text(COLOR_ACCENT, 1, 16, 64, "ARITHMETIC DECODER DEVICE");
  display.drawRect(12, 76, 296, 22, COLOR_ACCENT_DARK);
  drawCalculatorInput();
  drawCalculatorGraph();
  drawFooter("TYPE LIVE  LEFT/RIGHT MEASURE  F1 IN  F OUT  U UNITS");
}

void handleCalculatorKey(uint8_t key) {
  if (key == KEY_BACK) { goBack(); return; }
  
  
  if (key == 'u' || key == 'U') { calcView = calcView == CalcView::Graph ? CalcView::Convert : CalcView::Graph; drawCalculator(); return; }
  if (calcView == CalcView::Convert) {
    if (calcUnitPickerVisible) {
      const CalcConversionFamily &family = calculatorFamily();
      if (key == KEY_BACK) { calcUnitPickerVisible = false; drawCalculatorConverter(); return; }
      if (key == KEY_UP || key == KEY_JOY_UP) {
        calcUnitPickerSelection = (calcUnitPickerSelection + family.count - 1) % family.count;
        drawCalculatorUnitPicker(); return;
      }
      if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
        calcUnitPickerSelection = (calcUnitPickerSelection + 1) % family.count;
        drawCalculatorUnitPicker(); return;
      }
      if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
        if (calcUnitPickerForLeft) calcLeftUnit = calcUnitPickerSelection;
        else calcRightUnit = calcUnitPickerSelection;
        calcUnitPickerVisible = false;
        drawCalculatorConverter(); return;
      }
      return;
    }
    if (key == KEY_UP || key == KEY_JOY_UP) {
      calcConversionSelection = (calcConversionSelection + CALC_CONVERSION_COUNT - 1) % CALC_CONVERSION_COUNT;
      calcLeftUnit = 0;
      calcRightUnit = calcConversionSelection == 0 ? 1 : (calcConversionSelection == 1 ? 2 : 4);
      calcValueFromLeft = true;
      drawCalculatorConverter(); return;
    }
    if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
      calcConversionSelection = (calcConversionSelection + 1) % CALC_CONVERSION_COUNT;
      calcLeftUnit = 0;
      calcRightUnit = calcConversionSelection == 0 ? 1 : (calcConversionSelection == 1 ? 2 : 4);
      calcValueFromLeft = true;
      drawCalculatorConverter(); return;
    }
    if (key == KEY_LEFT || key == KEY_JOY_LEFT || key == KEY_RIGHT || key == KEY_JOY_RIGHT) {
      
      calcUseOppositePane();
      drawCalculatorConversionValue(); return;
    }
    if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      calcUnitPickerForLeft = calcValueFromLeft;
      calcUnitPickerSelection = calcUnitPickerForLeft ? calcLeftUnit : calcRightUnit;
      calcUnitPickerVisible = true;
      drawCalculatorUnitPicker(); return;
    }
    if (key == KEY_DELETE || key == 0x7F) {
      if (calcValueLength) calcValueInput[--calcValueLength] = '\0';
      drawCalculatorConversionValue(); return;
    }
    if ((key >= '0' && key <= '9') || key == '.' || key == '-') {
      if (calcValueLength < CALC_VALUE_LEN) {
        calcValueInput[calcValueLength++] = static_cast<char>(key);
        calcValueInput[calcValueLength] = '\0';
        drawCalculatorConversionValue();
      }
    }
    return;
  }
  if (key == KEY_LEFT || key == KEY_JOY_LEFT) { calcProbeX -= calcRange / 10.0f; drawCalculatorGraph(); return; }
  if (key == KEY_RIGHT || key == KEY_JOY_RIGHT) { calcProbeX += calcRange / 10.0f; drawCalculatorGraph(); return; }
  if (key == KEY_UP || key == KEY_JOY_UP) { calcCenterY += calcRange / 8.0f; drawCalculatorGraph(); return; }
  if (key == KEY_DOWN || key == KEY_JOY_DOWN) { calcCenterY -= calcRange / 8.0f; drawCalculatorGraph(); return; }
  if (key == KEY_F1) { if (calcRange > 0.625f) calcRange *= 0.5f; drawCalculatorGraph(); return; }
  if (key == 'f' || key == 'F') { if (calcRange < 80.0f) calcRange *= 2.0f; drawCalculatorGraph(); return; }
  if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) { float ignored; if (evaluateCalculator(calcProbeX, ignored)) strncpy(calcStatus, "GRAPH UPDATED", sizeof(calcStatus)); else strncpy(calcStatus, "CHECK EQUATION", sizeof(calcStatus)); calcStatus[sizeof(calcStatus) - 1] = '\0'; drawCalculator(); return; }
  if (key == KEY_DELETE || key == 0x7F) {
    if (calcExpressionLength) { calcExpression[--calcExpressionLength] = '\0'; calcCursor = calcExpressionLength; }
    drawCalculatorInput(); drawCalculatorGraph(); return;
  }
  if ((key >= 32 && key <= 126) && calcExpressionLength < CALC_EXPR_LEN) {
    calcExpression[calcExpressionLength++] = static_cast<char>(key);
    calcExpression[calcExpressionLength] = '\0'; calcCursor = calcExpressionLength;
    drawCalculatorInput(); drawCalculatorGraph();
  }
}

void openGameWorkspace() {
  strcpy(gameFolder, "/");
  gameSelection = 0;
  gameMode = GameMode::Browser;
  refreshGameFolder();
  switchApp(AppId::GameBoy);
}

void runGameFrame() {
  
  
  
  
  
  
  
  if (activeApp == AppId::GameBoy && gameMode == GameMode::Playing && !quickMenuVisible) EricGameBoy::presentFrame();
}

void handleGameKey(uint8_t key) {
  if (gameMode == GameMode::Notice) {
    if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER || key == KEY_BACK) {
      gameMode = GameMode::Browser; drawGameBrowser();
    }
    return;
  }
  if (gameMode == GameMode::Menu) {
    if (key == KEY_F1 || key == KEY_BACK) {
      EricGameBoy::setPaused(false);
      gameMode = GameMode::Playing;
      return;
    }
    if (key == KEY_UP || key == KEY_JOY_UP) {
      gameMenuItem = static_cast<GameMenuItem>((static_cast<uint8_t>(gameMenuItem) + 3) % 4); drawGameMenu();
    } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
      gameMenuItem = static_cast<GameMenuItem>((static_cast<uint8_t>(gameMenuItem) + 1) % 4); drawGameMenu();
    } else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      if (gameMenuItem == GameMenuItem::Resume) {
        EricGameBoy::setPaused(false);
        gameMode = GameMode::Playing;
      } else if (gameMenuItem == GameMenuItem::SaveState) {
        if (!EricGameBoy::saveState()) drawGameNotice("Could not save state.");
        else { EricGameBoy::setPaused(false); gameMode = GameMode::Playing; }
      } else if (gameMenuItem == GameMenuItem::LoadState) {
        if (!EricGameBoy::loadState()) drawGameNotice("No matching state found.");
        else { EricGameBoy::setPaused(false); gameMode = GameMode::Playing; }
      } else switchApp(AppId::Home);
    }
    return;
  }
  if (gameMode == GameMode::Playing) {
    if (key == KEY_F1 || key == KEY_BACK) {
      
      
      
      
      EricGameBoy::setPaused(true);
      EricGameBoy::yieldDisplayToSystem();
      gameMode = GameMode::Menu;
      drawGameMenu();
      return;
    }
    uint8_t buttons = 0;
    bool direction = false;
    if (key == KEY_RIGHT || key == KEY_JOY_RIGHT) { buttons = 16; direction = true; }
    else if (key == KEY_LEFT || key == KEY_JOY_LEFT) { buttons = 32; direction = true; }
    else if (key == KEY_UP || key == KEY_JOY_UP) { buttons = 64; direction = true; }
    else if (key == KEY_DOWN || key == KEY_JOY_DOWN) { buttons = 128; direction = true; }
    
    
    else if (key == '=') buttons = 1;
    else if (key == '-') buttons = 2;
    else if (key == '[') buttons = 4;
    else if (key == ']') buttons = 8;
    if (buttons) {
      
      
      
      
      EricGameBoy::setButtons(buttons, direction ? 115UL : 52UL);
    }
    return;
  }
  if (key == KEY_UP || key == KEY_JOY_UP) {
    const uint8_t previous = gameSelection;
    if (gameEntryCount && gameSelection > 0) --gameSelection;
    drawGameBrowserSelection(previous);
  } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
    const uint8_t previous = gameSelection;
    if (gameEntryCount && gameSelection + 1 < gameEntryCount) ++gameSelection;
    drawGameBrowserSelection(previous);
  } else if (key == KEY_LEFT || key == KEY_JOY_LEFT) {
    if (strcmp(gameFolder, "/") != 0) {
      char *slash = strrchr(gameFolder, '/');
      if (slash == gameFolder) gameFolder[1] = '\0'; else *slash = '\0';
      gameSelection = 0; refreshGameFolder(); drawGameBrowser();
    }
  } else if (key == KEY_BACK) {
    goBack();
  } else if ((key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) && gameEntryCount) {
    const char *selectedPath = gameEntryPaths[gameSelection];
    if (gameEntryIsDirectory[gameSelection]) {
      if (selectedPath[0] != '\0' && strlen(selectedPath) < sizeof(gameFolder)) {
        strncpy(gameFolder, selectedPath, sizeof(gameFolder) - 1);
        gameFolder[sizeof(gameFolder) - 1] = '\0';
        gameSelection = 0;
        refreshGameFolder();
        drawGameBrowser();
      }
    } else {
      if (selectedPath[0] == '\0') { drawGameNotice("ROM path is unavailable."); return; }
      const EricGameBoy::StartResult result = EricGameBoy::start(selectedPath);
      if (result == EricGameBoy::StartResult::Ok) gameMode = GameMode::Playing;
      else if (result == EricGameBoy::StartResult::NoCard) drawGameNotice("SD card is unavailable.");
      else if (result == EricGameBoy::StartResult::InvalidRom) drawGameNotice("That is not a valid GB ROM.");
      else if (result == EricGameBoy::StartResult::SaveTooLarge) drawGameNotice("This ROM needs too much save RAM.");
      else drawGameNotice("This cartridge is unsupported.");
    }
  }
}

#if 0 
void openNetworkWorkspace() {
  networkMode = NetworkMode::List;
  networkSelection = 0;
  networkPassword[0] = '\0';
  networkPasswordLength = 0;
  EricWiFi::scan();
  switchApp(AppId::Network);
}

void drawNetworkWorkspace() {
  display.fillScreen(COLOR_BG);
  drawTopBar("W.I.F.I.", "");
  drawRoundedPanel(12, 66, 296, 48, COLOR_SURFACE, COLOR_ACCENT_DARK);
  text(COLOR_ACCENT, 1, 22, 76, "WACKY INTERNET FREQUENCING INVENTION");
  if (EricWiFi::state() == EricWiFi::State::Connected) {
    char line[48];
    snprintf(line, sizeof(line), "ONLINE: %s", EricWiFi::connectedName());
    text(COLOR_TEXT, 1, 22, 94, line);
  } else if (EricWiFi::state() == EricWiFi::State::Connecting) {
    text(COLOR_TEXT, 1, 22, 94, "CONNECTING IN BACKGROUND...");
  } else if (EricWiFi::state() == EricWiFi::State::NoCard) {
    text(COLOR_TEXT, 1, 22, 94, "INSERT SD CARD TO SAVE A NETWORK");
  } else {
    text(COLOR_MUTED, 1, 22, 94, "SELECT A NEARBY NETWORK");
  }
  display.drawRect(12, 124, 296, 150, COLOR_ACCENT_DARK);
  if (networkMode == NetworkMode::Password) {
    text(COLOR_TEXT, 1, 26, 142, EricWiFi::networkName(networkSelection));
    text(COLOR_MUTED, 1, 26, 164, "NETWORK PASSWORD");
    drawNetworkPasswordInput();
    text(COLOR_MUTED, 1, 26, 222, "ENTER CONNECT  DEL ERASE");
    text(COLOR_MUTED, 1, 26, 238, "BACK CANCEL");
  } else if (networkMode == NetworkMode::Status) {
    if (EricWiFi::state() == EricWiFi::State::Connected) {
      char ip[32]; snprintf(ip, sizeof(ip), "IP: %s", EricWiFi::ipAddress());
      text(COLOR_TEXT, 1, 26, 150, "CONNECTED");
      text(COLOR_TEXT, 1, 26, 170, ip);
      text(COLOR_MUTED, 1, 26, 208, "ENTER DISCONNECT");
      text(COLOR_MUTED, 1, 26, 224, "DEL FORGET THIS NETWORK");
    } else if (EricWiFi::state() == EricWiFi::State::Failed) {
      text(COLOR_TEXT, 1, 26, 150, "CONNECTION DID NOT COMPLETE.");
      text(COLOR_MUTED, 1, 26, 174, "CHECK PASSWORD, THEN TRY AGAIN.");
      text(COLOR_MUTED, 1, 26, 218, "ENTER SCAN AGAIN");
    } else {
      text(COLOR_TEXT, 1, 26, 150, "CONNECTING...");
      text(COLOR_MUTED, 1, 26, 174, "YOU CAN LEAVE THIS SCREEN SAFELY.");
    }
  } else if (EricWiFi::networkCount() == 0) {
    text(COLOR_MUTED, 1, 26, 154, "No networks found. ENTER scans again.");
  } else {
    for (uint8_t i = 0; i < EricWiFi::networkCount(); ++i) drawNetworkRow(i);
  }
  drawFooter(networkMode == NetworkMode::List ? "UP/DOWN CHOOSE  ENTER JOIN  BACK HOME" : "BACK RETURN");
}



void drawNetworkPasswordInput() {
  display.fillRect(26, 178, 260, 18, COLOR_BLACK);
  char dots[EricWiFi::PasswordLength + 1];
  const uint8_t shown = networkPasswordLength > 42 ? 42 : networkPasswordLength;
  for (uint8_t i = 0; i < shown; ++i) dots[i] = '*';
  dots[shown] = '\0';
  text(COLOR_TEXT, 1, 26, 184, dots);
  display.fillRect(26 + shown * 6, 183, 4, 9, COLOR_ACCENT);
}



void drawNetworkRow(uint8_t index) {
  if (index >= EricWiFi::networkCount()) return;
  const int16_t y = 138 + index * 16;
  const bool selected = index == networkSelection;
  display.fillRect(18, y - 1, 284, 14, COLOR_BLACK);
  if (selected) display.fillRoundRect(18, y - 1, 284, 14, 3, COLOR_SURFACE_2);
  char label[31]; clippedText(label, sizeof(label), EricWiFi::networkName(index), 22);
  char signal[14]; snprintf(signal, sizeof(signal), "%ddBm%s", EricWiFi::networkRssi(index), EricWiFi::networkSecure(index) ? " *" : "");
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 26, y, selected ? ">" : " ");
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 40, y, label);
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 238, y, signal);
}

void drawNetworkSelection(uint8_t previous) {
  drawNetworkRow(previous);
  drawNetworkRow(networkSelection);
}

void handleNetworkKey(uint8_t key) {
  if (networkMode == NetworkMode::Password) {
    if (key == KEY_BACK) { networkMode = NetworkMode::List; drawNetworkWorkspace(); }
    else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      EricWiFi::connect(EricWiFi::networkName(networkSelection), networkPassword);
      networkMode = NetworkMode::Status; drawNetworkWorkspace();
    } else if ((key == KEY_DELETE || key == 0x7F) && networkPasswordLength) {
      networkPassword[--networkPasswordLength] = '\0'; drawNetworkPasswordInput();
    } else if (key >= 32 && key <= 126 && networkPasswordLength < EricWiFi::PasswordLength) {
      networkPassword[networkPasswordLength++] = static_cast<char>(key);
      networkPassword[networkPasswordLength] = '\0'; drawNetworkPasswordInput();
    }
    return;
  }
  if (networkMode == NetworkMode::Status) {
    if (key == KEY_BACK) { networkMode = NetworkMode::List; drawNetworkWorkspace(); }
    else if (key == KEY_DELETE && EricWiFi::state() == EricWiFi::State::Connected) { EricWiFi::forgetSavedNetwork(); networkMode = NetworkMode::List; EricWiFi::scan(); drawNetworkWorkspace(); }
    else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      if (EricWiFi::state() == EricWiFi::State::Connected) EricWiFi::disconnect();
      else { EricWiFi::scan(); networkMode = NetworkMode::List; }
      drawNetworkWorkspace();
    }
    return;
  }
  if (key == KEY_BACK) { goBack(); return; }
  if (key == KEY_UP || key == KEY_JOY_UP) {
    const uint8_t previous = networkSelection;
    if (networkSelection) --networkSelection;
    if (networkSelection != previous) drawNetworkSelection(previous);
  }
  else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
    const uint8_t previous = networkSelection;
    if (networkSelection + 1 < EricWiFi::networkCount()) ++networkSelection;
    if (networkSelection != previous) drawNetworkSelection(previous);
  }
  else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
    if (EricWiFi::networkCount() == 0) EricWiFi::scan();
    else if (EricWiFi::networkSecure(networkSelection)) { networkPassword[0] = '\0'; networkPasswordLength = 0; networkMode = NetworkMode::Password; }
    else { EricWiFi::connect(EricWiFi::networkName(networkSelection), ""); networkMode = NetworkMode::Status; }
    drawNetworkWorkspace();
  }
}
#endif

void openRadioWorkspace() {
  radioSelection = 0;
  lastRadioState = EricRadio::state();
  switchApp(AppId::Radio);
}

void drawRadioRow(uint8_t index) {
  if (index >= EricRadio::StationCount) return;
  const int16_t y = 140 + index * 28;
  const EricRadio::State current = EricRadio::state();
  const bool selected = index == radioSelection;
  const bool active = (current == EricRadio::State::Connecting || current == EricRadio::State::Playing) && index == EricRadio::station();
  display.fillRect(18, y - 3, 284, 24, COLOR_BLACK);
  if (selected) display.fillRoundRect(18, y - 3, 284, 24, 3, COLOR_SURFACE_2);
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 26, y, selected ? ">" : " ");
  text(active ? COLOR_ACCENT : (selected ? COLOR_TEXT : COLOR_MUTED), 1, 42, y, EricRadio::stationName(index));
  text(selected ? COLOR_MUTED : COLOR_ACCENT_DARK, 1, 42, y + 11, EricRadio::stationGenre(index));
  if (active) {
    const char *stateLabel = current == EricRadio::State::Playing ? "ON AIR" : "PLAY";
    text(COLOR_ACCENT, 1, 250, y, stateLabel);
  }
}

void drawRadioSelection(uint8_t previous) {
  drawRadioRow(previous);
  drawRadioRow(radioSelection);
}

void drawRadioWorkspace() {
  display.fillScreen(COLOR_BG);
  drawTopBar("R.A.D.I.O.", "");
  drawRoundedPanel(12, 66, 296, 48, COLOR_SURFACE, COLOR_ACCENT_DARK);
  ditherRect(18, 72, 284, 36, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 22, 76, "REALLY AWESOME DIGITAL INTERNET OUTPUT");
  const EricRadio::State current = EricRadio::state();
  if (current == EricRadio::State::Playing) text(COLOR_TEXT, 1, 22, 94, "NOW PLAYING // LIVE AUDIO STREAM");
  else if (current == EricRadio::State::Connecting) text(COLOR_TEXT, 1, 22, 94, EricRadio::message());
  else if (current == EricRadio::State::Failed) text(COLOR_TEXT, 1, 22, 94, EricRadio::message());
  else if (WiFi.status() == WL_CONNECTED) text(COLOR_MUTED, 1, 22, 94, "SELECT A STATION // ENTER TO TUNE");
  else if (networkJoinPending) text(COLOR_TEXT, 1, 22, 94, "W.I.F.I. STILL CONNECTING // WAIT");
  else text(COLOR_TEXT, 1, 22, 94, "CONNECT W.I.F.I. BEFORE TUNING");
  display.drawRect(12, 124, 296, 150, COLOR_ACCENT_DARK);
  for (uint8_t i = 0; i < EricRadio::StationCount; ++i) drawRadioRow(i);
  drawFooter("UP/DOWN SELECT  ENTER TUNE/STOP  BACK HOME");
}

void handleRadioKey(uint8_t key) {
  if (key == KEY_BACK) { goBack(); return; }
  if (key == KEY_UP || key == KEY_JOY_UP) {
    const uint8_t previous = radioSelection;
    if (radioSelection > 0) --radioSelection;
    if (previous != radioSelection) drawRadioSelection(previous);
  } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
    const uint8_t previous = radioSelection;
    if (radioSelection + 1 < EricRadio::StationCount) ++radioSelection;
    if (previous != radioSelection) drawRadioSelection(previous);
  } else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
    const EricRadio::State current = EricRadio::state();
    if ((current == EricRadio::State::Connecting || current == EricRadio::State::Playing) && EricRadio::station() == radioSelection) EricRadio::stop();
    else EricRadio::play(radioSelection);
    lastRadioState = EricRadio::state();
    drawRadioWorkspace();
  }
}

#if 0 
void oldRadioWorkspace() {
  radioSelection = 0;
  switchApp(AppId::Radio);
}

void drawRadioWorkspace() {
  display.fillScreen(COLOR_BG);
  drawTopBar("R.A.D.I.O.", "");
  drawRoundedPanel(12, 66, 296, 48, COLOR_SURFACE, COLOR_ACCENT_DARK);
  ditherRect(18, 72, 284, 36, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 22, 76, "REALLY AWESOME DIGITAL INTERNET OUTPUT");

  const EricRadio::State radioState = EricRadio::state();
  if (radioState == EricRadio::State::Playing) text(COLOR_TEXT, 1, 22, 94, "NOW PLAYING // LIVE STREAM");
  else if (radioState == EricRadio::State::Connecting) text(COLOR_TEXT, 1, 22, 94, "TUNING... KEEP THIS SCREEN OPEN");
  else if (radioState == EricRadio::State::Failed) text(COLOR_TEXT, 1, 22, 94, "STREAM UNAVAILABLE // TRY AGAIN");
  else if (EricWiFi::state() != EricWiFi::State::Connected) text(COLOR_TEXT, 1, 22, 94, "CONNECT W.I.F.I. BEFORE TUNING");
  else text(COLOR_MUTED, 1, 22, 94, "SELECT A STATION // ENTER TO TUNE");

  display.drawRect(12, 124, 296, 150, COLOR_ACCENT_DARK);
  for (uint8_t i = 0; i < EricRadio::StationCount; ++i) {
    const int16_t y = 140 + i * 28;
    const bool selected = i == radioSelection;
    const bool active = (radioState == EricRadio::State::Connecting || radioState == EricRadio::State::Playing) && i == EricRadio::station();
    if (selected) display.fillRoundRect(18, y - 3, 284, 24, 3, COLOR_SURFACE_2);
    text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 26, y, selected ? ">" : " ");
    text(active ? COLOR_ACCENT : (selected ? COLOR_TEXT : COLOR_MUTED), 1, 42, y, EricRadio::stationName(i));
    text(selected ? COLOR_MUTED : COLOR_ACCENT_DARK, 1, 42, y + 11, EricRadio::stationGenre(i));
    if (active) text(COLOR_ACCENT, 1, 250, y, radioState == EricRadio::State::Playing ? "ON AIR" : "...");
  }
  drawFooter("UP/DOWN SELECT  ENTER TUNE/STOP  BACK HOME");
}



void drawRadioRow(uint8_t index) {
  if (index >= EricRadio::StationCount) return;
  const int16_t y = 140 + index * 28;
  const EricRadio::State radioState = EricRadio::state();
  const bool selected = index == radioSelection;
  const bool active = (radioState == EricRadio::State::Connecting || radioState == EricRadio::State::Playing) && index == EricRadio::station();
  display.fillRect(18, y - 3, 284, 24, COLOR_BLACK);
  if (selected) display.fillRoundRect(18, y - 3, 284, 24, 3, COLOR_SURFACE_2);
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 26, y, selected ? ">" : " ");
  text(active ? COLOR_ACCENT : (selected ? COLOR_TEXT : COLOR_MUTED), 1, 42, y, EricRadio::stationName(index));
  text(selected ? COLOR_MUTED : COLOR_ACCENT_DARK, 1, 42, y + 11, EricRadio::stationGenre(index));
  if (active) text(COLOR_ACCENT, 1, 250, y, radioState == EricRadio::State::Playing ? "ON AIR" : "...");
}

void drawRadioSelection(uint8_t previous) {
  drawRadioRow(previous);
  drawRadioRow(radioSelection);
}

void handleRadioKey(uint8_t key) {
  if (key == KEY_BACK) { goBack(); return; }
  if (key == KEY_UP || key == KEY_JOY_UP) {
    const uint8_t previous = radioSelection;
    if (radioSelection > 0) --radioSelection;
    if (radioSelection != previous) drawRadioSelection(previous);
  } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
    const uint8_t previous = radioSelection;
    if (radioSelection + 1 < EricRadio::StationCount) ++radioSelection;
    if (radioSelection != previous) drawRadioSelection(previous);
  } else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
    const EricRadio::State radioState = EricRadio::state();
    if ((radioState == EricRadio::State::Connecting || radioState == EricRadio::State::Playing) && EricRadio::station() == radioSelection) {
      EricRadio::stop();
    } else if (EricWiFi::state() == EricWiFi::State::Connected) {
      EricRadio::start(radioSelection);
    }
    
    
    drawRadioWorkspace();
  }
}

#endif



void setNetworkStatus(const char *message) {
  strncpy(networkStatus, message, sizeof(networkStatus) - 1);
  networkStatus[sizeof(networkStatus) - 1] = '\0';
}



void scanNearbyNetworks() {
  
  
  networkCount = 0;
  networkSelection = 0;
  setNetworkStatus("SCANNING...");
  const int found = WiFi.scanNetworks();
  if (found <= 0) {
    setNetworkStatus("NO NETWORKS FOUND");
  } else {
    const uint8_t keep = found < NETWORK_ITEMS ? found : NETWORK_ITEMS;
    for (uint8_t i = 0; i < keep; ++i) {
      
      strncpy(networkNames[i], WiFi.SSID(i), NETWORK_NAME_LEN);
      networkNames[i][NETWORK_NAME_LEN] = '\0';
      networkRssi[i] = WiFi.RSSI(i);
      networkSecure[i] = WiFi.encryptionType(i) != ENC_TYPE_NONE;
    }
    networkCount = keep;
    snprintf(networkStatus, sizeof(networkStatus), "%u NETWORKS FOUND", networkCount);
  }
  WiFi.scanDelete();
}

void openNetworkWorkspace() {
  networkMode = NetworkMode::Survey;
  networkPassword[0] = '\0';
  networkPasswordLength = 0;
  scanNearbyNetworks();
  switchApp(AppId::Network);
}

void drawNetworkRow(uint8_t index) {
  if (index >= networkCount) return;
  const int16_t y = 138 + index * 16;
  const bool selected = index == networkSelection;
  display.fillRect(18, y - 1, 284, 14, COLOR_BLACK);
  if (selected) display.fillRoundRect(18, y - 1, 284, 14, 3, COLOR_SURFACE_2);
  char label[29];
  clippedText(label, sizeof(label), networkNames[index], 20);
  char signal[14];
  snprintf(signal, sizeof(signal), "%ddBm%s", networkRssi[index], networkSecure[index] ? " *" : "");
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 26, y, selected ? ">" : " ");
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 40, y, label);
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 238, y, signal);
}

void drawNetworkSelection(uint8_t previous) {
  drawNetworkRow(previous);
  drawNetworkRow(networkSelection);
}

void drawNetworkPasswordInput() {
  display.fillRect(26, 178, 260, 18, COLOR_BLACK);
  const uint8_t shown = networkPasswordLength > 42 ? 42 : networkPasswordLength;
  char field[43];
  
  
  
  for (uint8_t i = 0; i < shown; ++i) field[i] = networkPasswordRevealed ? networkPassword[i] : '*';
  field[shown] = '\0';
  text(networkPasswordRevealed ? COLOR_ACCENT : COLOR_TEXT, 1, 26, 184, field);
  display.fillRect(26 + shown * 6, 183, 4, 9, COLOR_ACCENT);
}





constexpr int16_t TRANSFER_ADDR_Y = 136;
constexpr int16_t TRANSFER_VALUE_Y = 158;
constexpr int16_t TRANSFER_BAR_X = 22;
constexpr int16_t TRANSFER_BAR_Y = 176;
constexpr int16_t TRANSFER_BAR_W = 200;
constexpr int16_t TRANSFER_BAR_H = 14;



constexpr int16_t TRANSFER_PCT_X = 226;
constexpr int16_t TRANSFER_PCT_Y = 178;
constexpr int16_t TRANSFER_PCT_W = 76;
constexpr int16_t TRANSFER_STATUS_Y = 196;
constexpr int16_t TRANSFER_HINT_Y = 214;
constexpr int16_t TRANSFER_LINE_H = 12;

char transferStatusShown[56] = "";
char transferValueShown[40] = "";
char transferPctShown[8] = "";

int16_t transferBarShown = -1;




void formatBytes(char *out, size_t capacity, uint32_t bytes) {
  if (bytes < 1024UL) {
    snprintf(out, capacity, "%lu B", static_cast<unsigned long>(bytes));
  } else if (bytes < 1048576UL) {
    snprintf(out, capacity, "%lu.%01lu KB", static_cast<unsigned long>(bytes / 1024UL),
             static_cast<unsigned long>((bytes % 1024UL) * 10UL / 1024UL));
  } else {
    snprintf(out, capacity, "%lu.%02lu MB", static_cast<unsigned long>(bytes / 1048576UL),
             static_cast<unsigned long>((bytes % 1048576UL) * 100UL / 1048576UL));
  }
}



void repaintTransferLine(int16_t x, int16_t y, int16_t w, uint16_t color, const char *value) {
  display.fillRect(x, y, w, TRANSFER_LINE_H, COLOR_SURFACE);
  
  
  
  
  
  
  
  
  
  
  for (int16_t row = y; row < y + TRANSFER_LINE_H && row < 234; ++row) {
    if (row < 72 || row % 6 != 0) continue;
    for (int16_t col = 18 + ((row / 6) & 1 ? 3 : 0); col < 302; col += 6) {
      if (col < x || col >= x + w) continue;
      display.drawPixel(col, row, COLOR_ACCENT_DARK);
    }
  }
  text(color, 1, x, y + 2, value);
}




constexpr int16_t TRANSFER_STRIP_X = 22;
constexpr int16_t TRANSFER_STRIP_W = 280;




const char *transferValuePrefix() {
  switch (EricTransfer::direction()) {
    case EricTransfer::Direction::Incoming: return "RECEIVED";
    case EricTransfer::Direction::Outgoing: return "SENT TO PHONE";
    default: return "TRANSFERRED";
  }
}












void repaintTransferValue() {
  char count[24] = "";
  formatBytes(count, sizeof(count), EricTransfer::progressDone());
  char value[40] = "";
  snprintf(value, sizeof(value), "%s %s", transferValuePrefix(), count);
  if (strcmp(value, transferValueShown) == 0) return;
  repaintTransferLine(TRANSFER_STRIP_X, TRANSFER_VALUE_Y, TRANSFER_STRIP_W, COLOR_ACCENT, value);
  strncpy(transferValueShown, value, sizeof(transferValueShown) - 1);
  transferValueShown[sizeof(transferValueShown) - 1] = '\0';
}





void repaintTransferBar(int16_t percent) {
  if (percent < 0 || transferBarShown < 0 || percent < transferBarShown) {
    
    
    display.fillRect(TRANSFER_BAR_X + 1, TRANSFER_BAR_Y + 1, TRANSFER_BAR_W - 2,
                     TRANSFER_BAR_H - 2, COLOR_SURFACE);
    transferBarShown = percent < 0 ? -1 : 0;
  }
  if (percent >= 0) {
    const int16_t innerW = TRANSFER_BAR_W - 2;
    const int16_t doneW = static_cast<int16_t>((static_cast<int32_t>(innerW) * percent) / 100);
    if (doneW > 0) {
      display.fillRect(TRANSFER_BAR_X + 1 + transferBarShown, TRANSFER_BAR_Y + 1,
                       doneW - transferBarShown, TRANSFER_BAR_H - 2, COLOR_ACCENT);
      transferBarShown = doneW;
    }
  }
  
  
  
  char pct[8] = "";
  snprintf(pct, sizeof(pct), percent >= 0 ? "%d%%" : "--", percent);
  if (strcmp(pct, transferPctShown) != 0) {
    
    
    repaintTransferLine(TRANSFER_PCT_X, TRANSFER_PCT_Y, TRANSFER_PCT_W, COLOR_ACCENT, pct);
    strncpy(transferPctShown, pct, sizeof(transferPctShown) - 1);
    transferPctShown[sizeof(transferPctShown) - 1] = '\0';
  }
}




void updateTransferStatusLines() {
  char status[56];
  snprintf(status, sizeof(status), "%s", EricTransfer::message());
  if (strcmp(status, transferStatusShown) != 0) {
    repaintTransferLine(TRANSFER_STRIP_X, TRANSFER_STATUS_Y, TRANSFER_STRIP_W, COLOR_MUTED, status);
    strncpy(transferStatusShown, status, sizeof(transferStatusShown) - 1);
    transferStatusShown[sizeof(transferStatusShown) - 1] = '\0';
  }
  const uint32_t total = EricTransfer::progressTotal();
  const int16_t percent = total > 0
                              ? static_cast<int16_t>((static_cast<uint32_t>(EricTransfer::progressDone()) * 100UL) / total)
                              : -1;
  repaintTransferValue();
  repaintTransferBar(percent > 100 ? 100 : percent);
}

void drawNetworkTransfer() {
  display.fillScreen(COLOR_BG);
  drawTopBar("W.I.F.I.", "FILE TRANSFER");
  drawRoundedPanel(12, 66, 296, 174, COLOR_SURFACE, COLOR_ACCENT_DARK);
  ditherRect(18, 72, 284, 162, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 24, 78, "PHONE FILE TRANSFER");
  text(COLOR_TEXT, 1, 24, 96, "1. JOIN   ERIC-Files");
  text(COLOR_TEXT, 1, 24, 108, "2. PASSWORD   eosfiles");
  text(COLOR_TEXT, 1, 24, 120, "3. OPEN IN BROWSER");
  
  
  char address[26];
  snprintf(address, sizeof(address), "%s", EricTransfer::ipAddress());
  text(COLOR_ACCENT, 2, 24, TRANSFER_ADDR_Y, address);
  
  
  
  display.drawRect(TRANSFER_BAR_X, TRANSFER_BAR_Y, TRANSFER_BAR_W, TRANSFER_BAR_H, COLOR_ACCENT_DARK);
  text(COLOR_MUTED, 1, 24, TRANSFER_HINT_Y, "FILES LAND IN /ERIC/INBOX/");
  
  
  
  text(COLOR_MUTED, 1, 24, TRANSFER_HINT_Y + 12, "HOTSPOT STAYS ON AFTER BACK");
  
  
  
  drawFooter("BACK STOPS TRANSFER  ESC MENU");
  
  
  transferStatusShown[0] = '\0';
  transferValueShown[0] = '\0';
  transferPctShown[0] = '\0';
  transferBarShown = -1;
  updateTransferStatusLines();
}

void drawNetworkWorkspace() {
  if (networkMode == NetworkMode::Transfer) { drawNetworkTransfer(); return; }
  display.fillScreen(COLOR_BG);
  drawTopBar("W.I.F.I.", "");
  drawRoundedPanel(12, 66, 296, 48, COLOR_SURFACE, COLOR_ACCENT_DARK);
  text(COLOR_ACCENT, 1, 22, 76, "WACKY INTERNET FREQUENCING INVENTION");
  text(COLOR_MUTED, 1, 22, 94, networkStatus);
  display.drawRect(12, 124, 296, 150, COLOR_ACCENT_DARK);
  if (networkMode == NetworkMode::Password) {
    text(COLOR_TEXT, 1, 26, 142, networkNames[networkSelection]);
    text(COLOR_MUTED, 1, 26, 164, "NETWORK PASSWORD (RAM ONLY)");
    drawNetworkPasswordInput();
    text(COLOR_MUTED, 1, 26, 222, networkPasswordRevealed ? "ENTER CONNECT  DEL ERASE  F1 HIDE" : "ENTER CONNECT  DEL ERASE  F1 SHOW");
  } else if (networkMode == NetworkMode::Status) {
    if (WiFi.status() == WL_CONNECTED) {
      char line[40];
      snprintf(line, sizeof(line), "ONLINE: %s", WiFi.localIP().toString().c_str());
      text(COLOR_TEXT, 1, 26, 150, "CONNECTED");
      text(COLOR_TEXT, 1, 26, 170, line);
      text(COLOR_MUTED, 1, 26, 210, "ENTER DISCONNECT");
    } else {
      text(COLOR_TEXT, 1, 26, 150, networkStatus);
      text(COLOR_MUTED, 1, 26, 184, "ENTER SCAN AGAIN");
    }
  } else if (networkCount == 0) {
    text(COLOR_MUTED, 1, 26, 154, "No access points found.");
    text(COLOR_MUTED, 1, 26, 176, "ENTER scans again.");
  } else {
    for (uint8_t i = 0; i < networkCount; ++i) drawNetworkRow(i);
  }
  drawFooter(networkMode == NetworkMode::Survey ? "UP/DOWN CHOOSE  ENTER JOIN  F PHONE TRANSFER" : "BACK RETURN");
}

void startSelectedNetwork() {
  if (networkSelection >= networkCount) return;
  setNetworkStatus("CONNECTING...");
  WiFi.begin(networkNames[networkSelection], networkSecure[networkSelection] ? networkPassword : "");
  networkConnectStartedAt = millis();
  networkJoinPending = true;
  networkMode = NetworkMode::Status;
}





constexpr uint32_t NETWORK_JOIN_TIMEOUT_MS = 45000UL;

void serviceNetworkConnection() {
  if (!networkJoinPending) return;
  const uint8_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    networkJoinPending = false;
    networkConnectStartedAt = 0;
    setNetworkStatus("CONNECTED");
    
    
    
    if (activeApp == AppId::Network) drawNetworkWorkspace();
    else if (activeApp == AppId::Radio) drawRadioWorkspace();
    return;
  }
  
  
  
  const bool settled = status == WL_CONNECT_FAILED || status == WL_IDLE_STATUS;
  if (!settled) return;
  if (millis() - networkConnectStartedAt <= NETWORK_JOIN_TIMEOUT_MS) return;
  networkJoinPending = false;
  networkConnectStartedAt = 0;
  
  
  setNetworkStatus(status == WL_CONNECT_FAILED ? "PASSWORD OR NETWORK REJECTED" : "CONNECTION TIMED OUT");
  if (activeApp == AppId::Network) drawNetworkWorkspace();
}

void handleNetworkKey(uint8_t key) {
  if (networkMode == NetworkMode::Transfer) {
    if (key == KEY_BACK) {
      EricTransfer::stop();
      networkMode = NetworkMode::Survey;
      setNetworkStatus("PHONE TRANSFER STOPPED");
      drawNetworkWorkspace();
    }
    return;
  }
  if (networkMode == NetworkMode::Password) {
    if (key == KEY_BACK) { networkMode = NetworkMode::Survey; drawNetworkWorkspace(); }
    else if (key == KEY_F1) { networkPasswordRevealed = !networkPasswordRevealed; drawNetworkWorkspace(); }
    else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) { startSelectedNetwork(); drawNetworkWorkspace(); }
    else if ((key == KEY_DELETE || key == 0x7F) && networkPasswordLength) { networkPassword[--networkPasswordLength] = '\0'; drawNetworkPasswordInput(); }
    else if (key >= 32 && key <= 126 && networkPasswordLength < NETWORK_PASSWORD_LEN) { networkPassword[networkPasswordLength++] = static_cast<char>(key); networkPassword[networkPasswordLength] = '\0'; drawNetworkPasswordInput(); }
    return;
  }
  if (networkMode == NetworkMode::Status) {
    if (key == KEY_BACK) {
      
      networkMode = NetworkMode::Survey;
      drawNetworkWorkspace();
    } else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      if (WiFi.status() == WL_CONNECTED) {
        
        WiFi.disconnect();
        networkConnectStartedAt = 0;
        networkJoinPending = false;
        setNetworkStatus("DISCONNECTED");
        networkMode = NetworkMode::Survey;
      } else {
        
        networkMode = NetworkMode::Password;
      }
      drawNetworkWorkspace();
    }
    return;
  }
  if (key == KEY_BACK) { goBack(); return; }
  if (key == 'f' || key == 'F') {
    networkConnectStartedAt = 0;
    networkJoinPending = false;
    if (EricTransfer::start()) networkMode = NetworkMode::Transfer;
    else setNetworkStatus(EricTransfer::message());
    drawNetworkWorkspace();
    return;
  }
  if (key == KEY_UP || key == KEY_JOY_UP) { const uint8_t previous = networkSelection; if (networkSelection) --networkSelection; if (previous != networkSelection) drawNetworkSelection(previous); }
  else if (key == KEY_DOWN || key == KEY_JOY_DOWN) { const uint8_t previous = networkSelection; if (networkSelection + 1 < networkCount) ++networkSelection; if (previous != networkSelection) drawNetworkSelection(previous); }
  else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
    if (!networkCount) { scanNearbyNetworks(); drawNetworkWorkspace(); }
    else if (networkSecure[networkSelection]) { networkPassword[0] = '\0'; networkPasswordLength = 0; networkPasswordRevealed = false; networkMode = NetworkMode::Password; drawNetworkWorkspace(); }
    else { startSelectedNetwork(); drawNetworkWorkspace(); }
  }
}

#if 0

void openBlueExchangeWorkspace() {
  EricBlue::reset();
  blueMode = BlueMode::Status;
  blueAction = BlueAction::Power;
  blueTransferAction = BlueTransferAction::Receive;
  blueNotice[0] = '\0';
  switchApp(AppId::BlueExchange);
}

void drawBlueExchangeStatus() {
  display.fillRect(20, 116, 280, 118, COLOR_SURFACE);
  const EricBlueBt::State radio = EricBlueBt::state();
  const EricBlue::State transfer = EricBlue::state();
  text(COLOR_TEXT, 1, 22, 122, EricBlueBt::message());
  if (radio == EricBlueBt::State::Off) {
    text(COLOR_MUTED, 1, 22, 148, "DEL: BLUETOOTH CONTROLS");
    text(COLOR_MUTED, 1, 22, 166, "Bluetooth is off. Wi-Fi is untouched.");
    text(COLOR_MUTED, 1, 22, 184, "Turn it on before pairing or transfer.");
  } else if (transfer == EricBlue::State::Offer) {
    const EricBlue::Offer &item = EricBlue::offer();
    char name[34];
    clippedText(name, sizeof(name), item.name, 30);
    char size[40];
    snprintf(size, sizeof(size), "FILE: %s  %lu BYTES", name, static_cast<unsigned long>(item.declaredBytes));
    text(COLOR_TEXT, 1, 22, 148, size);
    text(COLOR_ACCENT, 1, 22, 174, "ENTER ACCEPT  BACK REJECT");
    text(COLOR_MUTED, 1, 22, 194, "Nothing is written until you accept.");
  } else if (transfer == EricBlue::State::Receiving) {
    const EricBlue::Offer &item = EricBlue::offer();
    char progress[44];
    snprintf(progress, sizeof(progress), "RECEIVED: %lu / %lu", static_cast<unsigned long>(item.receivedBytes), static_cast<unsigned long>(item.declaredBytes));
    text(COLOR_ACCENT, 1, 22, 154, "RECEIVING TO /ERIC/INBOX/");
    text(COLOR_TEXT, 1, 22, 176, progress);
    text(COLOR_MUTED, 1, 22, 202, "BACK CANCELS AND REMOVES PARTIAL FILE");
  } else if (transfer == EricBlue::State::Complete) {
    text(COLOR_ACCENT, 1, 22, 154, "FILE SAVED TO /ERIC/INBOX/");
    text(COLOR_MUTED, 1, 22, 180, "ENTER: RECEIVE ANOTHER FILE");
  } else if (radio == EricBlueBt::State::Ready) {
    text(COLOR_ACCENT, 1, 22, 148, "BLUETOOTH ON // E.R.I.C. VISIBLE");
    text(COLOR_MUTED, 1, 22, 170, "DEL: PAIR, RECEIVE, OR SEND");
    text(COLOR_MUTED, 1, 22, 192, "BACK STOPS BLUETOOTH.");
  } else {
    text(COLOR_MUTED, 1, 22, 150, EricBlueBt::message());
    text(COLOR_MUTED, 1, 22, 176, "DEL shows the available controls.");
  }
}

void drawBlueControls() {
  drawRoundedPanel(28, 72, 264, 182, COLOR_SURFACE, COLOR_ACCENT);
  ditherRect(34, 78, 252, 170, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 46, 88, "B.L.U.E. CONTROLS");
  for (uint8_t i = 0; i < 6; ++i) drawBlueControlRow(i);
  text(COLOR_MUTED, 1, 46, 230, "UP/DOWN CHOOSE  ENTER RUN  BACK CLOSE");
}



void drawBlueControlRow(uint8_t index) {
  if (index >= 6) return;
  const bool on = EricBlueBt::state() == EricBlueBt::State::Ready || EricBlueBt::state() == EricBlueBt::State::Connected;
  const char *items[] = {on ? "Turn Bluetooth off" : "Turn Bluetooth on", "Pair a phone", "Receive from phone", "Send a file", "How transfers work", "Close menu"};
  const int16_t y = 112 + index * 19;
  const bool selected = index == static_cast<uint8_t>(blueAction);
  display.fillRect(42, y - 2, 236, 16, COLOR_SURFACE);
  if (selected) display.fillRoundRect(42, y - 2, 236, 16, 3, COLOR_SURFACE_2);
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 50, y, selected ? ">" : " ");
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 66, y, items[index]);
}

void drawBlueControlSelection(uint8_t previous) {
  drawBlueControlRow(previous);
  drawBlueControlRow(static_cast<uint8_t>(blueAction));
}

void drawBlueNotice() {
  drawRoundedPanel(28, 96, 264, 122, COLOR_SURFACE, COLOR_ACCENT);
  ditherRect(34, 102, 252, 110, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 46, 112, "B.L.U.E.");
  char first[39];
  char second[39];
  clippedText(first, sizeof(first), blueNotice, 38);
  text(COLOR_TEXT, 1, 46, 140, first);
  text(COLOR_MUTED, 1, 46, 174, "ENTER OR BACK TO CONTINUE");
}

void drawBlueTransferMenu() {
  drawRoundedPanel(34, 90, 252, 138, COLOR_SURFACE, COLOR_ACCENT);
  text(COLOR_ACCENT, 1, 50, 106, "TRANSFER");
  for (uint8_t i = 0; i < 3; ++i) drawBlueTransferRow(i);
  text(COLOR_MUTED, 1, 50, 204, "ENTER CHOOSE  BACK CLOSE");
}

void drawBlueTransferRow(uint8_t index) {
  if (index >= 3) return;
  const char *items[] = {"Receive from Android", "Send a file to phone", "Cancel"};
  const int16_t y = 132 + index * 22;
  const bool selected = index == static_cast<uint8_t>(blueTransferAction);
  display.fillRect(46, y - 2, 222, 17, COLOR_SURFACE);
  if (selected) display.fillRoundRect(46, y - 2, 222, 17, 3, COLOR_SURFACE_2);
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 54, y, selected ? ">" : " ");
  text(selected ? COLOR_TEXT : COLOR_MUTED, 1, 70, y, items[index]);
}

void drawBlueTransferSelection(uint8_t previous) {
  drawBlueTransferRow(previous);
  drawBlueTransferRow(static_cast<uint8_t>(blueTransferAction));
}

void drawBluePairing() {
  drawRoundedPanel(28, 82, 264, 150, COLOR_SURFACE, COLOR_ACCENT);
  text(COLOR_ACCENT, 1, 46, 98, "PAIR A PHONE");
  if (EricBlueBt::hasConfirmation()) {
    char code[28];
    snprintf(code, sizeof(code), "CODE: %06lu", static_cast<unsigned long>(EricBlueBt::confirmationCode()));
    text(COLOR_TEXT, 2, 94, 122, code);
    text(COLOR_TEXT, 1, 46, 158, "Does your phone show the same code?");
    text(COLOR_ACCENT, 1, 46, 184, "ENTER APPROVE  BACK REJECT");
    text(COLOR_MUTED, 1, 46, 204, "Approve only when both codes match.");
  } else if (EricBlueBt::connected()) {
    text(COLOR_ACCENT, 1, 46, 136, "PHONE PAIRED FOR THIS SESSION");
    text(COLOR_MUTED, 1, 46, 166, "DEL: CHOOSE A TRANSFER ACTION");
    text(COLOR_MUTED, 1, 46, 198, "BACK RETURNS TO B.L.U.E. CONTROLS.");
  } else {
    text(COLOR_TEXT, 1, 46, 132, "On Android, open Bluetooth settings");
    text(COLOR_TEXT, 1, 46, 148, "and select E.R.I.C. Object Push.");
    text(COLOR_MUTED, 1, 46, 178, "Wait here for the confirmation code.");
    text(COLOR_MUTED, 1, 46, 204, "BACK CANCELS PAIRING.");
  }
}

void drawBlueExchangeWorkspace() {
  display.fillScreen(COLOR_BG);
  drawTopBar("B.L.U.E.", "");
  drawRoundedPanel(12, 66, 296, 174, COLOR_SURFACE, COLOR_ACCENT_DARK);
  ditherRect(18, 72, 284, 162, COLOR_ACCENT_DARK, 6);
  text(COLOR_ACCENT, 1, 22, 78, "BEAMING LOCAL USER EXCHANGE");
  text(COLOR_TEXT, 1, 22, 98, "ANDROID CLASSIC OBJECT PUSH");
  drawBlueExchangeStatus();
  if (blueMode == BlueMode::Controls) drawBlueControls();
  else if (blueMode == BlueMode::Pairing) drawBluePairing();
  else if (blueMode == BlueMode::Transfer) drawBlueTransferMenu();
  else if (blueMode == BlueMode::Notice) drawBlueNotice();
  drawFooter(blueMode == BlueMode::Status ? "DEL CONTROLS  BACK STOP + HOME  ESC MENU" : "BACK CLOSES THIS MENU");
}

void handleBlueExchangeKey(uint8_t key) {
  const bool enter = key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER;
  const EricBlue::State transfer = EricBlue::state();
  if (blueMode == BlueMode::Notice) {
    if (enter || key == KEY_BACK) { blueMode = BlueMode::Controls; drawBlueExchangeWorkspace(); }
    return;
  }
  if (blueMode == BlueMode::Pairing) {
    if (EricBlueBt::hasConfirmation()) {
      if (enter) EricBlueBt::confirmPairing();
      else if (key == KEY_BACK) EricBlueBt::rejectPairing();
      drawBlueExchangeWorkspace();
    } else if (key == KEY_BACK || enter) {
      blueMode = BlueMode::Controls;
      drawBlueExchangeWorkspace();
    }
    return;
  }
  if (blueMode == BlueMode::Transfer) {
    if (key == KEY_BACK) { blueMode = BlueMode::Controls; drawBlueExchangeWorkspace(); return; }
    if (key == KEY_UP || key == KEY_JOY_UP) {
      const uint8_t previous = static_cast<uint8_t>(blueTransferAction);
      blueTransferAction = static_cast<BlueTransferAction>((previous + 2) % 3);
      drawBlueTransferSelection(previous);
      return;
    } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
      const uint8_t previous = static_cast<uint8_t>(blueTransferAction);
      blueTransferAction = static_cast<BlueTransferAction>((previous + 1) % 3);
      drawBlueTransferSelection(previous);
      return;
    } else if (enter) {
      if (blueTransferAction == BlueTransferAction::Receive) {
        EricBlueBt::start();
        strncpy(blueNotice, EricBlueBt::state() == EricBlueBt::State::Ready ? "Use Android Share via Bluetooth." : "Object Push transport is not installed.", sizeof(blueNotice) - 1);
      } else if (blueTransferAction == BlueTransferAction::Send) {
        strncpy(blueNotice, "Sending files needs the Object Push client.", sizeof(blueNotice) - 1);
      } else { blueMode = BlueMode::Controls; drawBlueExchangeWorkspace(); return; }
      blueNotice[sizeof(blueNotice) - 1] = '\0';
      blueMode = BlueMode::Notice;
    }
    drawBlueExchangeWorkspace();
    return;
  }
  if (blueMode == BlueMode::Controls) {
    if (key == KEY_BACK) { blueMode = BlueMode::Status; drawBlueExchangeWorkspace(); return; }
    if (key == KEY_UP || key == KEY_JOY_UP) {
      const uint8_t previous = static_cast<uint8_t>(blueAction);
      blueAction = static_cast<BlueAction>((previous + 5) % 6);
      drawBlueControlSelection(previous);
      return;
    } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
      const uint8_t previous = static_cast<uint8_t>(blueAction);
      blueAction = static_cast<BlueAction>((previous + 1) % 6);
      drawBlueControlSelection(previous);
      return;
    } else if (enter) {
      if (blueAction == BlueAction::Power) {
        if (EricBlueBt::state() == EricBlueBt::State::Off) {
          
          
          if (WiFi.status() == WL_CONNECTED) WiFi.disconnect();
          EricBlueBt::start();
          if (EricBlueBt::state() != EricBlueBt::State::Ready) {
            strncpy(blueNotice, "Bluetooth driver is unavailable in this build.", sizeof(blueNotice) - 1);
            blueMode = BlueMode::Notice;
          }
        } else EricBlueBt::stop();
      } else if (blueAction == BlueAction::Pair) {
        if (EricBlueBt::state() == EricBlueBt::State::Ready || EricBlueBt::state() == EricBlueBt::State::Connected) {
          EricBlueBt::beginPairing();
          blueMode = BlueMode::Pairing;
        } else { strncpy(blueNotice, "Turn Bluetooth on before pairing.", sizeof(blueNotice) - 1); blueMode = BlueMode::Notice; }
      } else if (blueAction == BlueAction::Receive || blueAction == BlueAction::Send) {
        blueTransferAction = blueAction == BlueAction::Receive ? BlueTransferAction::Receive : BlueTransferAction::Send;
        blueMode = BlueMode::Transfer;
      } else if (blueAction == BlueAction::About) {
        strncpy(blueNotice, "Android uses Bluetooth Share for Object Push.", sizeof(blueNotice) - 1);
        blueMode = BlueMode::Notice;
      } else { blueMode = BlueMode::Status; }
      blueNotice[sizeof(blueNotice) - 1] = '\0';
    }
    drawBlueExchangeWorkspace();
    return;
  }
  if (transfer == EricBlue::State::Offer) {
    if (enter) EricBlueBt::acceptOffer();
    else if (key == KEY_BACK) EricBlueBt::rejectOffer();
    drawBlueExchangeWorkspace();
    return;
  }
  if (transfer == EricBlue::State::Receiving) {
    if (key == KEY_BACK) { EricBlueBt::stop(); drawBlueExchangeWorkspace(); }
    return;
  }
  if (key == KEY_DELETE || key == 0x7F) {
    blueMode = BlueMode::Controls;
    drawBlueExchangeWorkspace();
  } else if (enter) {
    if (transfer == EricBlue::State::Complete) EricBlue::reset();
    else { blueMode = BlueMode::Controls; }
    drawBlueExchangeWorkspace();
  } else if (key == KEY_BACK) {
    EricBlueBt::stop();
    goBack();
  }
}

#endif

void openSelectedTile() {
  if (selectedTile == HomeTile::Shell) switchApp(AppId::Shell);
  else if (selectedTile == HomeTile::Explorer) switchApp(AppId::Explorer);
  else if (selectedTile == HomeTile::AudioPlayer) {
    
    
    openSoundWorkspace();
  } else if (selectedTile == HomeTile::Radio) openRadioWorkspace();
  else if (selectedTile == HomeTile::Scratchpad) switchApp(AppId::Scratchpad);
  else if (selectedTile == HomeTile::GameBoy) openGameWorkspace();
  else if (selectedTile == HomeTile::Calculator) switchApp(AppId::Calculator);
  else if (selectedTile == HomeTile::Network) openNetworkWorkspace();
  else if (selectedTile == HomeTile::Weather) openWeatherWorkspace();
  else if (selectedTile == HomeTile::Tracks) switchApp(AppId::Tracks);
  else switchApp(AppId::SystemInfo);
}

void handleHomeKey(uint8_t key) {
  const uint8_t index = homeCarouselIndex(selectedTile);
  if (key == KEY_LEFT || key == KEY_JOY_LEFT || key == KEY_UP || key == KEY_JOY_UP) {
    uint8_t newIndex = (index == 0) ? (HOME_TILE_COUNT - 1) : (index - 1);
    selectedTile = HOME_CAROUSEL_ORDER[newIndex];
    drawHomeSelection();
  } else if (key == KEY_RIGHT || key == KEY_JOY_RIGHT || key == KEY_DOWN || key == KEY_JOY_DOWN) {
    uint8_t newIndex = (index + 1) % HOME_TILE_COUNT;
    selectedTile = HOME_CAROUSEL_ORDER[newIndex];
    drawHomeSelection();
  } else if (key == KEY_ENTER || key == 0x0D || key == ' ' || key == KEY_JOY_CENTER) {
    openSelectedTile();
  }
}

void switchApp(AppId next) {
  
  
  if (activeApp == AppId::AudioPlayer && next != AppId::AudioPlayer) EricAudio::stop();
  if (activeApp == AppId::Radio && next != AppId::Radio) EricRadio::stop();
  if (activeApp == AppId::Network && next != AppId::Network && networkMode == NetworkMode::Transfer) {
    EricTransfer::stop();
    networkMode = NetworkMode::Survey;
  }
  if (activeApp == AppId::GameBoy && next != AppId::GameBoy) EricGameBoy::stop();
  
  
  if (activeApp == AppId::Weather && next != AppId::Weather) EricWeather::abort();
  
  if (next != activeApp) previousApp = activeApp;
  activeApp = next;
  if (next == AppId::Home) drawHome();
  else if (next == AppId::Shell) drawShell();
  else if (next == AppId::SystemInfo) drawSystemInfo();
  else if (next == AppId::Scratchpad) drawScratchpad();
  else if (next == AppId::Explorer) drawExplorer();
  else if (next == AppId::Tracks) drawTracksWorkspace();
  else if (next == AppId::ImageViewer) {
    if (EricBmp::isOpen()) drawBmpViewer();
    else drawJpegViewer();
  }
  else if (next == AppId::AudioPlayer) drawAudioPlayer();
  else if (next == AppId::Radio) drawRadioWorkspace();
  else if (next == AppId::GameBoy) drawGameBrowser();
  else if (next == AppId::Network) drawNetworkWorkspace();
  else if (next == AppId::Calculator) drawCalculator();
  else if (next == AppId::Weather) drawWeatherWorkspace();
  
}

void goBack() {
  if (activeApp == AppId::Home) return;
  const AppId destination = previousApp;
  if (activeApp == AppId::Scratchpad && noteDirty) {
    requestScratchpadExit(destination);
    return;
  }
  previousApp = activeApp;
  switchApp(destination);
}










constexpr uint8_t WEATHER_QUERY_LEN = 47;
constexpr char WEATHER_CONFIG_PATH[] = "/ERIC/WEATHER.CFG";

WeatherMode weatherMode = WeatherMode::Now;
char weatherQuery[WEATHER_QUERY_LEN + 1] = "";
uint8_t weatherQueryLength = 0;
uint8_t weatherResultIndex = 0;
WeatherPlace weatherPlace = {};
bool weatherHasPlace = false;
bool weatherHasReport = false;
char weatherStatus[44] = "CHOOSE A CITY";










bool weatherTakeResult(bool finished, bool *handled) {
  if (!finished) {
    *handled = false;
    return false;
  }
  if (*handled) return false;
  *handled = true;
  return true;
}


bool weatherFetchHandled = true;


WeatherPlace weatherResults[WEATHER_MAX_PLACES] = {};
uint8_t weatherResultCount = 0;




void weatherTenths(char *out, size_t outSize, int16_t tenths, char unit) {
  const int magnitude = tenths < 0 ? -static_cast<int>(tenths) : static_cast<int>(tenths);
  const char *sign = tenths < 0 ? "-" : "";
  if (unit == '\0') snprintf(out, outSize, "%s%d.%d", sign, magnitude / 10, magnitude % 10);
  else snprintf(out, outSize, "%s%d.%d%c", sign, magnitude / 10, magnitude % 10, unit);
}



void weatherWholeDegrees(char *out, size_t outSize, int16_t tenths) {
  const int magnitude = tenths < 0 ? -static_cast<int>(tenths) : static_cast<int>(tenths);
  
  
  const int degrees = (magnitude + 5) / 10;
  snprintf(out, outSize, "%s%d", tenths < 0 && degrees != 0 ? "-" : "", degrees);
}

void weatherConfigBuffer(char *out, size_t outSize) {
  snprintf(out, outSize,
           "NAME=%s\nREGION=%s\nCOUNTRY=%s\nLAT=%.4f\nLON=%.4f\n", weatherPlace.name,
           weatherPlace.region, weatherPlace.country,
           static_cast<double>(weatherPlace.latitude),
           static_cast<double>(weatherPlace.longitude));
}



void saveWeatherPlace() {
  if (!sdReady) return;
  char blob[288];
  weatherConfigBuffer(blob, sizeof(blob));
  EricStorage::saveText(WEATHER_CONFIG_PATH, blob, strlen(blob));
}

void loadWeatherPlace() {
  if (!sdReady) return;
  char blob[288];
  size_t length = 0;
  if (!EricStorage::loadText(WEATHER_CONFIG_PATH, blob, sizeof(blob) - 1, length)) return;
  blob[length] = '\0';
  float latitude = 0.0f, longitude = 0.0f;
  bool sawLatitude = false, sawLongitude = false;
  char *line = blob;
  while (line != nullptr && *line != '\0') {
    char *end = strchr(line, '\n');
    if (end != nullptr) *end = '\0';
    if (strncmp(line, "NAME=", 5) == 0) snprintf(weatherPlace.name, sizeof(weatherPlace.name), "%s", line + 5);
    else if (strncmp(line, "REGION=", 7) == 0) snprintf(weatherPlace.region, sizeof(weatherPlace.region), "%s", line + 7);
    else if (strncmp(line, "COUNTRY=", 8) == 0) snprintf(weatherPlace.country, sizeof(weatherPlace.country), "%s", line + 8);
    else if (strncmp(line, "LAT=", 4) == 0) { latitude = atof(line + 4); sawLatitude = true; }
    else if (strncmp(line, "LON=", 4) == 0) { longitude = atof(line + 4); sawLongitude = true; }
    line = end != nullptr ? end + 1 : nullptr;
  }
  
  
  if (!sawLatitude || !sawLongitude || weatherPlace.name[0] == '\0') return;
  weatherPlace.latitude = latitude;
  weatherPlace.longitude = longitude;
  weatherHasPlace = true;
}

void drawWeatherFrame(const char *title, const char *subtitle) {
  display.fillScreen(COLOR_BG);
  drawTopBar(title, subtitle);
  
  
  
  
  
  
  
}









constexpr int16_t WEATHER_FIELD_X = SAFE_LEFT + 2;
constexpr int16_t WEATHER_FIELD_Y = 92;
constexpr int16_t WEATHER_FIELD_W = SAFE_WIDTH - 4;
constexpr int16_t WEATHER_FIELD_H = 36;


constexpr uint8_t WEATHER_FIELD_COLUMNS = 22;





uint8_t weatherFieldWindowStart(uint8_t queryLength, uint8_t columns) {
  return queryLength > columns ? static_cast<uint8_t>(queryLength - columns) : 0;
}

void drawWeatherSearchField() {
  
  
  
  display.fillRect(WEATHER_FIELD_X, WEATHER_FIELD_Y, WEATHER_FIELD_W, WEATHER_FIELD_H, COLOR_BG);
  drawRoundedPanel(WEATHER_FIELD_X, WEATHER_FIELD_Y, WEATHER_FIELD_W, WEATHER_FIELD_H,
                   COLOR_SURFACE, COLOR_ACCENT_DARK);

  char shown[WEATHER_QUERY_LEN + 1];
  const uint8_t start = weatherFieldWindowStart(weatherQueryLength, WEATHER_FIELD_COLUMNS);
  clippedText(shown, sizeof(shown), weatherQuery + start, WEATHER_FIELD_COLUMNS);
  text(COLOR_TEXT, 2, WEATHER_FIELD_X + 8, WEATHER_FIELD_Y + 10, shown);
  
  const int16_t cursorX = WEATHER_FIELD_X + 8 + static_cast<int16_t>(strlen(shown) * 12);
  display.fillRect(cursorX, WEATHER_FIELD_Y + 10, 10, 16, COLOR_ACCENT);
}





constexpr int16_t WEATHER_STATUS_Y = 148;
char weatherStatusShown[44] = "";



constexpr int16_t WEATHER_ROW_Y = 78;
constexpr int16_t WEATHER_ROW_PITCH = 40;







void drawWeatherStatusStrip() {
  display.fillRect(SAFE_LEFT + 2, WEATHER_STATUS_Y - 4, SAFE_WIDTH - 4, 42, COLOR_BG);
  if (EricWeather::busy()) {
    
    
    text(COLOR_ACCENT, 1, SAFE_LEFT + 4, WEATHER_STATUS_Y,
         weatherMode == WeatherMode::Search ? "LOOKING UP CITIES"
                                            : "CONTACTING WEATHER SERVICE");
    text(COLOR_MUTED, 1, SAFE_LEFT + 4, WEATHER_STATUS_Y + 18, EricWeather::message());
  } else if (weatherStatus[0] != '\0') {
    text(COLOR_MUTED, 1, SAFE_LEFT + 4, WEATHER_STATUS_Y, weatherStatus);
  }
  snprintf(weatherStatusShown, sizeof(weatherStatusShown), "%s",
           EricWeather::busy() ? EricWeather::message() : weatherStatus);
}

void drawWeatherSearch() {
  
  
  
  drawWeatherFrame("S.T.O.R.M.", "");
  
  
  
  text(COLOR_ACCENT, 2, SAFE_LEFT + 4, 60, "CHOOSE A CITY");
  drawWeatherSearchField();
  drawWeatherStatusStrip();
  text(COLOR_MUTED, 1, SAFE_LEFT + 4, 202, "TYPE A CITY NAME");
  text(COLOR_MUTED, 1, SAFE_LEFT + 4, 220, "NO COORDINATES NEEDED");
  drawFooter("ENTER SEARCH   DEL ERASE   BACK DASHBOARD");
}














void drawWeatherResultRow(uint8_t index) {
  if (index >= weatherResultCount) return;
  const int16_t y = WEATHER_ROW_Y + static_cast<int16_t>(index) * WEATHER_ROW_PITCH;
  const bool focused = index == weatherResultIndex;
  display.fillRect(SAFE_LEFT + 2, y, SAFE_WIDTH - 4, 36, COLOR_BG);
  if (focused) display.fillRect(SAFE_LEFT + 2, y, SAFE_WIDTH - 4, 36, COLOR_SURFACE);
  display.drawFastHLine(SAFE_LEFT + 2, y + 36, SAFE_WIDTH - 4, COLOR_ACCENT_DARK);
  char name[28], place[48];
  clippedText(name, sizeof(name), weatherResults[index].name, 27);
  
  
  snprintf(place, sizeof(place), "%s%s%s", weatherResults[index].region,
           weatherResults[index].country[0] != '\0' ? ", " : "",
           weatherResults[index].country);
  char placeClipped[48];
  clippedText(placeClipped, sizeof(placeClipped), place, 46);
  text(focused ? COLOR_TEXT : COLOR_MUTED, 2, SAFE_LEFT + 10, y + 4, name);
  text(COLOR_MUTED, 1, SAFE_LEFT + 10, y + 24, placeClipped);
}

void drawWeatherResultSelection(uint8_t previous) {
  drawWeatherResultRow(previous);
  drawWeatherResultRow(static_cast<uint8_t>(weatherResultIndex));
}

void drawWeatherResults() {
  char subtitle[20];
  snprintf(subtitle, sizeof(subtitle), "%u FOUND", weatherResultCount);
  drawWeatherFrame("S.T.O.R.M.", subtitle);
  text(COLOR_ACCENT, 1, SAFE_LEFT + 4, 58, "PICK A CITY");
  if (weatherResultCount == 0) {
    text(COLOR_MUTED, 1, SAFE_LEFT + 4, 96, weatherStatus);
    drawFooter("BACK TYPE A NEW CITY");
    return;
  }
  for (uint8_t i = 0; i < weatherResultCount; ++i) drawWeatherResultRow(i);
  drawFooter("UP/DOWN CHOOSE   ENTER SAVE   BACK SEARCH");
}

void drawWeatherTile(uint8_t index, const WeatherDay &day, bool today) {
  constexpr int16_t tileWidth = 41;
  constexpr int16_t tileGap = 2;
  const int16_t x = SAFE_LEFT + 2 + static_cast<int16_t>(index) * (tileWidth + tileGap);
  constexpr int16_t y = 176;
  constexpr int16_t h = 98;
  display.fillRect(x, y, tileWidth, h, COLOR_SURFACE);
  
  
  display.drawFastHLine(x, y, tileWidth, today ? COLOR_TEXT : COLOR_ACCENT_DARK);
  display.drawFastHLine(x, y + h - 1, tileWidth, COLOR_ACCENT_DARK);

  char head[8];
  if (today) snprintf(head, sizeof(head), "%s %u", weatherMonthShort(day.date), static_cast<uint8_t>(day.date[8] - '0') * 10 + static_cast<uint8_t>(day.date[9] - '0'));
  else snprintf(head, sizeof(head), "%s", weatherWeekday(day.date));
  char headClipped[8];
  clippedText(headClipped, sizeof(headClipped), head, 6);
  text(COLOR_TEXT, 1, x + 3, y + 6, headClipped);

  char tag[6];
  clippedText(tag, sizeof(tag), weatherCodeShort(day.code), 4);
  text(COLOR_MUTED, 1, x + 3, y + 20, tag);

  char high[6], low[6];
  weatherWholeDegrees(high, sizeof(high), day.highF);
  weatherWholeDegrees(low, sizeof(low), day.lowF);
  text(COLOR_TEXT, 2, x + 3, y + 40, high);
  text(COLOR_MUTED, 1, x + 3, y + 62, low);

  
  
  if (day.precipitationChance != 255 && day.precipitationChance > 0) {
    const uint8_t chance = day.precipitationChance > 100 ? 100 : day.precipitationChance;
    const int16_t barWidth = static_cast<int16_t>((chance * (tileWidth - 6)) / 100);
    display.fillRect(x + 3, y + h - 18, tileWidth - 6, 3, COLOR_SURFACE_2);
    display.fillRect(x + 3, y + h - 18, barWidth, 3, COLOR_ACCENT);
    char label[5];
    snprintf(label, sizeof(label), "%u%%", chance);
    text(COLOR_MUTED, 1, x + 3, y + h - 12, label);
  }
}

void drawWeatherNow() {
  if (!weatherHasPlace) {
    drawWeatherFrame("S.T.O.R.M.", "NO CITY");
    text(COLOR_MUTED, 1, SAFE_LEFT + 4, 100, "NO CITY CHOSEN YET");
    text(COLOR_MUTED, 1, SAFE_LEFT + 4, 118, "PRESS ENTER TO SEARCH");
    drawFooter("ENTER SEARCH   BACK DASHBOARD");
    return;
  }

  
  
  
  
  drawWeatherFrame("S.T.O.R.M.", "");

  
  
  
  if (EricWeather::busy() || !weatherHasReport) {
    drawWeatherStatusStrip();
    if (!EricWeather::busy()) {
      text(COLOR_MUTED, 1, SAFE_LEFT + 4, WEATHER_STATUS_Y + 18, "F1 TRIES AGAIN");
      drawFooter("ENTER NEW CITY   F1 RETRY   BACK DASHBOARD");
    } else {
      drawFooter("PLEASE WAIT");
    }
    return;
  }

  const WeatherReport &report = EricWeather::report();
  
  
  drawRoundedPanel(SAFE_LEFT + 2, 58, 150, 108, COLOR_SURFACE, COLOR_ACCENT_DARK);
  char now[8];
  weatherTenths(now, sizeof(now), report.now.temperatureF, 'F');
  text(COLOR_TEXT, 3, SAFE_LEFT + 12, 68, now);
  char label[30];
  clippedText(label, sizeof(label), weatherCodeLabel(report.now.code), 21);
  text(COLOR_ACCENT, 1, SAFE_LEFT + 12, 108, label);
  char line[20];
  if (report.now.feelsLikeF != report.now.temperatureF) {
    weatherTenths(line, sizeof(line), report.now.feelsLikeF, 'F');
    char feels[20];
    snprintf(feels, sizeof(feels), "FEELS %s", line);
    text(COLOR_MUTED, 1, SAFE_LEFT + 12, 126, feels);
  }
  snprintf(line, sizeof(line), "HUMIDITY %u%%", report.now.humidity);
  text(COLOR_MUTED, 1, SAFE_LEFT + 12, 144, line);

  
  
  char stamp[30];
  snprintf(stamp, sizeof(stamp), "%s %s", report.now.observedAt, report.now.timezone);
  char stampClipped[30];
  clippedText(stampClipped, sizeof(stampClipped), stamp, 28);
  text(COLOR_MUTED, 1, 166, 64, stampClipped);
  text(COLOR_MUTED, 1, 166, 82, weatherCodeShort(report.now.code));
  snprintf(line, sizeof(line), "WIND %u MPH", report.now.windMph);
  text(COLOR_MUTED, 1, 166, 100, line);
  char offset[16];
  weatherUtcOffsetLabel(offset, sizeof(offset), report.place.utcOffsetSeconds);
  text(COLOR_MUTED, 1, 166, 118, offset);
  
  
  
  
  
  
  
  
  
  
  
  char placeLine[32];
  weatherPlaceLine(placeLine, sizeof(placeLine), weatherPlace, 24);
  text(COLOR_MUTED, 1, 166, 136, placeLine);

  text(COLOR_ACCENT, 1, SAFE_LEFT + 4, 170, "NEXT 7 DAYS");
  for (uint8_t i = 0; i < report.dayCount && i < WEATHER_DAYS; ++i) {
    drawWeatherTile(i, report.days[i], i == 0);
  }
  drawFooter("ENTER NEW CITY   F1 REFRESH   BACK DASHBOARD");
}

void drawWeatherWorkspace() {
  switch (weatherMode) {
    case WeatherMode::Search: drawWeatherSearch(); break;
    case WeatherMode::Results: drawWeatherResults(); break;
    case WeatherMode::Now: drawWeatherNow(); break;
  }
}










bool startWeatherFetch() {
  weatherStatus[0] = '\0';
  weatherHasReport = false;
  
  
  
  
  
  
  
  
  
  weatherFetchHandled = false;
  EricWeather::fetchForecast(weatherPlace);
  if (EricWeather::busy()) return true;
  snprintf(weatherStatus, sizeof(weatherStatus), "FORECAST DID NOT START");
  weatherMode = WeatherMode::Now;
  return false;
}

void openWeatherWorkspace() {
  loadWeatherPlace();
  weatherQuery[0] = '\0';
  weatherQueryLength = 0;
  weatherResultCount = 0;
  weatherResultIndex = 0;
  if (weatherHasPlace) {
    weatherMode = WeatherMode::Now;
    startWeatherFetch();
  } else {
    weatherMode = WeatherMode::Search;
    snprintf(weatherStatus, sizeof(weatherStatus), "CHOOSE A CITY");
  }
  
  
  
  
  switchApp(AppId::Weather);
}




void serviceWeather() {
  if (activeApp != AppId::Weather) return;
  EricWeather::pump();

  
  
  
  
  
  
  if (EricWeather::busy() && strcmp(EricWeather::message(), weatherStatusShown) != 0) {
    drawWeatherStatusStrip();
  }

  if (!weatherTakeResult(EricWeather::finished(), &weatherFetchHandled)) return;

  if (EricWeather::state() == EricWeather::FetchState::Failed) {
    snprintf(weatherStatus, sizeof(weatherStatus), "%s", EricWeather::message());
    weatherResultCount = 0;
    weatherHasReport = false;
    weatherMode = weatherMode == WeatherMode::Now ? WeatherMode::Now : WeatherMode::Search;
    drawWeatherWorkspace();
    return;
  }

  const WeatherParse parsed = EricWeather::parseResult();
  if (weatherMode == WeatherMode::Now) {
    if (parsed == WeatherParse::Ok) {
      weatherHasReport = true;
    } else {
      
      
      weatherHasReport = false;
      snprintf(weatherStatus, sizeof(weatherStatus), "%s", weatherParseMessage(parsed));
    }
    drawWeatherNow();
    return;
  }

  if (parsed == WeatherParse::NoResults) {
    weatherResultCount = 0;
    snprintf(weatherStatus, sizeof(weatherStatus), "NO MATCHING CITY");
    weatherMode = WeatherMode::Search;
    drawWeatherSearch();
    return;
  }
  if (parsed != WeatherParse::Ok) {
    weatherResultCount = 0;
    snprintf(weatherStatus, sizeof(weatherStatus), "%s", weatherParseMessage(parsed));
    drawWeatherSearch();
    return;
  }

  weatherResultCount = EricWeather::placeCount();
  for (uint8_t i = 0; i < weatherResultCount; ++i) weatherResults[i] = EricWeather::places()[i];
  weatherResultIndex = 0;
  weatherMode = WeatherMode::Results;
  drawWeatherResults();
}

void handleWeatherKey(uint8_t key) {
  if (weatherMode == WeatherMode::Search) {
    if (key == KEY_BACK) { switchApp(AppId::Home); return; }
    if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      if (weatherQueryLength == 0) {
        snprintf(weatherStatus, sizeof(weatherStatus), "TYPE A CITY NAME FIRST");
        drawWeatherStatusStrip();
        return;
      }
      weatherResultCount = 0;
      weatherStatus[0] = '\0';
      weatherFetchHandled = false;
      EricWeather::search(weatherQuery);
      
      
      
      drawWeatherStatusStrip();
      return;
    }
    if ((key == KEY_DELETE || key == 0x7F) && weatherQueryLength) {
      weatherQuery[--weatherQueryLength] = '\0';
      
      
      
      drawWeatherSearchField();
      drawWeatherStatusStrip();
      return;
    }
    if (key >= 32 && key <= 126 && weatherQueryLength < WEATHER_QUERY_LEN) {
      weatherQuery[weatherQueryLength++] = static_cast<char>(key);
      weatherQuery[weatherQueryLength] = '\0';
      drawWeatherSearchField();
      drawWeatherStatusStrip();
    }
    return;
  }

  if (weatherMode == WeatherMode::Results) {
    if (key == KEY_BACK) { weatherMode = WeatherMode::Search; drawWeatherSearch(); return; }
    if (weatherResultCount == 0) return;
    if (key == KEY_UP || key == KEY_JOY_UP) {
      const uint8_t previous = static_cast<uint8_t>(weatherResultIndex);
      weatherResultIndex = static_cast<uint8_t>((previous + weatherResultCount - 1) % weatherResultCount);
      drawWeatherResultSelection(previous);
      return;
    }
    if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
      const uint8_t previous = static_cast<uint8_t>(weatherResultIndex);
      weatherResultIndex = static_cast<uint8_t>((previous + 1) % weatherResultCount);
      drawWeatherResultSelection(previous);
      return;
    }
    if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      weatherPlace = weatherResults[weatherResultIndex];
      weatherHasPlace = true;
      saveWeatherPlace();
      weatherMode = WeatherMode::Now;
      startWeatherFetch();
      drawWeatherWorkspace();
    }
    return;
  }

  if (key == KEY_BACK) { switchApp(AppId::Home); return; }
  if (key == KEY_F1) {
    if (weatherHasPlace) startWeatherFetch();
    drawWeatherWorkspace();
    return;
  }
  if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
    weatherMode = WeatherMode::Search;
    weatherQuery[0] = '\0';
    weatherQueryLength = 0;
    snprintf(weatherStatus, sizeof(weatherStatus), "CHOOSE A CITY");
    drawWeatherSearch();
  }
}


constexpr uint8_t BROWSER_URL_LEN = 95;
constexpr size_t BROWSER_PAGE_CAP = 4096;
enum class BrowserMode { Search, Loading, Viewing };
BrowserMode browserMode = BrowserMode::Search;
char browserUrl[BROWSER_URL_LEN + 1] = "";
uint8_t browserUrlLength = 0;
char browserStatus[44] = "ENTER A URL";
char browserPage[BROWSER_PAGE_CAP] = "";
size_t browserPageLen = 0;
WiFiClient browserClient;
uint32_t browserStartTime = 0;

void drawBrowserWorkspace() {
  display.fillScreen(COLOR_BG);
  drawTopBar("W.E.B", "BROWSER");
  
  if (browserMode == BrowserMode::Search) {
    const int centerY = 150;
    text(COLOR_ACCENT, 1, 60, centerY - 40, "ENTER URL OR SEARCH");
    drawRoundedPanel(16, centerY - 18, 288, 36, COLOR_SURFACE, COLOR_ACCENT_DARK);
    char bar[24] = {};
    uint8_t start = 0;
    if (browserUrlLength > 22) start = browserUrlLength - 22;
    for (uint8_t i = 0; i < 22 && (start + i) < browserUrlLength; ++i) bar[i] = browserUrl[start + i];
    bar[23] = '\0';
    text(COLOR_TEXT, 1, 26, centerY - 8, bar);
    const int cursorX = 26 + ((browserUrlLength - start) * 6);
    if ((millis() / 500) % 2 == 0) display.fillRect(cursorX, centerY - 12, 6, 24, COLOR_ACCENT);
    text(COLOR_MUTED, 1, 16, centerY + 40, browserStatus);
    text(COLOR_MUTED, 1, 16, 290, "ENTER GO  DEL ERASE  BACK HOME");
  } else if (browserMode == BrowserMode::Loading) {
    text(COLOR_ACCENT, 1, 90, 150, "LOADING...");
    text(COLOR_MUTED, 1, 16, 290, "BACK CANCEL");
  } else if (browserMode == BrowserMode::Viewing) {
    
    display.fillRect(16, 90, 288, 192, COLOR_BG);
    
    int16_t y = 90;
    size_t i = 0;
    
    if (strncmp(browserPage, "HTTP", 4) == 0) {
      while (i < browserPageLen - 3 && !(browserPage[i] == '\r' && browserPage[i+1] == '\n' && browserPage[i+2] == '\r' && browserPage[i+3] == '\n')) i++;
      if (i < browserPageLen - 3) i += 4;
    }
    while (i < browserPageLen && y < 280) {
      if (browserPage[i] == '<') {
        while (i < browserPageLen && browserPage[i] != '>') i++;
        if (i < browserPageLen) i++;
        continue;
      }
      if (browserPage[i] == '\\n') {
        y += 12;
        i++;
        continue;
      }
      if (browserPage[i] == '\\r') {
        if (i + 1 < browserPageLen && browserPage[i+1] == '\\n') i++;
        y += 12;
        i++;
        continue;
      }
      if (browserPage[i] == '\\t') {
        i++;
        continue;
      }
      if (browserPage[i] >= 32 && browserPage[i] <= 126) {
        display.setTextColor(COLOR_TEXT);
        display.setTextSize(1);
        display.setCursor(16, y);
        char c[2] = {browserPage[i], '\0'};
        display.print(c);
      }
      i++;
    }
    text(COLOR_MUTED, 1, 16, 290, "BACK HOME");
  }
}

void startBrowserFetch() {
  if (browserUrlLength == 0) return;
  browserMode = BrowserMode::Loading;
  browserPageLen = 0;
  browserPage[0] = '\0';
  snprintf(browserStatus, sizeof(browserStatus), "CONNECTING...");
  drawBrowserWorkspace();
  
  char url[128] = {0};
  if (strncmp(browserUrl, "http://", 7) == 0 || strncmp(browserUrl, "https://", 8) == 0) {
    strncpy(url, browserUrl, 127);
    url[127] = '\0';
  } else {
    strcpy(url, "http://");
    strncat(url, browserUrl, 120);
  }
  
  char host[64] = "";
  const char *p = strstr(url, "://");
  if (p) p += 3;
  else p = url;
  int hi = 0;
  while (*p && *p != '/' && *p != ':' && hi < 63) {
    if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '.' || *p == '-') {
      host[hi++] = *p;
    } else break;
    p++;
  }
  host[hi] = '\0';
  
  snprintf(browserStatus, sizeof(browserStatus), "CONNECTING %s", host);
  drawBrowserWorkspace();
  
  if (WiFi.status() == WL_CONNECTED && browserClient.connect(host, 80)) {
    snprintf(browserStatus, sizeof(browserStatus), "GETTING...");
    drawBrowserWorkspace();
    browserClient.print("GET ");
    const char *pathStart = strchr(strstr(url, "://") ? strstr(url, "://") + 3 : url, '/');
    if (pathStart) browserClient.print(pathStart);
    else browserClient.print("/");
    browserClient.print(" HTTP/1.0\r\nHost: ");
    browserClient.print(host);
    browserClient.print("\r\nUser-Agent: E.R.I.C-Pico/1.0\r\nConnection: close\r\n\r\n");
    browserStartTime = millis();
    browserMode = BrowserMode::Loading;
  } else {
    snprintf(browserStatus, sizeof(browserStatus), "CONNECTION FAILED");
    browserMode = BrowserMode::Search;
    drawBrowserWorkspace();
  }
}

void serviceBrowser() {
  if (browserMode == BrowserMode::Loading) {
    if (browserClient.connected()) {
      while (browserClient.available() && browserPageLen < BROWSER_PAGE_CAP - 1) {
        int c = browserClient.read();
        if (c < 0) break;
        browserPage[browserPageLen++] = (char)c;
      }
      browserPage[browserPageLen] = '\0';
      if (millis() - browserStartTime > 8000) {
        browserClient.stop();
        browserMode = BrowserMode::Viewing;
        drawBrowserWorkspace();
      }
    } else {
      while (browserClient.available() && browserPageLen < BROWSER_PAGE_CAP - 1) {
        int c = browserClient.read();
        if (c < 0) break;
        browserPage[browserPageLen++] = (char)c;
      }
      browserPage[browserPageLen] = '\0';
      if (browserPageLen > 0 || millis() - browserStartTime > 2000) {
        browserClient.stop();
        browserMode = BrowserMode::Viewing;
        drawBrowserWorkspace();
      }
    }
  }
}

void handleBrowserKey(uint8_t key) {
  if (key == KEY_BACK) {
    if (browserMode == BrowserMode::Viewing || browserMode == BrowserMode::Loading) {
      if (browserClient.connected()) browserClient.stop();
      browserMode = BrowserMode::Search;
      browserPageLen = 0;
      browserPage[0] = '\0';
      snprintf(browserStatus, sizeof(browserStatus), "ENTER A URL");
      
      display.fillRect(16, 132, 288, 36, COLOR_BG);
      drawRoundedPanel(16, 130, 288, 36, COLOR_SURFACE, COLOR_ACCENT_DARK);
      char bar[24] = {};
      uint8_t start = 0;
      if (browserUrlLength > 22) start = browserUrlLength - 22;
      for (uint8_t i = 0; i < 22 && (start + i) < browserUrlLength; ++i) bar[i] = browserUrl[start + i];
      bar[23] = '\0';
      text(COLOR_TEXT, 1, 26, 134, bar);
      text(COLOR_MUTED, 1, 16, 172, browserStatus);
      text(COLOR_MUTED, 1, 16, 290, "ENTER GO  DEL ERASE  BACK HOME");
      return;
    }
    switchApp(AppId::Home);
    return;
  }
  if (browserMode != BrowserMode::Search) return;
  if (key == KEY_DELETE || key == 0x08 || key == 0x7F) {
    if (browserUrlLength > 0) {
      --browserUrlLength;
      browserUrl[browserUrlLength] = '\0';
      
      display.fillRect(16, 132, 288, 36, COLOR_BG);
      drawRoundedPanel(16, 130, 288, 36, COLOR_SURFACE, COLOR_ACCENT_DARK);
      char bar[24] = {};
      uint8_t start = 0;
      if (browserUrlLength > 22) start = browserUrlLength - 22;
      for (uint8_t i = 0; i < 22 && (start + i) < browserUrlLength; ++i) bar[i] = browserUrl[start + i];
      bar[23] = '\0';
      text(COLOR_TEXT, 1, 26, 134, bar);
      text(COLOR_MUTED, 1, 16, 172, browserStatus);
      text(COLOR_MUTED, 1, 16, 290, "ENTER GO  DEL ERASE  BACK HOME");
    }
    return;
  }
  if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
    if (browserUrlLength == 0) {
      snprintf(browserStatus, sizeof(browserStatus), "ENTER A URL");
      text(COLOR_MUTED, 1, 16, 172, browserStatus);
      return;
    }
    startBrowserFetch();
    return;
  }
  if ((key >= 32 && key <= 126) && browserUrlLength < BROWSER_URL_LEN) {
    browserUrl[browserUrlLength++] = static_cast<char>(key);
    browserUrl[browserUrlLength] = '\0';
    
    display.fillRect(16, 132, 288, 36, COLOR_BG);
    drawRoundedPanel(16, 130, 288, 36, COLOR_SURFACE, COLOR_ACCENT_DARK);
    char bar[24] = {};
    uint8_t start = 0;
    if (browserUrlLength > 22) start = browserUrlLength - 22;
    for (uint8_t i = 0; i < 22 && (start + i) < browserUrlLength; ++i) bar[i] = browserUrl[start + i];
    bar[23] = '\0';
    text(COLOR_TEXT, 1, 26, 134, bar);
    text(COLOR_MUTED, 1, 16, 172, browserStatus);
    text(COLOR_MUTED, 1, 16, 290, "ENTER GO  DEL ERASE  BACK HOME");
    return;
  }
}

void setup() {
  
  
  
  
  set_sys_clock_khz(200000, true);
  delay(100);
  Wire1.setSDA(EricConfig::KbdSda);
  Wire1.setSCL(EricConfig::KbdScl);
  Wire1.begin();
  Wire1.setClock(400000);
  
  
  
  
  
  
  
  display.begin(80000000);
  
  
  display.invertDisplay(true);
  display.setRotation(0);
  display.setTextWrap(false);
  
  sdReady = EricStorage::begin();
  EricAudio::begin();
  EricRadio::begin();
  EricGameBoy::begin(&display);
  loadNote();
  drawBootScreen();
  switchApp(AppId::Home);
}

void loop() {
  
  
  if (activeApp == AppId::AudioPlayer) EricAudio::pump();
  if (activeApp == AppId::Tracks) EricTracks::pump();
  serviceWeather();
  serviceBrowser();
  if (activeApp == AppId::Radio) {
    EricRadio::pump();
    const EricRadio::State currentRadioState = EricRadio::state();
    if (currentRadioState != lastRadioState) {
      lastRadioState = currentRadioState;
      drawRadioWorkspace();
    }
  }
  
  
  
  
  
  
  
  
  const bool emulating = activeApp == AppId::GameBoy && gameMode == GameMode::Playing;
  if (networkJoinPending && !emulating) serviceNetworkConnection();
  
  
  
  
  EricTransfer::maintenance();
  if (activeApp == AppId::Network && networkMode == NetworkMode::Transfer) {
    EricTransfer::pump();
    
    
    
    
    if (millis() - lastTransferRedraw > 100) {
      lastTransferRedraw = millis();
      updateTransferStatusLines();
    }
  }
  if (activeApp != AppId::GameBoy) {
    serviceWirelessStatusIcon();
  }
  
  
  runGameFrame();
  
  
  
  if (millis() - lastPoll < 4) return;
  lastPoll = millis();
  const int event = readKeyboardEvent();
  
  
  if (event < 0) {
    keyboardAwaitingRelease = false;
    
    
    
    return;
  }
  if (keyboardAwaitingRelease) return;
  const uint8_t key = static_cast<uint8_t>(event);
  const uint32_t now = millis();
  
  
  
  
  
  
  
  
  
  const bool gameActionKey = key == '=' || key == '-' || key == '[' || key == ']';
  const bool printable = key >= 0x20 && key <= 0x7E;
  const uint32_t duplicateGuardMs =
    (activeApp == AppId::GameBoy && gameMode == GameMode::Playing)
      ? (gameActionKey ? 70UL : 28UL)
      : (printable ? 45UL : 180UL);
  if (key == lastKeyCode && now - lastKeyAt < duplicateGuardMs) return;
  lastKeyCode = key;
  lastKeyAt = now;
  keyboardAwaitingRelease = true;
  
  
  if (activeApp == AppId::Scratchpad && scratchpadModal != ScratchpadModal::None) {
    handleScratchpadKey(key);
    return;
  }
  
  
  
  if (key == KEY_ESC || key == 0x1B) {
    if (!quickMenuVisible) {
      
      
      
      
      if (activeApp == AppId::GameBoy && gameMode == GameMode::Playing) {
        EricGameBoy::yieldDisplayToSystem();
      }
      quickMenuVisible = true;
      quickAction = QuickAction::SaveNote;
      drawQuickMenu();
    } else {
      quickMenuVisible = false;
      switchApp(activeApp);
    }
    return;
  }
  if (quickMenuVisible) {
    constexpr uint8_t itemCount = 5;
    if (key == KEY_UP || key == KEY_JOY_UP) {
      quickAction = static_cast<QuickAction>((static_cast<uint8_t>(quickAction) + itemCount - 1) % itemCount);
      drawQuickMenu();
    } else if (key == KEY_DOWN || key == KEY_JOY_DOWN) {
      quickAction = static_cast<QuickAction>((static_cast<uint8_t>(quickAction) + 1) % itemCount);
      drawQuickMenu();
    } else if (key == KEY_BACK) {
      quickMenuVisible = false;
      switchApp(activeApp);
    } else if (key == KEY_ENTER || key == 0x0D || key == KEY_JOY_CENTER) {
      const QuickAction action = quickAction;
      quickMenuVisible = false;
      if (action == QuickAction::NewNote) requestNewScratchpad();
      else if (action == QuickAction::SaveNote) { saveNote(); switchApp(activeApp); }
      else if (action == QuickAction::Dashboard) requestScratchpadExit(AppId::Home);
      else if (action == QuickAction::Back) goBack();
      else switchApp(activeApp);
    }
    return;
  }
  
  
  if (activeApp == AppId::Explorer && explorerMode == ExplorerMode::Browse &&
      (key == KEY_DELETE || key == 0x7F)) {
    explorerAction = ExplorerAction::Open;
    explorerMode = ExplorerMode::Actions;
    drawExplorerActionMenu();
    return;
  }
  
  
  if (activeApp == AppId::Explorer) {
    handleExplorerKey(key);
    return;
  }
  if (activeApp == AppId::ImageViewer) {
    handleBmpViewerKey(key);
    return;
  }
  
  
  if (activeApp == AppId::AudioPlayer && (key == 'f' || key == 'F')) {
    EricAudio::stop();
    soundFolder[0] = '/';
    soundFolder[1] = '\0';
    soundFolderSelection = 0;
    soundTrackCount = 0;
    soundMode = SoundMode::FolderSelect;
    refreshSoundFolders();
    drawSoundFolderSelect();
    return;
  }
  if (activeApp == AppId::AudioPlayer) {
    handleAudioPlayerKey(key);
    return;
  }
  if (activeApp == AppId::Network) {
    handleNetworkKey(key);
    return;
  }
  if (activeApp == AppId::Weather) {
    handleWeatherKey(key);
    return;
  }
  if (activeApp == AppId::Tracks) {
    handleTracksKey(key);
    return;
  }
  
  if (activeApp == AppId::Radio) {
    handleRadioKey(key);
    return;
  }
  if (activeApp == AppId::Calculator) {
    handleCalculatorKey(key);
    return;
  }
  if (activeApp == AppId::GameBoy) {
    handleGameKey(key);
    return;
  }
  
  if (key == KEY_BACK) {
    goBack();
    return;
  }
  if (activeApp == AppId::Home) handleHomeKey(key);
  else if (activeApp == AppId::Shell) handleShellKey(key);
  else if (activeApp == AppId::Scratchpad) handleScratchpadKey(key);
}