#ifndef BAMBU_PRINTER_H
#define BAMBU_PRINTER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

#define MQTT_BUFFER_SIZE 32768
#define MAX_DETECTED_AMS 4

struct AmsInfo {
  uint8_t id;
  bool connected;
  char fwVer[32];
  char productName[32];
  char serial[32];
  char trays[4][32];
  char trayTypes[4][16];
  char trayColors[4][9];
  float temperature;
  float humidity;
};

enum PrinterState {
  PRINTER_DISCONNECTED,
  PRINTER_CONNECTING,
  PRINTER_CONNECTED,
  PRINTER_ERROR
};

class BambuPrinter {
public:
  void begin(const SystemConfig &cfg);
  void update();
  void sendSpoolData(uint8_t slot, const SpoolInfo &info);
  void requestPrinterStatus();
  void sendBmeData(float temp, float humidity);
  bool isConnected() const;
  bool isPrinterOnline() const { return printerOnline; }
  PrinterState getState() const;
  bool isAmsDetected(uint8_t amsId) const;
  uint8_t getAmsExistBits() const;
  uint8_t getDetectedAmsCount() const;
  const char* getAmsTrayMaterial(uint8_t amsId, uint8_t trayId) const;
  const char* getAmsTrayType(uint8_t amsId, uint8_t trayId) const;
  const char* getAmsTrayColor(uint8_t amsId, uint8_t trayId) const;
  const char* getAmsFwVer(uint8_t amsId) const;
  const char* getAmsProductName(uint8_t amsId) const;
  const char* getAmsSerial(uint8_t amsId) const;
  float getAmsTemperature(uint8_t amsId) const;
  float getAmsHumidity(uint8_t amsId) const;
  void reconnect();

private:
  void mqttCallback(char* topic, byte* payload, unsigned int length);
  static void staticMqttCallback(char* topic, byte* payload, unsigned int length);
  void parseReport(JsonDocument &doc);

  WiFiClient* tcpClient;
  WiFiClientSecure* tlsClient;
  PubSubClient* mqttClient;
  SystemConfig config;
  PrinterState state;
  char mqttClientId[48];
  unsigned long lastReconnectAttempt;
  unsigned long lastStatusRequest;
  bool printerOnline;
  bool amsDetected;
  uint8_t amsExistBits;
  AmsInfo detectedAms[MAX_DETECTED_AMS];

  static BambuPrinter* instance;
};

#endif
