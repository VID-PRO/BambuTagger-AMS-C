#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "rfid_manager.h"
#include "bambu_printer.h"

class WebInterface {
public:
  void begin(SystemConfig &cfg, RfidManager* rfid, BambuPrinter* printer,
             void (*rebootCallback)(void) = nullptr,
             void (*otaCallback)(void) = nullptr);
  void handleClient();
  void updateStatus(bool wifiConnected, const char* ipAddress,
                    bool mqttConnected, bool printerOnline);

private:
  void handleRoot();
  void handleFavicon();
  void handleStatus();
  void handleAmsStatus();
  void handleConfigGet();
  void handleConfigPost();
  void handleScan();
  void handleSend();
  void handleSync();
  void handleOta();
  void handleOtaCheck();
  void handleVersion();
  void sendJsonResponse(JsonDocument &doc, int code = 200);
  void setupRoutes();

  WebServer* server;
  SystemConfig* config;
  RfidManager* rfidManager;
  BambuPrinter* bambuPrinter;
  void (*rebootFn)(void);
  void (*otaFn)(void);

  bool wifiStatus;
  String ipAddress;
  bool mqttStatus;
  bool printerOnlineStatus;
};

#endif
