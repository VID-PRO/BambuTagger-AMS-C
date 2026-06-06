/*
 * BambuTagger AMS - Multi-Spool NFC Tag Reader for Bambu Lab
 *
 * Arduino IDE: ESP32 Dev Module (Tools > Board > ESP32 > ESP32 Dev Module)
 *
 * Required libraries (install via Tools > Manage Libraries):
 *   - MFRC522-spi-i2c-uart-async
 *   - Adafruit NeoPixel by Adafruit    (v1.12.3+)
 *   - Adafruit GFX Library by Adafruit (v1.11.11+)
 *   - Adafruit ST7735 and ST7789 Library by Adafruit (v1.10.0+)
 *   - PubSubClient   by Nick O'Leary   (v2.8+)
 *   - ArduinoJson    by Benoit Blanchon (v6.21.5+)
 *
 * Wiring: see config.h for pin assignments
 */

#include <ESPmDNS.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Adafruit_BME280.h>

#include "config.h"
#include "rfid_manager.h"
#include "led_manager.h"
#include "display_manager.h"
#include "web_server.h"
#include "bambu_printer.h"

SystemConfig cfg;
RfidManager rfidManager;
LedManager ledManager;
DisplayManager displayManager;
WebInterface webInterface;
BambuPrinter bambuPrinter;
DNSServer dnsServer;
Adafruit_BME280 bme;
float bmeTemp = 0;
float bmeHumidity = 0;
bool bmeOk = false;

bool wifiConnected = false;
bool apMode = false;
String localIP = "";
const byte DNS_PORT = 53;

void handleReboot();
void connectWiFi();
void startCaptivePortal();
void updateLedStatus();

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.print(F("=== BambuTagger AMS v"));
  Serial.print(FIRMWARE_VERSION);
  Serial.println(F(" ==="));
  Serial.println(F("Multi-Spool NFC Tag Reader for Bambu Lab"));

  loadConfig(cfg);

  displayManager.begin(cfg.deviceName);
  Serial.println(F("TFT initialized"));
  displayManager.showBootScreen();
  delay(2000);

  Wire.begin();
  bmeOk = bme.begin(0x76);
  if (!bmeOk) bmeOk = bme.begin(0x77);
  if (bmeOk) {
    Serial.println(F("BME280 sensor found"));
  } else {
    Serial.println(F("BME280 not found"));
  }

  Serial.print(F("Device: "));
  Serial.println(cfg.deviceName);

  ledManager.begin();
  ledManager.setAllLeds(LED_IDLE);
  ledManager.update();

  rfidManager.begin();
  Serial.println(F("RFID readers initialized"));

  connectWiFi();

  if (wifiConnected) {
    bambuPrinter.begin(cfg);
  }

  webInterface.begin(cfg, &rfidManager, &bambuPrinter, handleReboot, performOTAUpdate);
  webInterface.updateStatus(wifiConnected, localIP.c_str(), false, false);
  if (wifiConnected) {
    Serial.println(F("Web server started on port 80"));
    Serial.println(localIP);
  } else {
    Serial.println(F("Web server started on AP 192.168.4.1"));
  }

  ledManager.setAllLeds(wifiConnected ? LED_WIFI_CONNECTED : LED_WIFI_DISCONNECTED);
  ledManager.update();

  Serial.println(F("Setup complete"));
}

void loop() {
  rfidManager.loop();
  webInterface.handleClient();
  yield();
  if (apMode) {
    dnsServer.processNextRequest();
  }
  if (wifiConnected) {
    static unsigned long lastMqttLoop = 0;
    if (millis() - lastMqttLoop > 200) {
      lastMqttLoop = millis();
      bambuPrinter.update();
    }
  }

  static unsigned long lastReconnectCheck = 0;
  static unsigned long lastLedUpdate = 0;
  static unsigned long lastDisplayUpdate = 0;
  static unsigned long lastMqttUpdate = 0;
  unsigned long now = millis();

  if (apMode && strlen(cfg.wifiSSID) > 0 && now - lastReconnectCheck > 30000) {
    lastReconnectCheck = now;
    Serial.println(F("AP Mode: retrying STA connection..."));
    WiFi.begin(cfg.wifiSSID, cfg.wifiPassword);
  }

  if (WiFi.status() == WL_CONNECTED && apMode) {
    apMode = false;
    wifiConnected = true;
    localIP = WiFi.localIP().toString();
    dnsServer.stop();
    Serial.print(F("WiFi connected! IP: "));
    Serial.println(localIP);
    displayManager.showOtaProgress("WiFi Connected", localIP.c_str());
    bambuPrinter.begin(cfg);
    if (MDNS.begin(cfg.deviceName)) {
      Serial.println(F("mDNS responder started"));
    }
  }

  static unsigned long lastBmeRead = 0;
  if (bmeOk && now - lastBmeRead > 5000) {
    lastBmeRead = now;
    bmeTemp = bme.readTemperature();
    bmeHumidity = bme.readHumidity();
  }

  static unsigned long lastBmeSend = 0;
  if (bmeOk && now - lastBmeSend > 60000) {
    lastBmeSend = now;
    bambuPrinter.sendBmeData(bmeTemp, bmeHumidity);
  }

  if (now - lastLedUpdate > 200) {
    lastLedUpdate = now;
    updateLedStatus();
    ledManager.update();
  }

  if (now - lastDisplayUpdate > 2000) {
    lastDisplayUpdate = now;
    SpoolInfo displaySlots[NUM_SLOTS];
    for (uint8_t i = 0; i < NUM_SLOTS; i++) {
      rfidManager.getSpoolInfo(i, displaySlots[i]);
    }
    bool mqttOk = bambuPrinter.isConnected();
    webInterface.updateStatus(wifiConnected, localIP.c_str(), mqttOk,
                              (bambuPrinter.getState() == PRINTER_CONNECTED));
    displayManager.update(displaySlots, wifiConnected, mqttOk,
                          &bambuPrinter, cfg.amsUnit, bmeTemp, bmeHumidity);
  }

  if (now - lastMqttUpdate > (cfg.mqttUpdateIntervalMs > 0 ? cfg.mqttUpdateIntervalMs : 5000)) {
    lastMqttUpdate = now;
    if (wifiConnected) {
      bool mqttOk = bambuPrinter.isConnected();
      webInterface.updateStatus(wifiConnected, localIP.c_str(), mqttOk,
                                (bambuPrinter.getState() == PRINTER_CONNECTED));

      if (cfg.mqttEnabled && mqttOk) {
        for (uint8_t i = 0; i < NUM_SLOTS; i++) {
          SpoolInfo info;
          if (rfidManager.getSpoolInfo(i, info) && info.present && info.tagReadSuccess) {
            bambuPrinter.sendSpoolData(i, info);
          }
        }
      }
    }
  }
}

void connectWiFi() {
  if (strlen(cfg.wifiSSID) == 0) {
    Serial.println(F("No WiFi credentials configured — starting AP"));
    wifiConnected = false;
    startCaptivePortal();
    return;
  }

  Serial.print(F("Connecting to WiFi: "));
  Serial.println(cfg.wifiSSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.wifiSSID, cfg.wifiPassword);

  ledManager.setAllLeds(LED_WIFI_DISCONNECTED);
  ledManager.update();

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    apMode = false;
    localIP = WiFi.localIP().toString();
    Serial.print(F("Connected! IP: "));
    Serial.println(localIP);
    displayManager.showOtaProgress("WiFi Connected", localIP.c_str());

    if (MDNS.begin(cfg.deviceName)) {
      Serial.println(F("mDNS responder started"));
    }
  } else {
    wifiConnected = false;
    Serial.println(F("WiFi connection failed — starting AP"));
    displayManager.showOtaProgress("WiFi Failed!", "Opening AP mode...");
    startCaptivePortal();
  }
}

void startCaptivePortal() {
  apMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(cfg.deviceName, NULL);
  delay(100);

  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  localIP = "192.168.4.1";

  dnsServer.start(DNS_PORT, "*", apIP);

  Serial.print(F("AP started: "));
  Serial.print(cfg.deviceName);
  Serial.println(F(" (open)"));
  Serial.println(F("Connect and visit http://192.168.4.1"));

  char apMsg1[32];
  char apMsg2[32];
  char apMsg3[32];
  snprintf(apMsg1, sizeof(apMsg1), "WiFi: %s", cfg.deviceName);
  snprintf(apMsg2, sizeof(apMsg2), "IP: 192.168.4.1");
  snprintf(apMsg3, sizeof(apMsg3), "Configure WiFi");
  displayManager.showOtaProgress(apMsg1, apMsg2, apMsg3);
}

static uint8_t hexToByte(const char* hex) {
  uint8_t val = 0;
  for (uint8_t i = 0; i < 2; i++) {
    char c = hex[i];
    val <<= 4;
    if (c >= '0' && c <= '9') val |= (c - '0');
    else if (c >= 'A' && c <= 'F') val |= (c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') val |= (c - 'a' + 10);
  }
  return val;
}

void updateLedStatus() {
  uint8_t amsUnit = cfg.amsUnit;
  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    if (!wifiConnected && !apMode) {
      ledManager.setSlotLed(i, LED_WIFI_DISCONNECTED);
    } else if (apMode) {
      ledManager.setSlotLed(i, LED_IDLE);
    } else if (bambuPrinter.isAmsDetected(amsUnit)) {
      const char* col = bambuPrinter.getAmsTrayColor(amsUnit, i);
      if (col && col[0]) {
        uint8_t r = hexToByte(col);
        uint8_t g = hexToByte(col + 2);
        uint8_t b = hexToByte(col + 4);
        ledManager.setSlotColor(i, r, g, b);
      } else {
        ledManager.setSlotLed(i, LED_IDLE);
      }
    } else {
      ledManager.setSlotLed(i, LED_IDLE);
    }
  }
}

void handleReboot() {
  Serial.println(F("Reboot requested via web..."));
  delay(500);
  ESP.restart();
}

void performOTAUpdate() {
  displayManager.showOtaProgress("OTA Update", "Checking version...");

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  http.begin(client, String("https://api.github.com/repos/") + OTA_REPO + "/releases/latest");
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("User-Agent", String("BambuTagger-AMS/") + FIRMWARE_VERSION);

  int code = http.GET();
  if (code != 200) {
    http.end();
    displayManager.showOtaProgress("OTA Update", "", "GitHub API error");
    delay(3000);
    return;
  }

  StaticJsonDocument<96> filter;
  filter["tag_name"] = true;
  JsonArray fa = filter.createNestedArray("assets");
  JsonObject fa0 = fa.createNestedObject();
  fa0["name"] = true;
  fa0["browser_download_url"] = true;
  fa0["size"] = true;

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) {
    displayManager.showOtaProgress("OTA Update", "", "JSON parse error");
    delay(3000);
    return;
  }

  String tag = doc["tag_name"] | "";
  if (tag.startsWith("v") || tag.startsWith("V")) tag = tag.substring(1);
  if (!tag.length()) {
    displayManager.showOtaProgress("OTA Update", "", "No release found");
    delay(3000);
    return;
  }

  // Version check
  int rMaj = 0, rMin = 0, rPat = 0, lMaj = 0, lMin = 0, lPat = 0;
  sscanf(tag.c_str(), "%d.%d.%d", &rMaj, &rMin, &rPat);
  const char* l = FIRMWARE_VERSION;
  if (l[0] == 'v' || l[0] == 'V') l++;
  sscanf(l, "%d.%d.%d", &lMaj, &lMin, &lPat);
  if (rMaj * 10000 + rMin * 100 + rPat <= lMaj * 10000 + lMin * 100 + lPat) {
    displayManager.showOtaProgress("OTA Update", "", "Already up to date");
    delay(3000);
    return;
  }

  JsonArray assets = doc["assets"];
  String dlUrl;
  int binSize = 0;
  for (JsonObject asset : assets) {
    String name = asset["name"] | "";
    if (name.endsWith(".bin") && name.indexOf("merged") < 0 && name.indexOf("bootloader") < 0 && name.indexOf("partition") < 0) {
      dlUrl = asset["browser_download_url"] | "";
      binSize = asset["size"] | 0;
      break;
    }
  }

  if (!dlUrl.length()) {
    displayManager.showOtaProgress("OTA Update", "", "No .bin found");
    delay(3000);
    return;
  }

  displayManager.showOtaProgress("OTA Update", "Downloading...", tag.c_str());

  for (int retry = 0; retry < 3; retry++) {
    if (retry > 0) {
      displayManager.showOtaProgress("OTA Update", "Retrying...", tag.c_str(), 0);
      delay(2000);
    }

    HTTPClient dl;
    dl.begin(client, dlUrl);
    dl.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    dl.addHeader("User-Agent", String("BambuTagger-AMS/") + FIRMWARE_VERSION);
    int dlCode = dl.GET();
    if (dlCode != 200) {
      dl.end();
      continue;
    }

    int totalSize = (binSize > 0) ? binSize : dl.getSize();
    if (!Update.begin((totalSize > 0) ? (size_t)totalSize : UPDATE_SIZE_UNKNOWN)) {
      dl.end();
      displayManager.showOtaProgress("OTA Update", "", "Update begin failed");
      delay(3000);
      return;
    }

    WiFiClient* stream = dl.getStreamPtr();
    uint8_t buf[512];
    int written = 0;
    unsigned long lastDraw = 0;
    unsigned long stallSince = millis();

    while (dl.connected() && (totalSize <= 0 || written < totalSize)) {
      int avail = stream->available();
      if (!avail) {
        if (written > 0 && millis() - stallSince > 5000) break;
        delay(2);
        continue;
      }
      stallSince = millis();
      int n = stream->readBytes(buf, min(avail, (int)sizeof(buf)));
      if (n <= 0) break;
      if (Update.write(buf, n) != (size_t)n) { break; }
      written += n;
      if (millis() - lastDraw > 200) {
        int pct = (totalSize > 0) ? (written * 100 / totalSize) : 50;
        displayManager.showOtaProgress("OTA Update", "Flashing...", tag.c_str(), pct);
        lastDraw = millis();
      }
    }
    dl.end();

    if (written >= totalSize && Update.end(true)) {
      return;
    }
    Update.abort();
  }

  displayManager.showOtaProgress("OTA Update", "", "Update failed");
  delay(5000);
}
