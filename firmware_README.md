# Firmware — DMM-MASTER 16SRMS

Denne mappe indeholder den monolitiske firmware til DMM-MASTER 16SRMS.

## Filer

| Fil | Beskrivelse |
| --- | --- |
| `Multimeter_Firmware.ino` | Den komplette firmware (én fil, 100 % inline-kommenteret) |
| `User_Setup.h` | TFT_eSPI-konfiguration til 5" 800×480 SSD1963 (8-bit parallel) |

## Krav

- **Arduino IDE 2.x** (nyeste version)
- **ESP32 Core** installeret via Boards Manager
- **Board:** ESP32 Dev Module
- **Partition Scheme:** No OTA (2MB APP/2MB SPIFFS)
- **Biblioteker:**
  - Adafruit ADS1X15 v2.4.0+
  - TFT_eSPI v2.5.0+ (af Bodman)

## TFT_eSPI-konfiguration

TFT_eSPI kræver en `User_Setup.h` i sin **egen biblioteksmappe**. Den medfølgende
`User_Setup.h` i denne mappe skal **kopieres til**:
