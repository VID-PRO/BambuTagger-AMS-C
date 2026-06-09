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
#include <HTTPUpdate.h>

#include <esp_http_client.h>
#include <esp_ota_ops.h>

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

SET_LOOP_TASK_STACK_SIZE(16 * 1024);

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
  Preferences prefs;
  prefs.begin("led", false);
  uint8_t savedBrightness = prefs.getUChar("brightness", LED_BRIGHTNESS);
  prefs.end();

  ledManager.setBrightness(savedBrightness);

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
  static bool otaRunning = false;
  if (otaRunning) return;
  otaRunning = true;
  displayManager.showOtaProgress("OTA Update", "Checking version...");

  String dlUrl;
  int binSize = 0;
  String tag;

  {
    // Get latest tag via redirect — no API, no rate limits, no JSON
    WiFiClientSecure client;  // ← add this
    HTTPClient http;          // ← add this
    client.setInsecure();
    client.setTimeout(10000);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.begin(client, String("https://github.com/") + OTA_REPO + "/releases/latest");
    http.addHeader("User-Agent", String("BambuTagger-AMS/") + FIRMWARE_VERSION);
    const char* hdrs[] = { "Location" };
    http.collectHeaders(hdrs, 1);

    int code = http.GET();
    String latest = "";
    if ((code == 301 || code == 302) && http.hasHeader("Location")) {
      String loc = http.header("Location");
      int tagIdx = loc.lastIndexOf("/tag/");
      if (tagIdx >= 0) latest = loc.substring(tagIdx + 5);
    }
    http.end();

    if (latest.isEmpty()) {
      displayManager.showOtaProgress("OTA Update", "", "No release found");
      otaRunning = false;
      delay(3000);
      return;
    }

    // Version check
    tag = latest;
    const char* r = latest.c_str();
    if (r[0] == 'v' || r[0] == 'V') r++;
    const char* l = FIRMWARE_VERSION;
    if (l[0] == 'v' || l[0] == 'V') l++;
    int rMaj = 0, rMin = 0, rPat = 0, lMaj = 0, lMin = 0, lPat = 0;
    sscanf(r, "%d.%d.%d", &rMaj, &rMin, &rPat);
    sscanf(l, "%d.%d.%d", &lMaj, &lMin, &lPat);
    if (rMaj * 10000 + rMin * 100 + rPat <= lMaj * 10000 + lMin * 100 + lPat) {
      displayManager.showOtaProgress("OTA Update", "", "Already up to date");
      otaRunning = false;
      delay(3000);
      return;
    }

    // Construct download URL directly — no asset JSON needed
    dlUrl = String("https://github.com/") + OTA_REPO
            + "/releases/download/" + latest
            + "/BambuTagger-AMS-C.ino.bin";
    Serial.printf("[OTA] tag: %s\n", latest.c_str());
    Serial.printf("[OTA] dlUrl: %s\n", dlUrl.c_str());
  }  // ← client, http destroyed here — heap reclaimed

  if (!dlUrl.length()) {
    displayManager.showOtaProgress("OTA Update", "", "No .bin found");
    otaRunning = false;
    delay(3000);
    return;
  }

  displayManager.showOtaProgress("OTA Update", "Downloading...", tag.c_str());

  String finalUrl = dlUrl;
  {
    WiFiClientSecure rc;
    rc.setInsecure();
    rc.setTimeout(10000);
    HTTPClient rh;
    const char* hdrs[] = { "Location" };
    rh.collectHeaders(hdrs, 1);
    rh.begin(rc, dlUrl);
    rh.addHeader("User-Agent", String("BambuTagger-AMS/") + FIRMWARE_VERSION);
    rh.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    int code = rh.GET();
    Serial.printf("[OTA] redirect code: %d\n", code);
    if ((code == 301 || code == 302) && rh.hasHeader("Location")) {
      finalUrl = rh.header("Location");
      Serial.printf("[OTA] CDN URL: %s\n", finalUrl.c_str());
    }
    rh.end();
  }

    // Raw WiFiClientSecure — same as redirect blocks, bypasses IDF TLS enforcement
  String dlHost, dlPath;
  {
    String u = finalUrl;
    if (u.startsWith("https://")) u = u.substring(8);
    int si = u.indexOf('/');
    dlHost = (si >= 0) ? u.substring(0, si) : u;
    dlPath = (si >= 0) ? u.substring(si) : "/";
  }

  WiFiClientSecure dlClient;
  dlClient.setInsecure();
  dlClient.setTimeout(30000);

  displayManager.showOtaProgress("OTA Update", tag.c_str(), "Connecting...");
  if (!dlClient.connect(dlHost.c_str(), 443)) {
    Serial.println("[OTA] CDN connect failed");
    displayManager.showOtaProgress("OTA Update", "", "Connect failed");
    otaRunning = false; delay(3000); return;
  }

  // Send raw HTTP/1.1 GET
  dlClient.print(String("GET ") + dlPath + " HTTP/1.1\r\n");
  dlClient.print(String("Host: ") + dlHost + "\r\n");
  dlClient.print("User-Agent: BambuTagger-AMS/" FIRMWARE_VERSION "\r\n");
  dlClient.print("Connection: close\r\n\r\n");

  // Parse response headers
  int httpStatus = 0, contentLen = -1;
  {
    String line = dlClient.readStringUntil('\n');
    line.trim();
    if (line.length() > 9) httpStatus = line.substring(9, 12).toInt();
    Serial.printf("[OTA] HTTP status: %d\n", httpStatus);
    while (true) {
      line = dlClient.readStringUntil('\n');
      line.trim();
      if (line.isEmpty()) break;
      String lower = line; lower.toLowerCase();
      if (lower.startsWith("content-length:")) {
        String val = line.substring(15);
        val.trim();
        contentLen = val.toInt();
      }
    }
    Serial.printf("[OTA] Content-Length: %d\n", contentLen);
  }

  if (httpStatus != 200 || contentLen <= 0) {
    dlClient.stop();
    displayManager.showOtaProgress("OTA Update", "", "Bad response");
    otaRunning = false; delay(3000); return;
  }

  const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
  esp_ota_handle_t otaHandle = 0;
  if (esp_ota_begin(part, contentLen, &otaHandle) != ESP_OK) {
    dlClient.stop();
    displayManager.showOtaProgress("OTA Update", "", "OTA begin failed");
    otaRunning = false; delay(3000); return;
  }

  uint8_t buf[2048];
  int written = 0, lastPct = -1;
  unsigned long lastData = millis();

  while (written < contentLen) {
    int avail = dlClient.available();
    if (avail > 0) {
      lastData = millis();
      int rd = dlClient.read(buf, min(avail, (int)sizeof(buf)));
      if (rd > 0) {
        esp_ota_write(otaHandle, buf, rd);
        written += rd;
        int pct = (written * 100) / contentLen;
        if (pct != lastPct) {
          Serial.printf("[OTA] progress: %d / %d\n", written, contentLen);
          displayManager.showOtaProgress("OTA Update", tag.c_str(), "Downloading...", pct);
          lastPct = pct;
        }
      }
    } else if (!dlClient.connected()) {
      Serial.printf("[OTA] connection closed at %d/%d\n", written, contentLen);
      break;
    } else {
      if (millis() - lastData > 30000) {
        Serial.printf("[OTA] stalled at %d/%d\n", written, contentLen);
        break;
      }
      delay(1);  // yield proper RTOS tick to Core-0 WiFi stack
    }
  }

  dlClient.stop();
  Serial.printf("[OTA] written: %d / %d\n", written, contentLen);

  if (written == contentLen) {
    esp_err_t err = esp_ota_end(otaHandle);
    if (err == ESP_OK) err = esp_ota_set_boot_partition(part);
    if (err == ESP_OK) {
      displayManager.showOtaProgress("OTA Update", "", "Update OK", 100);
      otaRunning = false;
      delay(2000);
      ESP.restart();
    } else {
      Serial.printf("[OTA] finalize error: 0x%x\n", err);
    }
  }

  displayManager.showOtaProgress("OTA Update", "", "OTA failed");
  otaRunning = false;
  delay(3000);
}