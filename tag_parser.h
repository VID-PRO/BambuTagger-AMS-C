#ifndef TAG_PARSER_H
#define TAG_PARSER_H

#include <Arduino.h>
#include "config.h"

// --- Bambu Lab AMS Tag Constants ---
#define BAMBU_TAG_MAGIC          0x5A
#define BAMBU_TAG_VERSION        0x01
#define BAMBU_HEADER_SIZE        8
#define BAMBU_TLV_TYPE_MATERIAL  0x01
#define BAMBU_TLV_TYPE_COLOR     0x02
#define BAMBU_TLV_TYPE_WEIGHT    0x03
#define BAMBU_TLV_TYPE_BATCH     0x04
#define BAMBU_TLV_TYPE_MFG       0x05
#define BAMBU_TLV_TYPE_UNKNOWN   0xFF

// Material type lookup (Bambu Lab IDs)
struct MaterialEntry {
  uint8_t id;
  const char* name;
};

const MaterialEntry MATERIAL_TABLE[] = {
  {0x00, "PLA Basic"},
  {0x01, "PLA Matte"},
  {0x02, "PLA Silk"},
  {0x03, "PLA Metal"},
  {0x04, "PLA Tough"},
  {0x05, "PLA-CF"},
  {0x06, "PLA-GF"},
  {0x07, "PLA Marble"},
  {0x08, "PLA Wood"},
  {0x09, "PLA Glow"},
  {0x0A, "PLA Sparkle"},
  {0x0B, "PLA Rainbow"},
  {0x0C, "PLA Trans"},
  {0x0D, "PLA Dual"},
  {0x20, "PETG Basic"},
  {0x21, "PETG-CF"},
  {0x22, "PETG-GF"},
  {0x23, "PETG Translucent"},
  {0x40, "ABS"},
  {0x41, "ASA"},
  {0x60, "TPU"},
  {0x80, "PC"},
  {0x81, "PA"},
  {0x82, "PA-CF"},
  {0x83, "PA-GF"},
  {0xA0, "PVA"},
  {0xC0, "Support"},
  {0xC1, "Support-W"},
  {0xFF, "Unknown"}
};

class TagParser {
public:
  static void clear(SpoolInfo &info);
  static bool parse(uint8_t* data, uint16_t length, const char* uid, SpoolInfo &info);
  static bool parseRawNTAG(uint8_t* data, uint16_t length, const char* uid, SpoolInfo &info);
  static bool parseTigerTag(uint8_t* data, uint16_t length, const char* uid, SpoolInfo &info);
  static void dumpHex(uint8_t* data, uint16_t length, char* out, uint16_t outLen);

private:
  static bool parseBambuTLV(uint8_t* data, uint16_t length, SpoolInfo &info);
  static const char* materialName(uint8_t id);
  static void bytesToHex(uint8_t* bytes, uint8_t len, char* out);
};

#endif
