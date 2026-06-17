#include "rfid_manager.h"
#include "tag_parser.h"
#include "mbedtls/md.h"

static const uint8_t BAMBU_KDF_SALT[16] = {
  0x9a, 0x75, 0x9c, 0xf2, 0xc4, 0xf7, 0xca, 0xff,
  0x22, 0x2c, 0xb9, 0x76, 0x9b, 0x41, 0xbc, 0x96
};

static void hmacSHA256(const uint8_t* key, size_t kLen,
                       const uint8_t* data, size_t dLen,
                       uint8_t* out32) {
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_setup(&ctx, md, 1);
  mbedtls_md_hmac_starts(&ctx, key, kLen);
  mbedtls_md_hmac_update(&ctx, data, dLen);
  mbedtls_md_hmac_finish(&ctx, out32);
  mbedtls_md_free(&ctx);
}

static void hkdf256(const uint8_t* ikm, size_t ikmLen,
                    const uint8_t* salt, size_t saltLen,
                    const uint8_t* info, size_t infoLen,
                    uint8_t* okm, size_t okmLen) {
  uint8_t prk[32];
  hmacSHA256(salt, saltLen, ikm, ikmLen, prk);
  uint8_t T[32] = { 0 };
  size_t tLen = 0;
  uint8_t ctr = 0;
  size_t done = 0;
  while (done < okmLen) {
    ctr++;
    size_t inLen = tLen + infoLen + 1;
    uint8_t* input = new uint8_t[inLen];
    if (!input) return;
    if (tLen) memcpy(input, T, tLen);
    memcpy(input + tLen, info, infoLen);
    input[tLen + infoLen] = ctr;
    hmacSHA256(prk, 32, input, inLen, T);
    delete[] input;
    tLen = 32;
    size_t n = (32 < (okmLen - done)) ? 32 : (okmLen - done);
    memcpy(okm + done, T, n);
    done += n;
  }
}

static void bambuDeriveKeys(const uint8_t uid[4],
                            uint8_t keysA[16][6], uint8_t keysB[16][6]) {
  static const uint8_t INFO_A[7] = { 'R', 'F', 'I', 'D', '-', 'A', '\0' };
  static const uint8_t INFO_B[7] = { 'R', 'F', 'I', 'D', '-', 'B', '\0' };
  uint8_t okm[96];
  hkdf256(uid, 4, BAMBU_KDF_SALT, 16, INFO_A, 7, okm, 96);
  for (int i = 0; i < 16; i++) memcpy(keysA[i], okm + i * 6, 6);
  hkdf256(uid, 4, BAMBU_KDF_SALT, 16, INFO_B, 7, okm, 96);
  for (int i = 0; i < 16; i++) memcpy(keysB[i], okm + i * 6, 6);
}

void RfidManager::begin() {
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SPI.setFrequency(1000000);

//  Serial.println(F("RFID init:"));
  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    rfidSPI[i] = new MFRC522_SPI(SS_PINS[i], RST_PINS[i], &SPI, SPISettings(1000000, MSBFIRST, SPI_MODE0));
    mfrc522[i] = new MFRC522(rfidSPI[i]);
    mfrc522[i]->PCD_Init();

    byte ver = mfrc522[i]->PCD_ReadRegister(MFRC522::VersionReg);
    readerOk[i] = (ver == 0x92 || ver == 0x91 || ver == 0xB2);
//    Serial.printf("  Slot %d SS=%d RST=%d ver=0x%02X %s\n", i,
//                  SS_PINS[i], RST_PINS[i], ver,
//                  readerOk[i] ? "OK" : "FAIL");

    TagParser::clear(spoolData[i]);
    lastPoll[i] = 0;
  }

  currentSlot = 0;
  initialized = true;
}

void RfidManager::loop() {
  if (!initialized) return;

  unsigned long now = millis();
  if (now - lastPoll[currentSlot] < RFID_POLL_INTERVAL_MS) return;

  if (!readerOk[currentSlot]) {
    currentSlot = (currentSlot + 1) % NUM_SLOTS;
    return;
  }

  lastPoll[currentSlot] = now;

  static unsigned long lastCycle = 0;
  if (now - lastCycle > 1000) {
    lastCycle = now;
    // Serial.printf("RFID: poll slot %d (SS=%d)\n", currentSlot, SS_PINS[currentSlot]);
  }

  SpoolInfo newInfo;
  TagParser::clear(newInfo);

  bool tagFound = readNtag(currentSlot, newInfo);

  if (tagFound) {
    bool wasPresent = spoolData[currentSlot].present;
    bool sameTag = (strcmp(newInfo.uid, spoolData[currentSlot].uid) == 0);

    if (!wasPresent || !sameTag) {
      spoolData[currentSlot] = newInfo;
      spoolData[currentSlot].lastSeen = now;
      spoolData[currentSlot].present = true;
    } else {
      spoolData[currentSlot].lastSeen = now;
    }
  } else {
    if (spoolData[currentSlot].present) {
      if (now - spoolData[currentSlot].lastSeen > RFID_DEBOUNCE_MS) {
        spoolData[currentSlot].present = false; // keep last known data for LED
      }
    }
  }

  currentSlot = (currentSlot + 1) % NUM_SLOTS;
}

bool RfidManager::readNtag(uint8_t slot, SpoolInfo &info) {
  MFRC522* reader = mfrc522[slot];

  if (!reader->PICC_IsNewCardPresent()) return false;
  if (!reader->PICC_ReadCardSerial()) return false;

  MFRC522::PICC_Type piccType = reader->PICC_GetType(reader->uid.sak);
  // Serial.printf("Slot %d: tag type=%s (SAK=0x%02X)\n",
  //               slot, reader->PICC_GetTypeName(piccType), reader->uid.sak);

  char uidStr[16];
  uint8_t uidLen = reader->uid.size;
  if (uidLen > 7) uidLen = 7;
  uidStr[0] = '\0';
  for (uint8_t b = 0; b < uidLen; b++) {
    snprintf(uidStr + b * 2, 3, "%02X", reader->uid.uidByte[b]);
  }

  bool success = false;
  if (piccType == MFRC522::PICC_TYPE_MIFARE_1K || piccType == MFRC522::PICC_TYPE_MIFARE_4K) {
    success = authenticateAndRead(slot, info, reader->uid.uidByte);
    if (!success) success = readNtagPages(slot, info); // fallback: NDEF on MIFARE
  } else if (piccType == MFRC522::PICC_TYPE_MIFARE_UL || piccType == MFRC522::PICC_TYPE_MIFARE_UL) {
    // NTAG/Ultralight fallback — try reading without auth
    success = readNtagPages(slot, info);
  } else {
//    Serial.printf("Slot %d: unsupported tag type\n", slot);
  }

  strncpy(info.uid, uidStr, sizeof(info.uid) - 1);

  if (success) {
    info.present = true;
  } else {
    info.present = true;
    strcpy(info.materialType, "Unknown tag");
    info.tagReadSuccess = false;
  }

  reader->PICC_HaltA();

  return true;
}

bool RfidManager::authenticateAndRead(uint8_t slot, SpoolInfo &info, uint8_t* uid) {
  MFRC522* reader = mfrc522[slot];
  // Serial.printf("Slot %d: authenticateAndRead enter\n", slot);

  uint8_t keysA[16][6], keysB[16][6];
  bambuDeriveKeys(uid, keysA, keysB);

  // Serial.printf("Slot %d: UID=%02X%02X%02X%02X\n", slot,
  //               uid[0], uid[1], uid[2], uid[3]);

  MFRC522::MIFARE_Key keyDef;
  for (byte i = 0; i < 6; i++) keyDef.keyByte[i] = 0xFF;

  const uint16_t bufferSize = 1024;
  uint8_t* dataBuffer = new uint8_t[bufferSize];
  if (!dataBuffer) return false;
  memset(dataBuffer, 0, bufferSize);

  // Block 0 = UID
  memcpy(dataBuffer, uid, 4);

  uint16_t bytesRead = 0;

  for (uint8_t sector = 0; sector < 16; sector++) {
    uint8_t trailerBlock = sector * 4 + 3;
    uint8_t dataBlock = sector * 4;

    MFRC522::MIFARE_Key kA, kB;
    memcpy(kA.keyByte, keysA[sector], 6);
    memcpy(kB.keyByte, keysB[sector], 6);

    MFRC522::StatusCode authStatus = MFRC522::STATUS_ERROR;

    authStatus = reader->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &kA, &(reader->uid));
    if (authStatus != MFRC522::STATUS_OK) { reader->PCD_AntennaOff(); delay(2); reader->PCD_AntennaOn(); delay(2); }
    else goto authOk2;

    authStatus = reader->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &kB, &(reader->uid));
    if (authStatus != MFRC522::STATUS_OK) { reader->PCD_AntennaOff(); delay(2); reader->PCD_AntennaOn(); delay(2); }
    else goto authOk2;

    authStatus = reader->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &keyDef, &(reader->uid));
    if (authStatus != MFRC522::STATUS_OK) { reader->PCD_AntennaOff(); delay(2); reader->PCD_AntennaOn(); delay(2); }
    else goto authOk2;

    authStatus = reader->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &keyDef, &(reader->uid));
    if (authStatus != MFRC522::STATUS_OK) { reader->PCD_AntennaOff(); delay(2); reader->PCD_AntennaOn(); delay(2); }

    if (bytesRead > 0) break;
    continue;

authOk2:
    uint8_t startBlock = (sector == 0) ? (dataBlock + 1) : dataBlock;
    uint8_t endBlock = dataBlock + 3;

    for (uint8_t block = startBlock; block < endBlock; block++) {
      byte readBuf[18];
      byte readSize = sizeof(readBuf);
      MFRC522::StatusCode status = reader->MIFARE_Read(block, readBuf, &readSize);
      if (status != MFRC522::STATUS_OK) break;

      memcpy(dataBuffer + block * 16, readBuf, 16);
      bytesRead += 16;
    }
  }

  reader->PCD_StopCrypto1();

//  Serial.printf("Slot %d: MIFARE auth done, bytesRead=%d\n", slot, bytesRead);

  // Parse using Touch fixed-block offsets (Bambu Lab RFID Tag Guide)
  info.tagReadSuccess = false;

  // Block 1 bytes 8-15 = material index (e.g. "GFA00")
  memcpy(info.materialType, dataBuffer + 1 * 16 + 8, 8);
  info.materialType[8] = '\0';
  for (int i = 7; i >= 0; i--) {
    if (info.materialType[i] == ' ' || info.materialType[i] == '\0') info.materialType[i] = '\0';
    else break;
  }

  // Block 4 = detailed type string (e.g. "PLA Basic")
  memcpy(info.detailedType, dataBuffer + 4 * 16, 16);
  info.detailedType[16] = '\0';
  for (int i = 15; i >= 0; i--) {
    if (info.detailedType[i] == ' ' || info.detailedType[i] == '\0') info.detailedType[i] = '\0';
    else break;
  }

  // Use first word of detailed type as material name, tag label as "Bambu"
  char typeName[32];
  strncpy(typeName, info.detailedType, sizeof(typeName) - 1);
  typeName[sizeof(typeName) - 1] = '\0';
  char* sp = strchr(typeName, ' ');
  if (sp) *sp = '\0';
  if (typeName[0]) strncpy(info.materialType, typeName, sizeof(info.materialType) - 1);
  strcpy(info.detailedType, "Bambu");
  // Serial.printf("Slot %d: block4 raw=", slot);
  // for (int i = 0; i < 16; i++) Serial.printf("%02X ", dataBuffer[4 * 16 + i]);
  // Serial.printf(" sub='%s'\n", info.detailedType);

  // Block 5 bytes 0-3 = RGBA → force alpha to FF for printer format
  snprintf(info.colorHex, sizeof(info.colorHex), "%02X%02X%02XFF",
           dataBuffer[5 * 16 + 0], dataBuffer[5 * 16 + 1],
           dataBuffer[5 * 16 + 2]);

  // Block 5 bytes 4-5 = spool weight (LE uint16)
  info.totalGrams = (uint16_t)dataBuffer[5 * 16 + 4] | ((uint16_t)dataBuffer[5 * 16 + 5] << 8);
  info.remainingGrams = info.totalGrams;

  // Block 6 bytes 8-9 = max nozzle temp, bytes 10-11 = min nozzle temp (LE uint16)
  info.nozzleTempMin = (uint16_t)dataBuffer[6 * 16 + 10] | ((uint16_t)dataBuffer[6 * 16 + 11] << 8);
  info.nozzleTempMax = (uint16_t)dataBuffer[6 * 16 + 8]  | ((uint16_t)dataBuffer[6 * 16 + 9]  << 8);

  info.tagReadSuccess = (info.materialType[0] != '\0');
  bool result = info.tagReadSuccess;

  if (result) {
//    Serial.printf("Slot %d: idx=%s type=%s color=%s weight=%d\n", slot,
//                   info.materialType, info.detailedType, info.colorHex, info.totalGrams);
  } else {
//    Serial.printf("Slot %d: parse FAIL (bytesRead=%d)\n", slot, bytesRead);
  }

  delete[] dataBuffer;
  return result;
}

bool RfidManager::readNtagPages(uint8_t slot, SpoolInfo &info) {
  MFRC522* reader = mfrc522[slot];

  uint16_t bufferSize = 900;
  uint8_t* dataBuffer = new uint8_t[bufferSize];
  if (!dataBuffer) return false;

  uint16_t bytesRead = 0;

  for (uint8_t page = 4; page < 230 && bytesRead < bufferSize; page += 4) {
    byte readBuf[18];
    byte readSize = sizeof(readBuf);
    MFRC522::StatusCode status = reader->MIFARE_Read(page, readBuf, &readSize);
    if (status != MFRC522::STATUS_OK) continue;

    byte bytesThisBlock = (readSize > 16) ? 16 : readSize;

    for (byte b = 0; b < bytesThisBlock && bytesRead < bufferSize; b++) {
      dataBuffer[bytesRead++] = readBuf[b];
    }
  }

//  Serial.printf("Slot %d: NTAG read, bytesRead=%d\n", slot, bytesRead);

  bool result = TagParser::parse(dataBuffer, bytesRead, info.uid, info);
  if (result) {
    info.present = true;
  } else {
    info.present = true;
    info.tagReadSuccess = false;
  }

  delete[] dataBuffer;
  return result;
}

bool RfidManager::isTagPresent(uint8_t slot) {
  if (slot >= NUM_SLOTS) return false;
  return spoolData[slot].present;
}

bool RfidManager::getSpoolInfo(uint8_t slot, SpoolInfo &info) {
  if (slot >= NUM_SLOTS) return false;
  info = spoolData[slot];
  return spoolData[slot].present;
}

void RfidManager::forceRescan(uint8_t slot) {
  if (slot >= NUM_SLOTS) return;
  lastPoll[slot] = 0;
}

void RfidManager::selectReader(uint8_t slot) {
  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    digitalWrite(SS_PINS[i], HIGH);
  }
  delayMicroseconds(50);
  if (slot < NUM_SLOTS) {
    digitalWrite(SS_PINS[slot], LOW);
  }
}

void RfidManager::deselectAll() {
  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    digitalWrite(SS_PINS[i], HIGH);
  }
}
