#include "bambu_printer.h"

BambuPrinter* BambuPrinter::instance = nullptr;

void BambuPrinter::begin(const SystemConfig &cfg) {
  config = cfg;
  state = PRINTER_DISCONNECTED;
  printerOnline = false;
  lastReconnectAttempt = 0;
  lastStatusRequest = 0;
  instance = this;
  amsDetected = false;
  amsExistBits = 0;

  for (uint8_t i = 0; i < MAX_DETECTED_AMS; i++) {
    detectedAms[i].id = i;
    detectedAms[i].connected = false;
    for (uint8_t t = 0; t < 4; t++) {
      detectedAms[i].trays[t][0] = '\0';
      detectedAms[i].trayColors[t][0] = '\0';
      detectedAms[i].trayRemain[t] = 0;
    }
  }

  tcpClient = nullptr;
  tlsClient = nullptr;
  mqttClient = nullptr;

  if (!config.mqttEnabled) return;

  if (config.mqttUseTLS) {
    tlsClient = new WiFiClientSecure();
    tlsClient->setInsecure();
    mqttClient = new PubSubClient(*tlsClient);
  } else {
    tcpClient = new WiFiClient();
    mqttClient = new PubSubClient(*tcpClient);
  }

  snprintf(mqttClientId, sizeof(mqttClientId), "BambuTagger-%06X", (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF));
  mqttClient->setServer(config.printerIP, config.printerPort);
  mqttClient->setCallback(staticMqttCallback);
  mqttClient->setBufferSize(MQTT_BUFFER_SIZE);
}

void BambuPrinter::update() {
  if (!config.mqttEnabled || !mqttClient) return;

  if (!mqttClient->connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 15000) {
      lastReconnectAttempt = now;
      reconnect();
    }
    state = PRINTER_DISCONNECTED;
    printerOnline = false;
    return;
  }

  mqttClient->loop();
  state = PRINTER_CONNECTED;

  unsigned long now = millis();
  if (now - lastStatusRequest > 10000) {
    lastStatusRequest = now;
    requestPrinterStatus();
  }
}

void BambuPrinter::reconnect() {
  if (!config.mqttEnabled || !mqttClient || !config.printerIP[0]) return;

  state = PRINTER_CONNECTING;

  // Set TCP timeout to avoid long blocks when printer is offline
  if (config.mqttUseTLS && tlsClient) tlsClient->setTimeout(2);
  else if (tcpClient) tcpClient->setTimeout(2);

  char username[48];
  snprintf(username, sizeof(username), "bblp");
  if (mqttClient->connect(mqttClientId, username, config.printerAccessCode)) {
    state = PRINTER_CONNECTED;

    char topic[128];
    snprintf(topic, sizeof(topic), "%s/%s/report",
             config.mqttTopicPrefix, config.printerSerial);
    mqttClient->subscribe(topic);

    char reqTopic[128];
    snprintf(reqTopic, sizeof(reqTopic), "%s/%s/request",
             config.mqttTopicPrefix, config.printerSerial);
    char payload2[128];
    snprintf(payload2, sizeof(payload2),
             "{\"info\":{\"sequence_id\":\"0\",\"command\":\"get_version\"}}");
    mqttClient->publish(reqTopic, payload2);
  } else {
    state = PRINTER_ERROR;
  }
}

void BambuPrinter::sendBmeData(float temp, float humidity) {
  if (!config.mqttEnabled || !mqttClient || !mqttClient->connected()) return;

  char topic[128];
  snprintf(topic, sizeof(topic), "%s/%s/request",
           config.mqttTopicPrefix, config.printerSerial);

  char payload[256];
  snprintf(payload, sizeof(payload),
           "{\"print\":{\"command\":\"ams_user_setting\","
           "\"sequence_id\":\"%lu\","
           "\"ams_id\":%d,"
           "\"ams_user_temp\":%.1f,"
           "\"ams_user_humidity\":%.0f}}",
           millis(), config.amsUnit, temp, humidity);

  mqttClient->publish(topic, payload);
}

void BambuPrinter::sendAmsGetRfid(uint8_t trayId) {
  if (!config.mqttEnabled || !mqttClient || !mqttClient->connected()) return;

  char topic[128];
  snprintf(topic, sizeof(topic), "%s/%s/request",
           config.mqttTopicPrefix, config.printerSerial);

  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"print\":{\"sequence_id\":\"%lu\","
           "\"command\":\"ams_get_rfid\","
           "\"ams_id\":%d,"
           "\"slot_id\":%d}}",
           millis(), config.amsUnit, trayId);

  mqttClient->publish(topic, payload);
}

void BambuPrinter::sendSpoolData(uint8_t slot, const SpoolInfo &info) {
  if (!config.mqttEnabled || !mqttClient || !mqttClient->connected()) return;

  char ttype[32];
  const char* src = info.materialType[0] ? info.materialType : info.detailedType;
  strncpy(ttype, src, sizeof(ttype) - 1);
  ttype[sizeof(ttype) - 1] = '\0';
  char* sp = strchr(ttype, ' ');
  if (sp) *sp = '\0';

  if (info.materialType[0] == 'G' && info.materialType[1] == 'F') {
    if (strncmp(info.materialType, "GFA", 3) == 0 || strncmp(info.materialType, "GFB", 3) == 0 ||
        strncmp(info.materialType, "GFC", 3) == 0 || strncmp(info.materialType, "GFD", 3) == 0 ||
        strncmp(info.materialType, "GFE", 3) == 0) strcpy(ttype, "PLA");
    else if (strncmp(info.materialType, "GFG", 3) == 0) strcpy(ttype, "PETG");
    else if (strncmp(info.materialType, "GFH", 3) == 0 || strncmp(info.materialType, "GFI", 3) == 0) strcpy(ttype, "ABS");
    else if (strncmp(info.materialType, "GFJ", 3) == 0) strcpy(ttype, "ASA");
    else if (strncmp(info.materialType, "GFK", 3) == 0) strcpy(ttype, "TPU");
    else if (strncmp(info.materialType, "GFL", 3) == 0) strcpy(ttype, "PLA");
  }
  char payload[512];
  snprintf(payload, sizeof(payload),
           "{\"print\":{"
           "\"sequence_id\":\"%lu\","
           "\"command\":\"ams_filament_setting\","
           "\"ams_id\":%d,"
           "\"tray_id\":%d,"
           "\"tray_info_idx\":\"%s\","
           "\"tray_color\":\"%s\","
           "\"nozzle_temp_min\":%d,"
           "\"nozzle_temp_max\":%d,"
           "\"tray_type\":\"%s\"}}",
           millis(),
           config.amsUnit,
           (slot % 4),
           info.materialType,
           info.colorHex,
           info.nozzleTempMin,
           info.nozzleTempMax,
           ttype);

  char topic[128];
  snprintf(topic, sizeof(topic), "%s/%s/request",
           config.mqttTopicPrefix, config.printerSerial);

  // Serial.printf("SEND: %s\n", payload);
  mqttClient->publish(topic, payload);
}

void BambuPrinter::requestPrinterStatus() {
  if (!config.mqttEnabled || !mqttClient || !mqttClient->connected()) return;

  char topic[128];
  snprintf(topic, sizeof(topic), "%s/%s/request",
           config.mqttTopicPrefix, config.printerSerial);

  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"info\":{\"sequence_id\":\"0\",\"command\":\"get_version\"}}");
  mqttClient->publish(topic, payload);

  snprintf(payload, sizeof(payload),
           "{\"pushing\":{\"sequence_id\":\"0\",\"command\":\"pushall\",\"version\":1,\"push_target\":1}}");
  mqttClient->publish(topic, payload);
}

void BambuPrinter::mqttCallback(char* topic, byte* payload, unsigned int length) {
  printerOnline = true;

  // Serial.printf("MQTT [%s] %u\n", topic, length);
  // Serial.write(payload, length);  // mute raw JSON
  // Serial.println();

  if (length >= MQTT_BUFFER_SIZE) return;

  DynamicJsonDocument doc(MQTT_BUFFER_SIZE);
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) {
//    Serial.printf("JSON deser error: %s\n", err.c_str());
    return;
  }

  parseReport(doc);
}

void BambuPrinter::parseReport(JsonDocument &doc) {
  JsonObject printObj = doc["print"];
  JsonObject infoObj = doc["info"];

  if (printObj) {
    JsonObject amsObj = printObj["ams"];
    if (amsObj) {
      if (amsObj.containsKey("ams_exist_bits")) {
        JsonVariant v = amsObj["ams_exist_bits"];
        amsExistBits = v.is<const char*>() ? (uint8_t)strtoul(v.as<const char*>(), nullptr, 10) : v.as<uint8_t>();
      }
      JsonArray arr = amsObj["ams"];
      if (arr) {
        for (JsonVariant v : arr) {
          JsonObject a = v.as<JsonObject>();
          const char* idStr = a["id"].as<const char*>();
          uint8_t id = (idStr && idStr[0] >= '0' && idStr[0] <= '9') ? (uint8_t)atoi(idStr) : 99;
          // Serial.printf("AMS obj id raw='%s' -> %d\n", idStr ? idStr : "(null)", id);
          if (id < MAX_DETECTED_AMS) {
            amsExistBits |= (1 << id);
            detectedAms[id].temperature = a["temp"].as<float>();
            detectedAms[id].humidity = a["humidity_raw"].as<float>();
          }
          JsonArray trays = a["tray"];
          if (trays) {
            for (JsonObject t : trays) {
                const char* tidStr = t["id"].as<const char*>();
                uint8_t tid = (tidStr && tidStr[0] >= '0' && tidStr[0] <= '9') ? (uint8_t)atoi(tidStr) : 99;
                if (tid >= 4) continue;
                const char* mat = t["tray_info_idx"] | "";
                if (mat) strncpy(detectedAms[id].trays[tid], mat, 31);
                const char* col = t["tray_color"] | "";
                if (col) strncpy(detectedAms[id].trayColors[tid], col, 8);
                const char* ttype = t["tray_type"] | "";
                if (ttype) strncpy(detectedAms[id].trayTypes[tid], ttype, 15);
                // remain: reported as 0-100 integer by the printer
                JsonVariant rem = t["remain"];
                if (!rem.isNull()) {
                  int remVal = rem.as<int>();
                  if (remVal < 0) remVal = 0;
                  if (remVal > 100) remVal = 100;
                  detectedAms[id].trayRemain[tid] = (uint8_t)remVal;
                }
          }
        }
        if (id < MAX_DETECTED_AMS) {
          // Serial.printf("AMS%d: tray[0]=%s/%s color=%s remain=%d%%\n", id,
          //               detectedAms[id].trayTypes[0], detectedAms[id].trays[0],
          //               detectedAms[id].trayColors[0], detectedAms[id].trayRemain[0]);
        }
      }
    }
  }
  }

  if (infoObj) {
    JsonArray modules = infoObj["module"];
    if (modules) {
      for (JsonVariant mv : modules) {
        JsonObject m = mv.as<JsonObject>();
        const char* name = m["name"] | "";
        // Serial.printf("Module: name='%s'\n", name);
        if (strstr(name, "ams") || strchr(name, '/')) {
          const char* slash = strchr(name, '/');
          if (slash) {
            uint8_t id = (uint8_t)strtoul(slash + 1, nullptr, 10);
            if (id < MAX_DETECTED_AMS) {
              amsExistBits |= (1 << id);
              const char* fw = m["sw_ver"] | "";
              const char* prod = m["product_name"] | "";
              const char* sn = m["sn"] | "";
              if (fw[0]) strncpy(detectedAms[id].fwVer, fw, 31);
              if (prod[0]) strncpy(detectedAms[id].productName, prod, 31);
              if (sn[0]) strncpy(detectedAms[id].serial, sn, 31);
              if (!detectedAms[id].productName[0]) snprintf(detectedAms[id].productName, 31, "AMS %c", 'A' + id);
              // Serial.printf("AMS%d: fw='%s' prod='%s' sn='%s'\n", id, fw, prod, sn);
            }
          }
        }
      }
    }
  }

  if (amsExistBits > 0) {
    amsDetected = true;
    for (uint8_t i = 0; i < MAX_DETECTED_AMS; i++) {
      detectedAms[i].connected = (amsExistBits & (1 << i)) != 0;
      if (detectedAms[i].connected && !detectedAms[i].productName[0]) {
        snprintf(detectedAms[i].productName, 31, "AMS %c", 'A' + i);
      }
    }
  }
}

bool BambuPrinter::isConnected() const {
  return mqttClient && mqttClient->connected();
}

PrinterState BambuPrinter::getState() const {
  return state;
}

bool BambuPrinter::isAmsDetected(uint8_t amsId) const {
  if (amsId >= MAX_DETECTED_AMS) return false;
  return amsDetected && detectedAms[amsId].connected;
}

uint8_t BambuPrinter::getAmsExistBits() const {
  return amsExistBits;
}

uint8_t BambuPrinter::getDetectedAmsCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_DETECTED_AMS; i++) {
    if (detectedAms[i].connected) count++;
  }
  return count;
}

const char* BambuPrinter::getAmsTrayMaterial(uint8_t amsId, uint8_t trayId) const {
  if (amsId >= MAX_DETECTED_AMS || trayId >= 4) return "";
  return detectedAms[amsId].trays[trayId];
}

const char* BambuPrinter::getAmsTrayType(uint8_t amsId, uint8_t trayId) const {
  if (amsId >= MAX_DETECTED_AMS || trayId >= 4) return "";
  return detectedAms[amsId].trayTypes[trayId];
}

const char* BambuPrinter::getAmsTrayColor(uint8_t amsId, uint8_t trayId) const {
  if (amsId >= MAX_DETECTED_AMS || trayId >= 4) return "";
  return detectedAms[amsId].trayColors[trayId];
}

uint8_t BambuPrinter::getAmsTrayRemain(uint8_t amsId, uint8_t trayId) const {
  if (amsId >= MAX_DETECTED_AMS || trayId >= 4) return 0;
  return detectedAms[amsId].trayRemain[trayId];
}

const char* BambuPrinter::getAmsFwVer(uint8_t amsId) const {
  if (amsId >= MAX_DETECTED_AMS) return "";
  return detectedAms[amsId].fwVer;
}

const char* BambuPrinter::getAmsProductName(uint8_t amsId) const {
  if (amsId >= MAX_DETECTED_AMS) return "";
  return detectedAms[amsId].productName;
}

const char* BambuPrinter::getAmsSerial(uint8_t amsId) const {
  if (amsId >= MAX_DETECTED_AMS) return "";
  return detectedAms[amsId].serial;
}

float BambuPrinter::getAmsTemperature(uint8_t amsId) const {
  if (amsId >= MAX_DETECTED_AMS) return 0;
  return detectedAms[amsId].temperature;
}

float BambuPrinter::getAmsHumidity(uint8_t amsId) const {
  if (amsId >= MAX_DETECTED_AMS) return 0;
  return detectedAms[amsId].humidity;
}

void BambuPrinter::staticMqttCallback(char* topic, byte* payload, unsigned int length) {
  if (instance) instance->mqttCallback(topic, payload, length);
}
