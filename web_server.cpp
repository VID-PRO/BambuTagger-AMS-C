#include <functional>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "web_server.h"

extern float bmeTemp;
extern float bmeHumidity;
extern bool bmeOk;
#include "index_html.h"
#include "favicon.h"

void WebInterface::begin(SystemConfig& cfg, RfidManager* rfid, BambuPrinter* printer,
                         void (*rebootCallback)(void), void (*otaCallback)(void)) {
  config = &cfg;
  rfidManager = rfid;
  bambuPrinter = printer;
  rebootFn = rebootCallback;
  otaFn = otaCallback;

  server = new WebServer(80);
  setupRoutes();
  server->begin();
  wifiStatus = false;
  mqttStatus = false;
  printerOnlineStatus = false;
}

void WebInterface::setupRoutes() {
  server->on("/", std::bind(&WebInterface::handleRoot, this));
  server->on("/favicon.ico", std::bind(&WebInterface::handleFavicon, this));
  server->on("/api/status", HTTP_GET, std::bind(&WebInterface::handleStatus, this));
  server->on("/api/ams", HTTP_GET, std::bind(&WebInterface::handleAmsStatus, this));
  server->on("/api/config", HTTP_GET, std::bind(&WebInterface::handleConfigGet, this));
  server->on("/api/config", HTTP_POST, std::bind(&WebInterface::handleConfigPost, this));
  server->on("/api/scan", HTTP_POST, std::bind(&WebInterface::handleScan, this));
  server->on("/api/send", HTTP_POST, std::bind(&WebInterface::handleSend, this));
  server->on("/api/sync", HTTP_POST, std::bind(&WebInterface::handleSync, this));
  server->on("/api/ota", HTTP_POST, std::bind(&WebInterface::handleOta, this));
  server->on("/api/ota-check", HTTP_GET, std::bind(&WebInterface::handleOtaCheck, this));
  server->on("/api/ams-get-rfid", HTTP_POST, std::bind(&WebInterface::handleAmsGetRfid, this));
  server->on("/api/version", HTTP_GET, std::bind(&WebInterface::handleVersion, this));
  server->on("/api/led/brightness", HTTP_POST, std::bind(&WebInterface::handleLedPost, this));
  server->on("/api/led/brightness", HTTP_GET, std::bind(&WebInterface::handleLedGet, this));
}

void WebInterface::handleClient() {
  if (server) server->handleClient();
}

void WebInterface::updateStatus(bool wifiConnected, const char* ip,
                                bool mqttConnected, bool printerOnline) {
  wifiStatus = wifiConnected;
  ipAddress = ip ? String(ip) : "";
  mqttStatus = mqttConnected;
  printerOnlineStatus = printerOnline;
}

void WebInterface::handleFavicon() {
  server->send_P(200, "image/x-icon", (const char*)FAVICON_ICO, FAVICON_SIZE);
}

void WebInterface::handleRoot() {
  server->send_P(200, "text/html", INDEX_HTML);
}

void WebInterface::handleStatus() {
  DynamicJsonDocument doc(8192);
  doc["wifiConnected"] = wifiStatus;
  doc["ipAddress"] = ipAddress;
  doc["mqttConnected"] = mqttStatus;
  doc["printerOnline"] = printerOnlineStatus;
  doc["mqttEnabled"] = config->mqttEnabled;
  doc["bmeOk"] = bmeOk;
  if (bmeOk) {
    doc["temperature"] = bmeTemp;
    doc["humidity"] = bmeHumidity;
  }

  JsonObject amsInfo = doc.createNestedObject("detectedAms");
  amsInfo["bits"] = bambuPrinter ? bambuPrinter->getAmsExistBits() : 0;
  amsInfo["count"] = bambuPrinter ? bambuPrinter->getDetectedAmsCount() : 0;
  JsonArray amsList = amsInfo.createNestedArray("units");
  if (bambuPrinter) {
    for (uint8_t a = 0; a < 4; a++) {
      JsonObject unit = amsList.createNestedObject();
      unit["id"] = a;
      unit["label"] = (const char*)(a == 0 ? "A" : a == 1 ? "B"
                                                 : a == 2 ? "C"
                                                          : "D");
      unit["connected"] = bambuPrinter->isAmsDetected(a);
      unit["fwVer"] = bambuPrinter->getAmsFwVer(a);
      unit["productName"] = bambuPrinter->getAmsProductName(a);
      unit["serial"] = bambuPrinter->getAmsSerial(a);
      unit["temperature"] = bambuPrinter->getAmsTemperature(a);
      unit["humidity"] = bambuPrinter->getAmsHumidity(a);
      JsonArray trays = unit.createNestedArray("trays");
      for (uint8_t t = 0; t < 4; t++) {
        JsonObject tray = trays.createNestedObject();
        tray["trayId"] = t;
        tray["material"] = bambuPrinter->getAmsTrayMaterial(a, t);
        tray["trayType"] = bambuPrinter->getAmsTrayType(a, t);
        tray["color"] = bambuPrinter->getAmsTrayColor(a, t);
        SpoolInfo tagInfo;
        if (rfidManager->getSpoolInfo(t, tagInfo) && tagInfo.present) {
          tray["sub"] = tagInfo.detailedType;
        }
      }
    }
  }
  amsInfo["configuredUnit"] = config->amsUnit;

  JsonArray slots = doc.createNestedArray("slots");
  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    SpoolInfo info;
    bool present = rfidManager->getSpoolInfo(i, info);
    JsonObject slot = slots.createNestedObject();
    slot["slot"] = i;
    slot["present"] = present || info.present;
    slot["uid"] = info.uid;
    slot["materialType"] = info.materialType;
    slot["detailedType"] = info.detailedType;
    slot["color"] = info.color;
    slot["colorHex"] = info.colorHex;
    slot["remainingGrams"] = info.remainingGrams;
    slot["totalGrams"] = info.totalGrams;
    slot["batchNumber"] = info.batchNumber;
    slot["manufacturer"] = info.manufacturer;
    slot["tagReadSuccess"] = info.tagReadSuccess;
  }

  JsonArray printerSlots = doc.createNestedArray("printerSlots");
  if (bambuPrinter) {
    uint8_t amsUnit = config->amsUnit;
    bool amsConnected = bambuPrinter->isAmsDetected(amsUnit);
    for (uint8_t t = 0; t < 4; t++) {
      JsonObject ps = printerSlots.createNestedObject();
      ps["slot"] = t;
      const char* ttype = bambuPrinter->getAmsTrayType(amsUnit, t);
      ps["trayType"] = ttype;
      ps["material"] = bambuPrinter->getAmsTrayMaterial(amsUnit, t);
      ps["color"] = bambuPrinter->getAmsTrayColor(amsUnit, t);
      ps["hasSpool"] = (ttype && ttype[0] != '\0');
      ps["amsConnected"] = amsConnected;
    }
  }

  sendJsonResponse(doc);
}

void WebInterface::handleAmsStatus() {
  DynamicJsonDocument doc(512);
  doc["configuredUnit"] = config->amsUnit;
  doc["configuredLabel"] = (const char*)(config->amsUnit == 0 ? "A" : config->amsUnit == 1 ? "B"
                                                                    : config->amsUnit == 2 ? "C"
                                                                                           : "D");
  doc["detected"] = bambuPrinter ? bambuPrinter->isAmsDetected(config->amsUnit) : false;
  doc["existBits"] = bambuPrinter ? bambuPrinter->getAmsExistBits() : 0;
  doc["count"] = bambuPrinter ? bambuPrinter->getDetectedAmsCount() : 0;

  JsonArray units = doc.createNestedArray("units");
  if (bambuPrinter) {
    const char* labels[] = { "A", "B", "C", "D" };
    for (uint8_t a = 0; a < 4; a++) {
      JsonObject unit = units.createNestedObject();
      unit["id"] = a;
      unit["label"] = labels[a];
      unit["connected"] = bambuPrinter->isAmsDetected(a);
    }
  }
  sendJsonResponse(doc);
}

void WebInterface::handleConfigGet() {
  DynamicJsonDocument doc(1024);
  doc["wifiSSID"] = config->wifiSSID;
  doc["wifiPassword"] = "********";
  doc["printerIP"] = config->printerIP;
  doc["printerPort"] = config->printerPort;
  doc["printerAccessCode"] = config->printerAccessCode[0] ? "********" : "";
  doc["printerSerial"] = config->printerSerial;
  doc["deviceName"] = config->deviceName;
  doc["mqttEnabled"] = config->mqttEnabled;
  doc["mqttUpdateIntervalMs"] = config->mqttUpdateIntervalMs;
  doc["mqttUseTLS"] = config->mqttUseTLS;
  doc["amsUnit"] = config->amsUnit;
  sendJsonResponse(doc);
}

void WebInterface::handleConfigPost() {
  if (!server->hasArg("plain")) {
    DynamicJsonDocument doc(128);
    doc["ok"] = false;
    doc["error"] = "No body";
    sendJsonResponse(doc, 400);
    return;
  }

  DynamicJsonDocument postDoc(1024);
  DeserializationError err = deserializeJson(postDoc, server->arg("plain"));
  if (err) {
    DynamicJsonDocument doc(128);
    doc["ok"] = false;
    doc["error"] = err.c_str();
    sendJsonResponse(doc, 400);
    return;
  }

  bool changed = false;

  if (postDoc.containsKey("wifiSSID")) {
    strncpy(config->wifiSSID, postDoc["wifiSSID"] | "", sizeof(config->wifiSSID) - 1);
    changed = true;
  }
  if (postDoc.containsKey("wifiPassword") && strcmp(postDoc["wifiPassword"] | "", "********") != 0) {
    strncpy(config->wifiPassword, postDoc["wifiPassword"] | "", sizeof(config->wifiPassword) - 1);
    changed = true;
  }
  if (postDoc.containsKey("printerIP")) {
    strncpy(config->printerIP, postDoc["printerIP"] | "", sizeof(config->printerIP) - 1);
    changed = true;
  }
  if (postDoc.containsKey("printerPort")) {
    config->printerPort = postDoc["printerPort"] | config->printerPort;
    changed = true;
  }
  if (postDoc.containsKey("printerAccessCode") && strcmp(postDoc["printerAccessCode"] | "", "********") != 0) {
    strncpy(config->printerAccessCode, postDoc["printerAccessCode"] | "", sizeof(config->printerAccessCode) - 1);
    changed = true;
  }
  if (postDoc.containsKey("printerSerial")) {
    strncpy(config->printerSerial, postDoc["printerSerial"] | "", sizeof(config->printerSerial) - 1);
    changed = true;
  }
  if (postDoc.containsKey("deviceName")) {
    strncpy(config->deviceName, postDoc["deviceName"] | "", sizeof(config->deviceName) - 1);
    changed = true;
  }
  if (postDoc.containsKey("mqttEnabled")) {
    config->mqttEnabled = postDoc["mqttEnabled"] | false;
    changed = true;
  }
  if (postDoc.containsKey("mqttUseTLS")) {
    config->mqttUseTLS = postDoc["mqttUseTLS"] | false;
    changed = true;
  }
  if (postDoc.containsKey("mqttInterval")) {
    config->mqttUpdateIntervalMs = postDoc["mqttInterval"] | config->mqttUpdateIntervalMs;
    changed = true;
  }
  if (postDoc.containsKey("amsUnit")) {
    config->amsUnit = postDoc["amsUnit"] | config->amsUnit;
    if (config->amsUnit > 3) config->amsUnit = 0;
    changed = true;
  }

  if (changed) {
    saveConfig(*config);
  }

  DynamicJsonDocument resp(128);
  resp["ok"] = true;
  resp["saved"] = changed;
  sendJsonResponse(resp);

  if (changed && rebootFn) {
    delay(500);
    rebootFn();
  }
}

void WebInterface::handleScan() {
  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    rfidManager->forceRescan(i);
  }
  DynamicJsonDocument doc(64);
  doc["ok"] = true;
  sendJsonResponse(doc);
}

void WebInterface::handleSend() {
  DynamicJsonDocument doc(256);
  int sent = 0;

  if (bambuPrinter && bambuPrinter->isConnected()) {
    for (uint8_t i = 0; i < NUM_SLOTS; i++) {
      SpoolInfo info;
      if (rfidManager->getSpoolInfo(i, info) && info.present && info.tagReadSuccess) {
        bambuPrinter->sendSpoolData(i, info);
        sent++;
      }
    }
    doc["ok"] = true;
    doc["sent"] = sent;
  } else {
    doc["ok"] = false;
    doc["error"] = bambuPrinter ? "MQTT not connected" : "MQTT not configured";
    doc["sent"] = 0;
  }

  sendJsonResponse(doc);
}

void WebInterface::handleSync() {
  DynamicJsonDocument doc(128);
  if (bambuPrinter && bambuPrinter->isConnected()) {
    bambuPrinter->requestPrinterStatus();
    doc["ok"] = true;
    doc["message"] = "Sync requested";
  } else {
    doc["ok"] = false;
    doc["error"] = bambuPrinter ? "MQTT not connected" : "MQTT not configured";
  }
  sendJsonResponse(doc);
}

void WebInterface::handleAmsGetRfid() {
  DynamicJsonDocument doc(128);
  if (bambuPrinter && bambuPrinter->isConnected()) {
    uint8_t tray = (uint8_t)(server->arg("tray").toInt());
    if (tray < 4) {
      bambuPrinter->sendAmsGetRfid(tray);
      doc["ok"] = true;
    } else {
      doc["ok"] = false;
      doc["error"] = "Invalid tray";
    }
  } else {
    doc["ok"] = false;
    doc["error"] = "MQTT not connected";
  }
  sendJsonResponse(doc);
}

void WebInterface::handleOtaCheck() {
  DynamicJsonDocument doc(256);
  doc["current"] = FIRMWARE_VERSION;

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10000);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);  // we want the 302, not the destination
  String url = String("https://github.com/") + OTA_REPO + "/releases/latest";

  if (!http.begin(client, url)) {
    doc["error"] = "begin failed";
    sendJsonResponse(doc);
    return;
  }
  http.addHeader("User-Agent", String("BambuTagger-AMS/") + FIRMWARE_VERSION);
  const char* hdrs[] = { "Location" };
  http.collectHeaders(hdrs, 1);

  int code = http.GET();
  String latest = "";

  if ((code == 301 || code == 302) && http.hasHeader("Location")) {
    String loc = http.header("Location");
    // loc looks like: /VID-PRO/BambuTagger-AMS-C/releases/tag/v1.1.1
    int tagIdx = loc.lastIndexOf("/tag/");
    if (tagIdx >= 0) latest = loc.substring(tagIdx + 5);
  }
  http.end();

  if (latest.isEmpty()) {
    doc["error"] = String("HTTP ") + code;
    sendJsonResponse(doc);
    return;
  }

  doc["latest"] = latest;

  const char* r = latest.c_str();
  if (r[0] == 'v' || r[0] == 'V') r++;
  const char* l = FIRMWARE_VERSION;
  if (l[0] == 'v' || l[0] == 'V') l++;

  int rMaj = 0, rMin = 0, rPat = 0, lMaj = 0, lMin = 0, lPat = 0;
  sscanf(r, "%d.%d.%d", &rMaj, &rMin, &rPat);
  sscanf(l, "%d.%d.%d", &lMaj, &lMin, &lPat);

  doc["newer"] = ((rMaj * 10000 + rMin * 100 + rPat) > (lMaj * 10000 + lMin * 100 + lPat));
  sendJsonResponse(doc);
}

void WebInterface::handleOta() {
  DynamicJsonDocument doc(128);
  doc["ok"] = true;
  doc["message"] = "OTA update started";
  sendJsonResponse(doc);

  if (otaFn) {
    delay(200);
    otaFn();
  }
}

void WebInterface::handleVersion() {
  DynamicJsonDocument doc(128);
  doc["version"] = FIRMWARE_VERSION;
  doc["repo"] = OTA_REPO;
  sendJsonResponse(doc);
}

void WebInterface::handleLedGet() {
  Preferences prefs;
  prefs.begin("bambu-ams", true);
  int currentBrightness = prefs.getUChar("brightness", 32);
  prefs.end();
  DynamicJsonDocument doc(128);
  doc["brightness"] = String(currentBrightness);
  sendJsonResponse(doc);
}

void WebInterface::handleLedPost() {
  DynamicJsonDocument doc(128);

  int brightness = 32;
  brightness = server->arg("value").toInt();
  if (brightness >= 0 && brightness <= 255) {
    Preferences prefs;
    prefs.begin("bambu-ams", false);
    prefs.putUChar("brightness", brightness);
    prefs.end();
    doc["brightness"] = String(brightness);
    sendJsonResponse(doc);
    return;
  }
  doc["error"] = "Invalid brightness value (0-255)";
  sendJsonResponse(doc);
}

void WebInterface::sendJsonResponse(JsonDocument& doc, int code) {
  String response;
  serializeJson(doc, response);
  server->send(code, "application/json", response);
}
