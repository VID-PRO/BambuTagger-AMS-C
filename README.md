# <img alt="logo" src="Logo/bambutagger.png" height="36" />  BambuTagger-AMS

Multi-spool NFC tag reader for Bambu Lab printers. Reads 4 Bambu Lab filament spool tags via RC522, displays live printer AMS tray data over MQTT, and sends RFID tag data to the printer/BMCU. Fully configurable via web interface with automatic AP fallback and OTA firmware updates.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/G8M220JASY)

<p align="center">
<img src="Pics/printer.png" />
<img src="Pics/status.png" />
<img src="Pics/pcb.png" />
</p>

## Features

- **4x RC522** on shared SPI bus polling MIFARE Classic 1K + NTAG tags via `MFRC522-spi-i2c-uart-async`
- **Multi-tag auto-detect** — MIFARE Classic 1K (Bambu Lab) or NTAG (SpoolEase), auto-routed to correct parser
- **Nested NDEF recursion** — auto-parses NDEF-in-NDEF payloads (OpenTag3D format)
- **TigerTag binary parser** — native TigerTag v2.1 protocol support (ID TigerTag magic detection, material, color, weight, temps)
- **OpenTag3D binary parser** — MIME record type "application/opentag3d", fixed-offset binary format
- **OpenSpool JSON parser** — NDEF text records with JSON payload, `protocol` key auto-detect
- **HKDF-SHA256 key derivation** — derives per-sector MIFARE keys from the tag UID using Bambu's KDF salt
- **Live printer AMS sync** — reads tray data (material, color, type) from the printer over MQTT
- **Bambu BMCU support** — sends `ams_filament_setting` with correct `tray_type`, `tray_color`, `nozzle_temp_min/max`
- **Web interface** — 3-tab SPA: Status (merged slots + Sub type + color swatches), Printer Config, WiFi Config
- **TFT display** — boot splash, live AMS tray data, OTA progress bar, BME280 temp/humidity, MQTT+PTR status
- **4x WS2812 LEDs** — per-slot color from printer AMS tray data (live MQTT sync)
- **Boot splash**: 2-second logo display from `Logo/splash.png`
- **MQTT bridge** — subscribes to printer status, publishes `ams_filament_setting` commands
- **Auto AP fallback** — captive portal on `192.168.4.1` when WiFi is unavailable
- **OTA updates** — one-click firmware update from GitHub Releases, TFT progress bar, web overlay with status
- **GitHub Actions** — CI build on commit, merged binary + OTA binary release on version tags

## Hardware

- **ESP32** Dev Module
- **4x RC522** RFID/NFC readers (SPI, shared bus)
- **4x WS2812** addressable LEDs (daisy-chained, single data pin)
- **240x240 1.3" TFT** ST7789VW SPI display
- **BME280** temperature/humidity sensor (I2C)

### Wiring

| Component       | ESP32 Pin |
|-----------------|-----------|
| **SPI MOSI**    | GPIO 23 |
| **SPI MISO**    | GPIO 19 |
| **SPI SCK**     | GPIO 18 |
| RC522 #1 SS     | GPIO 13 |
| RC522 #2 SS     | GPIO 12 |
| RC522 #3 SS     | GPIO 14 |
| RC522 #4 SS     | GPIO 27 |
| RC522 #1 RST    | GPIO 26 |
| RC522 #2 RST    | GPIO 25 |
| RC522 #3 RST    | GPIO 33 |
| RC522 #4 RST    | GPIO 32 |
| **WS2812 data** | GPIO 15 |
| **TFT SDA**     | GPIO 21 |
| **TFT SCL**     | GPIO 22 |
| **TFT DC**      | GPIO 4  |
| **TFT RES**     | GPIO 5  |
| **TFT BLK**     | GPIO 2  |

All RC522 share the same SPI bus (MOSI, MISO, SCK). Each has its own SS and RST pin.

### Slot Mapping

| RC522 # | SS Pin | Slot | WS2812 Pixel |
|---------|--------|------|--------------|
| 1 | 13 | 1 | 0 |
| 2 | 12 | 2 | 1 |
| 3 | 14 | 3 | 2 |
| 4 | 27 | 4 | 3 |

## Software Setup (Arduino IDE)

1. Install **ESP32 board package**:  
   File → Preferences → Additional Board Manager URLs:  
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`  
   Then Tools → Board → Boards Manager → search "ESP32" → install.

2. Install required libraries via **Tools → Manage Libraries**:
   - **MFRC522-spi-i2c-uart-async** — multi-reader SPI sharing (not standard MFRC522)
   - **Adafruit NeoPixel** by Adafruit (v1.12+)
   - **Adafruit GFX Library** by Adafruit (v1.11+)
    - **Adafruit ST7735 and ST7789 Library** by Adafruit (v1.10+)
   - **PubSubClient** by Nick O'Leary (v2.8+)
   - **ArduinoJson** by Benoit Blanchon (v6.x or v7.x)
    - **Adafruit BME280 Library** by Adafruit
    - **mbedTLS** — bundled with ESP32 core, used for HKDF-SHA256

3. Open `BambuTagger-AMS.ino`, select **ESP32 Dev Module** as board, and upload.

## WiFi & AP Mode

On first boot (or if saved WiFi credentials are invalid), the device automatically opens an access point:

| Scenario | Behavior |
|----------|----------|
| No WiFi configured | Opens AP immediately |
| WiFi connection fails | Opens AP after 15 seconds |
| AP active, credentials exist | Retries STA connection every 30 seconds |
| STA connects while AP active | Closes AP, switches to normal mode |

- **SSID**: Device name (default: `BambuTagger-AMS`)
- **Security**: Open (no password)
- **IP**: `192.168.4.1`
- **Captive portal**: DNS redirects all domains to the config page

## Web Interface

Available at `http://<esp32-ip>` on your network, or `http://192.168.4.1` in AP mode.

### Status Tab
- **Merged Slot Status** — each slot shows AMS data (Type, Sub, Material, Color) + scanned tag data
- **Color swatches** — 36x36px right-aligned per slot
- **Tag info row** — `Tag: Bambu - PLA · C12E1FFF · 1000g/1000g`
- **Printer AMS Cards** — all detected AMS units with tray grids
- **Update Firmware** — one-click OTA from GitHub Releases

### Printer Config Tab
- Printer IP, Port (default 8883), Access Code, Serial Number
- **AMS Unit selector** (A/B/C/D)

### WiFi Config Tab
- SSID, Password, Device Name

### Footer
- Sticky footer: `© 2026 by VID-PRO` with link to [www.vid-pro.de](https://www.vid-pro.de)

## TFT Display (240x240 1.3" ST7789VW)

```
┌────────────────────────────────────────┐
│ Device Name                      WiFi  │  ← status bar (white bg)
├────────────────────────────────────────┤
│ 1: PLA          [■] #C0C0C0FF   100%   │  ← color swatch + percentage
│ 2: empty                               │
│ 3: empty                               │
│ 4: empty                               │
├────────────────────────────────────────┤
│ 22C        45%                         │  ← BME280 (orange/blue)
│ MQTT:OK                         PTR:OK │  ← status line
└────────────────────────────────────────┘
```

OTA progress shown on TFT with header/footer preserved:
"OTA Update" → "Downloading..." → "Flashing... 45%" → auto-reboot

## Printer Communication

### Subscribe
- **Topic**: `device/<serial>/report`
- **Data**: `push_status` (periodic, ~3KB), `get_version` responses

### Publish
- **Topic**: `device/<serial>/request`
- **`ams_filament_setting`** — structure:
  ```json
  {
    "print": {
      "sequence_id": "0",
      "command": "ams_filament_setting",
      "ams_id": 0,
      "tray_id": 0,
      "tray_info_idx": "GFA00",
      "tray_color": "RRGGBBFF",
      "nozzle_temp_min": 190,
      "nozzle_temp_max": 230,
      "tray_type": "PLA"
    }
  }
  ```
- `tray_type` derived from filament index prefix (GFA→PLA, GFG→PETG, etc.)
- `tray_color` forced to RRGGBBFF format
- `nozzle_temp_min/max` from tag block 6

## Tag Format & Reading

### Bambu Lab (MIFARE Classic 1K)
Tag: `Bambu - PLA · C12E1FFF · 1000g/1000g`

### SpoolEase (NTAG, NDEF URI)
Tag: `SpoolEase - PLA · 000000FF · 1000g/1036g`

### TigerTag (NTAG, binary v2.1)
Tag: `TigerTag - ASA-AF · F078B4FF · 1000g/1000g`

### OpenSpool (NTAG, NDEF JSON)
Tag: `OpenSpool - ASA-AF · F078B4FF · 1000g/1000g`

### OpenTag3D (NTAG, MIME binary)
Tag: `OpenTag3D - ASA-AF · F078B4FF · 1000g/1000g`

Bambu Lab uses **MIFARE Classic 1K** tags with fixed block offsets:

| Block | Content |
|-------|---------|
| 0 | UID (4 bytes) |
| 1 | Variant ID + Material index (e.g. "GFA00") |
| 2 | Filament type short name |
| 4 | Detailed type string (e.g. "PLA Basic") |
| 5 | RGBA color (bytes 0-3) + spool weight LE (bytes 4-5) |
| 6 | Nozzle temps (bytes 8-11 LE) |

### SpoolEase (NTAG)

SpoolEase uses **NTAG** tags with NDEF URI records. The URL contains encoded spool data:

`https://tag.spoolease.io/S1/?TG=...&M=PLA&CC=000000FF&SC=GFL99&WL=1000&WE=179&WF=1215&NN=190&NX=240`

URL parameters parsed and mapped to SpoolInfo:

| Param | Field | Description |
|-------|-------|-------------|
| `M=` | display type | e.g. "PLA", "PETG" |
| `SC=` | `materialType` | Bambu index for MQTT (e.g. "GFL99") |
| `CC=` | `colorHex` | RGBA hex (e.g. "000000FF") |
| `B=` | `manufacturer` | Brand name (e.g. "Jayo") |
| `WL=` | `remainingGrams` | Remaining filament weight |
| `WE=` | empty spool | Empty spool weight |
| `WF=` | full spool | `totalGrams = WF - WE` |
| `NN=` | `nozzleTempMin` | Min nozzle temp °C |
| `NX=` | `nozzleTempMax` | Max nozzle temp °C |

Reading uses NDEF TLV parsing:
- NDEF Message TLV (0x03) → NDEF record with TNF=WellKnown, type="U"
- URI identifier code byte prepended to URI string
- Non-printable bytes terminate the URI scan
- Spool ID extracted from URL path after last `/`

### TigerTag (NTAG, binary)

TigerTag v2.1 uses **NTAG** chips with a raw 80/144-byte binary format (pages 0x04-0x27):

| Offset | Size | Field |
|--------|------|-------|
| +0 | 4 | ID TigerTag magic (0x5BF59264/0xBC0FCB97/0x6C41A2E1) |
| +4 | 4 | Product ID |
| +8 | 2 | Material ID → lookup table (PLA/PETG/ABS/TPU...) |
| +14 | 2 | Brand ID |
| +16 | 4 | Color 1 RGBA |
| +20 | 3 | Measure (u24 BE) |
| +24 | 2 | Nozzle Temp Min |
| +26 | 2 | Nozzle Temp Max |
| +76 | 3 | Measure Available |

Known material IDs mapped from TigerTag database (PLA=38219, PETG=38256, ABS=20562...)

### Authentication & Reading
- **Tag auto-detect**: SAK-based type detection (MIFARE 1K vs NTAG)
- **HKDF-SHA256** derives 16 per-sector Key A/B from 4-byte UID
- Bambu KDF salt/info vectors from reverse-engineered firmware
- Falls back to default key `0xFF...FF` for blank sectors
- Failed auth re-wakes tag via antenna power-cycle
- Dead readers auto-skipped (version register 0x92/0x91/0xB2 check)
- SPI: 1 MHz via `MFRC522_SPI`
- NTAG: page-level reads (page+=4, 4 pages per MIFARE_Read)

### Filament Type Mapping
| Prefix | Type |
|--------|------|
| GFA-GFE, GFL | PLA |
| GFG | PETG |
| GFH, GFI | ABS |
| GFJ | ASA |
| GFK | TPU |

## OTA Updates

- **Button**: "Update Firmware" on Status page (shows "Update to vX.Y.Z" or "up to date")
- **Overlay**: full-screen progress overlay with spinner, status text, progress bar
- **Endpoint**: `POST /api/ota` triggers update, `GET /api/ota-check` checks for newer version
- Downloads latest `.ino.bin` from GitHub Releases, flashes via `Update.h`
- 3 retry attempts with 5s stall detection, fresh HTTP client per attempt
- TFT shows "Checking version..." → "Downloading..." → "Flashing..." with percentage
- Device auto-reboots after successful flash, web UI auto-reloads

## CI / CD

Workflow at `GHActions/release.yml`:
- **On push/PR**: compiles sketch, uploads artifacts
- **On release tag**: creates merged flash binary + OTA binary, attaches to GitHub Release
- Arduino cache for fast rebuilds, pinned esp32:esp32@3.0.7 core

## Configuration Defaults

| Setting | Default |
|---------|---------|
| WiFi SSID | (empty) |
| WiFi Password | (empty) |
| Device Name | BambuTagger-AMS |
| Printer IP | 192.168.1.100 |
| Printer Port | 8883 |
| Access Code | (empty) |
| Printer Serial | (empty) |
| AMS Unit | A (0) |
| MQTT Enabled | Yes |
| MQTT TLS | No |
| MQTT Update Interval | 3000 ms |
| RFID Poll Interval | 100 ms |
| Firmware Version | 1.0.8 |

