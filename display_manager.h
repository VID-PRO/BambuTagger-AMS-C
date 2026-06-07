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

private:
  void drawStatusBar(bool wifiConnected);
  void drawSlotGrid(const SpoolInfo slots[NUM_SLOTS]);
  void drawPrinterSlots(BambuPrinter* printer, uint8_t amsUnit);
  void drawFooter(bool mqttConnected, bool printerOnline, float temp = -99, float humidity = -1);

  Adafruit_ST7789* display;
  SPIClass *hspi = NULL;
  char deviceName[32];
  unsigned long lastUpdate;
  bool wifiConnectedOld = false;
  bool mqttConnectedOld = false;
  bool printerOld = false;
  float tempOld = 0.0;
  float humidityOld = 0.0;
};

#endif