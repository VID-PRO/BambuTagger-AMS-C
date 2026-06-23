#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "config.h"
#include "bambu_printer.h"
#include "splash_logo.h"
#include "wifi_icon.h"

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240

// ── Colour palette (BambuTagger Console theme, RGB565) ──
#define COL_BG       0x18C5  // 0x1A1A2E dark navy background
#define COL_CARD     0x1107  // 0x16213E card/slot background
#define COL_TEXT     0xEF5D  // 0xEAEAEA near-white text
#define COL_SUBTEXT  0x8CD5  // 0x8899AA muted blue-grey secondary text
#define COL_GREEN    0x1DCA  // 0x1DB954 Bambu green / accent / OK
#define COL_RED      0xE267  // 0xE74C3C error / disconnected
#define COL_ORANGE   0xF524  // 0xF5A623 warning / medium
#define COL_BLUE     0x34DB  // 0x3498DB info / OTA progress
#define COL_GREY     0x6BAF  // 0x6C757D dim / disabled / empty
#define COL_SIDEBAR  0x09AC  // 0x0F3460 status bar / header background
#define COL_BORDER   0x29AC  // 0x2D3561 separator lines / borders

class DisplayManager {
public:
  void begin(const char* deviceName);
  void update(const SpoolInfo slots[NUM_SLOTS], bool wifiConnected,
              bool mqttConnected, BambuPrinter* printer = nullptr,
              uint8_t amsUnit = 0, float temp = -99, float humidity = -1);
  void showMessage(const char* line1, const char* line2 = nullptr,
                   const char* line3 = nullptr, const char* line4 = nullptr);
  void showOtaProgress(const char* line1, const char* line2 = nullptr,
                       const char* line3 = nullptr, int pct = -1);
  void showBootScreen();
  void setLayout(bool vertical);


private:
  void drawStatusBar(bool wifiConnected);
  void drawSlotGrid(const SpoolInfo slots[NUM_SLOTS]);
  void drawPrinterSlots(BambuPrinter* printer, uint8_t amsUnit);
  void drawFooter(bool mqttConnected, bool printerOnline, float temp = -99, float humidity = -1);
  void drawSlotGridVertical(const SpoolInfo slots[NUM_SLOTS]);
  void drawPrinterSlotsVertical(BambuPrinter* printer, uint8_t amsUnit);

  Adafruit_ST7789* display;
  SPIClass *hspi = NULL;
  char deviceName[32];
  unsigned long lastUpdate;
  bool wifiConnectedOld = false;
  bool mqttConnectedOld = false;
  bool printerOld = false;
  float tempOld = 0.0;
  float humidityOld = 0.0;
  SpoolInfo _lastSlots[NUM_SLOTS];
  bool _lastPrinterMode = false;
  char _lastTrayType[NUM_SLOTS][16];
  char _lastTrayColor[NUM_SLOTS][9];
  uint8_t _lastTrayRemain[NUM_SLOTS];   // cache for dirty-check
  bool _dirty = true;
  int _lastOtaPct = -1;
  bool _verticalLayout = false;
};

#endif
