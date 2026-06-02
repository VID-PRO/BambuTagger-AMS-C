#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "config.h"

class RfidManager {
public:
  void begin();
  void loop();
  bool isTagPresent(uint8_t slot);
  bool getSpoolInfo(uint8_t slot, SpoolInfo &info);
  void forceRescan(uint8_t slot);

private:
  void selectReader(uint8_t slot);
  void deselectAll();
  bool readNtag(uint8_t slot, SpoolInfo &info);
  bool authenticateAndRead(uint8_t slot, SpoolInfo &info, uint8_t* uid);
  bool readNtagPages(uint8_t slot, SpoolInfo &info);

  MFRC522* mfrc522[NUM_SLOTS];
  MFRC522_SPI* rfidSPI[NUM_SLOTS];
  SpoolInfo spoolData[NUM_SLOTS];
  unsigned long lastPoll[NUM_SLOTS];
  bool readerOk[NUM_SLOTS];
  uint8_t currentSlot;
  bool initialized;
};

#endif
