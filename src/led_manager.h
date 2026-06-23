#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

enum LedMode {
  LED_OFF,
  LED_IDLE,
  LED_READING,
  LED_TAG_PRESENT,
  LED_TAG_OK,
  LED_TAG_COLOR,
  LED_ERROR,
  LED_WIFI_DISCONNECTED,
  LED_WIFI_CONNECTED
};

class LedManager {
public:
  void begin();
  void setSlotLed(uint8_t slot, LedMode mode);
  void setSlotColor(uint8_t slot, uint8_t r, uint8_t g, uint8_t b);
  void setAllLeds(LedMode mode);
  void setBrightness(uint8_t brightness);
  void update();

private:
  Adafruit_NeoPixel* strip;
  LedMode currentMode[NUM_SLOTS];
  uint8_t customR[NUM_SLOTS];
  uint8_t customG[NUM_SLOTS];
  uint8_t customB[NUM_SLOTS];
  uint8_t lastR[NUM_SLOTS];
  uint8_t lastG[NUM_SLOTS];
  uint8_t lastB[NUM_SLOTS];
  LedMode lastMode[NUM_SLOTS];
  unsigned long lastUpdate;
  uint8_t brightness;
};

#endif
