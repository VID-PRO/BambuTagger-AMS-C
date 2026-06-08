#include "display_manager.h"

static uint16_t hexToRgb565(const char* hex) {
  uint8_t r = 0, g = 0, b = 0;
  for (int i = 0; i < 2; i++) {
    r <<= 4;
    char c = hex[i];
    if (c >= '0' && c <= '9') r |= (c - '0');
    else if (c >= 'A' && c <= 'F') r |= (c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') r |= (c - 'a' + 10);
  }
  for (int i = 2; i < 4; i++) {
    g <<= 4;
    char c = hex[i];
    if (c >= '0' && c <= '9') g |= (c - '0');
    else if (c >= 'A' && c <= 'F') g |= (c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') g |= (c - 'a' + 10);
  }
  for (int i = 4; i < 6; i++) {
    b <<= 4;
    char c = hex[i];
    if (c >= '0' && c <= '9') b |= (c - '0');
    else if (c >= 'A' && c <= 'F') b |= (c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') b |= (c - 'a' + 10);
  }
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void DisplayManager::begin(const char* devName) {
  strncpy(deviceName, devName, sizeof(deviceName) - 1);
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  hspi = new SPIClass(HSPI);
  hspi->begin(TFT_SCL, -1, TFT_SDA, -1);
  display = new Adafruit_ST7789(hspi, TFT_CS, TFT_DC, TFT_RES);

  display->init(240, 240, SPI_MODE3);
  //display->setSPISpeed(40000000);
  display->setRotation(2);
  display->fillScreen(ST77XX_BLACK);
  display->setTextSize(2);
  display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display->cp437(true);
  lastUpdate = 0;
}

void DisplayManager::update(const SpoolInfo slots[NUM_SLOTS], bool wifiConnected,
                            bool mqttConnected, BambuPrinter* printer,
                            uint8_t amsUnit, float temp, float humidity) {
  if (!display) return;
  unsigned long now = millis();
  if (now - lastUpdate < 1000) return;
  lastUpdate = now;

  if (wifiConnected != wifiConnectedOld) {
    drawStatusBar(wifiConnected);
    wifiConnectedOld = wifiConnected;
  }
  bool printerMode = mqttConnected && printer && printer->isAmsDetected(amsUnit);
  bool modeChanged = (printerMode != _lastPrinterMode);
  _lastPrinterMode = printerMode;

  if (_dirty) {
    display->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ST77XX_BLACK);
    drawStatusBar(wifiConnectedOld);
    memset(_lastSlots, 0, sizeof(_lastSlots));  // invalidate slot cache
    _lastPrinterMode = !printerMode;            // invalidate mode cache → forces redraw
    memset(_lastTrayType, 0, sizeof(_lastTrayType));
    memset(_lastTrayColor, 0, sizeof(_lastTrayColor));
    tempOld = -999;
    humidityOld = -999;  // invalidate footer cache
    mqttConnectedOld = !mqttConnected;
    _dirty = false;
  }

  if (printerMode) {
    bool changed = modeChanged;
    for (uint8_t i = 0; i < NUM_SLOTS && !changed; i++) {
      const char* t = printer->getAmsTrayType(amsUnit, i);
      const char* c = printer->getAmsTrayColor(amsUnit, i);
      if (strcmp(t ? t : "", _lastTrayType[i]) != 0) changed = true;
      if (strcmp(c ? c : "", _lastTrayColor[i]) != 0) changed = true;
    }
    if (changed) {
      drawPrinterSlots(printer, amsUnit);
      for (uint8_t i = 0; i < NUM_SLOTS; i++) {
        const char* t = printer->getAmsTrayType(amsUnit, i);
        const char* c = printer->getAmsTrayColor(amsUnit, i);
        strncpy(_lastTrayType[i], t ? t : "", 15);
        _lastTrayType[i][15] = '\0';
        strncpy(_lastTrayColor[i], c ? c : "", 8);
        _lastTrayColor[i][8] = '\0';
      }
    }
  } else {
    if (modeChanged || memcmp(slots, _lastSlots, sizeof(SpoolInfo) * NUM_SLOTS) != 0) {
      drawSlotGrid(slots);
      memcpy(_lastSlots, slots, sizeof(SpoolInfo) * NUM_SLOTS);
    }
  }
  if (mqttConnected != mqttConnectedOld || temp != tempOld || humidity != humidityOld || printer->isPrinterOnline() != printerOld) {
    drawFooter(mqttConnected, printer ? printer->isPrinterOnline() : false, temp, humidity);
    mqttConnectedOld = mqttConnected;
    tempOld = temp;
    humidityOld = humidity;
    printerOld = printer->isPrinterOnline();
  }
}

void DisplayManager::drawStatusBar(bool wifiConnected) {
  display->fillRect(0, 0, SCREEN_WIDTH, 24, ST77XX_GREEN);
  display->setTextSize(2);
  display->setTextColor(ST77XX_BLACK, ST77XX_GREEN);
  display->setCursor(4, 4);
  display->print(deviceName);

  if (wifiConnected == true) {
    int16_t x = SCREEN_WIDTH - 20;
    display->drawBitmap(x, 4, WIFI_BITMAP, WIFI_WIDTH, WIFI_HEIGHT, ST77XX_BLACK);
  } else {
    int16_t x = SCREEN_WIDTH - (2 * 12) - 4;
    display->setCursor(x, 4);
    display->print("AP");
  }

  display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
}

void DisplayManager::drawSlotGrid(const SpoolInfo slots[NUM_SLOTS]) {
  display->setTextSize(2);

  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    uint8_t y = 30 + (i * 42);
    display->fillRect(4, y + 20, 14, 14, ST77XX_BLACK);
    display->fillRect(205, y, 30, 30, ST77XX_BLACK);
    display->fillRect(32, y, 170, 16, ST77XX_BLACK);
    display->fillRect(24, y + 20, 175, 16, ST77XX_BLACK);

    display->setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    display->setCursor(4, y);
    display->printf("%d:", i + 1);

    if (slots[i].present && slots[i].tagReadSuccess) {
      display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      char matShort[13];
      strncpy(matShort, slots[i].materialType, 12);
      matShort[12] = '\0';
      display->setCursor(32, y);
      display->print(matShort);

      if (slots[i].colorHex[0] && strlen(slots[i].colorHex) >= 6) {
        uint16_t swatchColor = hexToRgb565(slots[i].colorHex);
        display->fillRect(4, y + 20, 14, 14, swatchColor);
        display->drawRect(4, y + 20, 14, 14, ST77XX_WHITE);

        display->setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
        display->setCursor(24, y + 20);
        display->print("#");
        display->print(slots[i].colorHex);
      }

      if (slots[i].totalGrams > 0) {
        uint8_t pct = (uint16_t)((uint32_t)slots[i].remainingGrams * 100 / slots[i].totalGrams);
        uint16_t pctColor = (pct > 50) ? ST77XX_GREEN : (pct > 20) ? ST77XX_YELLOW
                                                                   : ST77XX_RED;
        display->setTextColor(pctColor, ST77XX_BLACK);
        display->setCursor(SCREEN_WIDTH - 48, y + 20);
        display->printf("%3d%%", pct);
      }
    } else if (slots[i].present) {
      display->setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
      display->setCursor(32, y);
      display->print("reading...");
    } else {
      display->setTextColor(0x6B4D, ST77XX_BLACK);
      display->setCursor(32, y);
      display->print("empty");
      display->drawRect(205, y, 30, 30, 0x6B4D);
      display->setCursor(212, y + 5);
      display->setTextSize(3);
      display->print("X");
      display->setTextSize(2);
    }
  }
}

void DisplayManager::drawPrinterSlots(BambuPrinter* printer, uint8_t amsUnit) {
  display->setTextSize(2);

  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    uint8_t y = 30 + (i * 42);
    display->fillRect(4, y + 20, 14, 14, ST77XX_BLACK);
    display->fillRect(205, y, 30, 30, ST77XX_BLACK);
    display->fillRect(32, y, 170, 16, ST77XX_BLACK);
    display->fillRect(24, y + 20, 175, 16, ST77XX_BLACK);

    display->setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    display->setCursor(4, y);
    display->printf("%d:", i + 1);

    const char* ttype = printer->getAmsTrayType(amsUnit, i);
    if (ttype && ttype[0]) {
      display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      char matShort[14];
      strncpy(matShort, ttype, 12);
      matShort[12] = ' ';
      matShort[13] = '\0';
      display->setCursor(32, y);
      display->print(matShort);

      const char* col = printer->getAmsTrayColor(amsUnit, i);
      if (col && strlen(col) >= 6) {
        uint16_t swatchColor = hexToRgb565(col);
        display->fillRect(205, y, 30, 30, swatchColor);
        display->drawRect(205, y, 30, 30, 0x6B4D);

        char col6[7];
        strncpy(col6, col, 6);
        col6[6] = '\0';
        display->setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
        display->setCursor(32, y + 20);
        display->print("#");
        display->print(col6);
      }
    } else {
      display->setTextColor(0x6B4D, ST77XX_BLACK);
      display->setCursor(32, y);
      display->print("empty     ");
      display->drawRect(205, y, 30, 30, 0x6B4D);
      display->setCursor(212, y + 5);
      display->setTextSize(3);
      display->print("X");
      display->setTextSize(2);
    }
  }
}

void DisplayManager::drawFooter(bool mqttConnected, bool printerOnline, float temp, float humidity) {
  display->drawFastHLine(0, 198, SCREEN_WIDTH, ST77XX_GREEN);

  if (temp > -99) {
    display->setTextSize(2);
    display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    display->setCursor(4, 204);
    display->printf("%-2.0fC", temp);
    display->setTextSize(3);
    if (humidity < 30) {
      display->setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    } else if (humidity < 40) {
      display->setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    } else {
      display->setTextColor(ST77XX_RED, ST77XX_BLACK);
    }
    display->setCursor(100, 209);
    display->printf("%-2.0f%%", humidity);  // padded
  }
  display->setTextSize(2);
  display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display->setCursor(4, 226);
  display->print("MQTT:");
  display->setCursor(64, 226);
  mqttConnected ? display->setTextColor(ST77XX_GREEN, ST77XX_BLACK) : display->setTextColor(ST77XX_RED, ST77XX_BLACK);
  display->print(mqttConnected ? "OK" : "--");
  display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display->setCursor(SCREEN_WIDTH - 73, 226);
  display->print("PTR:");
  display->setCursor(SCREEN_WIDTH - 24, 226);
  printerOnline ? display->setTextColor(ST77XX_GREEN, ST77XX_BLACK) : display->setTextColor(ST77XX_RED, ST77XX_BLACK);
  display->print(printerOnline ? "OK" : "--");
  display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
}

void DisplayManager::showOtaProgress(const char* line1, const char* line2,
                                     const char* line3, int pct) {
  if (!display) return;

  const int barY = 130;
  const int barW = SCREEN_WIDTH - 16;

  bool firstDraw = (_lastOtaPct == -1);

  if (firstDraw) {
    display->fillScreen(ST77XX_BLACK);  // ← only on first call
    drawStatusBar(true);

    display->setTextSize(2);
    display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    if (line1) {
      display->setCursor(4, 36);
      display->println(line1);
    }
    if (line2) {
      display->setCursor(4, 60);
      display->println(line2);
    }
    if (line3) {
      display->setCursor(4, 84);
      display->println(line3);
    }

    if (pct >= 0) {
      display->drawRect(4, barY, barW + 8, 24, ST77XX_WHITE);  // bar outline once
    }
    drawFooter(false, false);
  }

  if (pct >= 0) {
    // Clear and redraw only the bar fill
    display->fillRect(8, barY + 4, barW, 16, ST77XX_BLACK);
    if (pct > 0) {
      uint16_t barColor = (pct < 100) ? ST77XX_CYAN : ST77XX_GREEN;
      display->fillRect(8, barY + 4, (barW * pct) / 100, 16, barColor);
    }
    // Clear and redraw only the pct text (fixed width avoids ghost digits)
    display->setTextSize(2);
    display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    display->setCursor(SCREEN_WIDTH / 2 - 24, barY + 30);
    display->printf("%3d%%", pct);
  }

  _lastOtaPct = pct;
  _dirty = true;
}

void DisplayManager::showMessage(const char* line1, const char* line2,
                                 const char* line3, const char* line4) {
  if (!display) return;
  display->fillScreen(ST77XX_BLACK);
  display->setTextSize(2);
  display->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display->setCursor(4, 4);
  display->println(line1);
  if (line2) display->println(line2);
  if (line3) display->println(line3);
  if (line4) display->println(line4);
  _lastOtaPct = -1;
  _dirty = true;
}

void DisplayManager::showBootScreen() {
  if (!display) return;
  display->fillScreen(ST77XX_BLACK);
  int16_t x = (SCREEN_WIDTH - SPLASH_WIDTH) / 2;
  int16_t y = (SCREEN_HEIGHT - SPLASH_HEIGHT) / 2 - 40;
  display->drawBitmap(x, y, SPLASH_BITMAP, SPLASH_WIDTH, SPLASH_HEIGHT, ST77XX_GREEN);
  display->setTextSize(2);
  display->setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  display->setCursor((SCREEN_WIDTH - 11 * 12) / 2, y + SPLASH_HEIGHT);
  display->print("BambuTagger");
  display->setCursor((SCREEN_WIDTH - 5 * 12) / 2, y + SPLASH_HEIGHT + 16);
  display->print("AMS-C");
  _lastOtaPct = -1;
  _dirty = true;
}