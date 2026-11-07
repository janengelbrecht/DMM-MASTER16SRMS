# Firmware — DMM-MASTER 16SRMS

Denne mappe indeholder den monolitiske firmware til DMM-MASTER 16SRMS.

## Filer

| Fil | Beskrivelse |
| --- | --- |
| `Multimeter_Firmware.ino` | Den komplette firmware (én fil, 100 % inline-kommenteret) |
| `User_Setup.h` | TFT_eSPI-konfiguration til 5" 800×480 RA8875 (SPI) |

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

> **Bemærk om display-controller:** Den medfølgende `User_Setup.h` er konfigureret
> til **RA8875** (SPI, 800×480). TFT_eSPI har ingen SSD1963 SPI-driver — SSD1963
> understøttes kun i 8-bit/16-bit parallel-tilstand, hvilket ikke er farbart på
> ESP32'en, da relæer + I²C + interrupt optager de nødvendige GPIOs. Det fysiske
> display-modul SKAL derfor være et RA8875-baseret 5" 800×480 SPI-modul (f.eks.
> Buydisplay ER-TFTM050-3 med RA8875), IKKE et SSD1963-modul.


Alternativt kan du bruge `User_Setup_Select.h` og inkludere vores fil derfra.

### Pin-konflikt-advarsel

Vores `User_Setup.h` bruger følgende GPIOs til TFT-data og -kontrol:

- D0–D7: GPIO 12, 13, 26, 25, 17, 16, 27, 14
- CS/DC/RST/WR/RD: GPIO 33, 15, 32, 4, 2

**Bemærk** at GPIO 12, 13, 14, 27 **også** bruges til relæstyring i firmwaren!
På et færdigt PCB skal TFT-pinnene vælges fra de ledige GPIOs. Se
`docs/HW-SW-SPEC-V6.0-MASTER.md` afsnit 1.4 for den endelige pin-mapping.

For en prototype (kun firmware-test) kan du midlertidigt flytte relæ-pinnene
eller TFT-pinnene efter behov.

## Upload

1. Åbn `Multimeter_Firmware.ino` i Arduino IDE
2. Vælg korrekt board og partition scheme
3. Tryk Upload

## Første opstart

Efter upload starter firmwaren i **DCV-mode, 1000 V range** (safe state).
Skærmen viser en stor grøn måleværdi, når ADC'en er initialiseret.

Hvis `LittleFS.begin()` fejler, formateres partitionen automatisk.

## Test via SCPI

```bash
# Linux/macOS
screen /dev/ttyACM0 115200

# Windows
# Brug PuTTY med COM-port og 115200 baud

Skriv derefter:

*IDN?
READ?
SYST:REM

Kendte begrænsninger
Firmwaren er ikke verificeret på fysisk hardware endnu

Kalibreringskonstanter er som standard gain=1.0, offset=0.0 (læses fra NVS)

Trend-graf-mode aktiveres endnu ikke via fysisk knap (planlagt)

# Schematics — DMM-MASTER 16SRMS

> ❌ **STATUS: MANGLER**

Denne mappe skal indeholde de komplette KiCad-skemaer for alle 10 blokke:

| Blok | Navn | Indhold |
| --- | --- | --- |
| 1 | Digital strømforsyning | 9V AC → LM7805 → AMS1117-3.3 |
| 2 | Isoleret analog strømforsyning | 9V-0-9V AC → LM7805 / LM7905 |
| 3 | Transientbeskyttelse | GDT + MOV + PTC + 1N4148 |
| 4 | Auto-ranging & FET-buffer | 10×1 MΩ daisy-chain, Omron G6K, ADA4530 |
| 5 | Modstandsmåling & klamper | LT1006, LM336-2.5, LM393 |
| 6 | True RMS | AD8436 + C_AVG |
| 7 | MCU & display | ESP32-WROOM-32, 5" TFT |
| 8 | ADC | ADS1115 |
| 9 | Digital isolation | ISO1541, PC817/6N137 |
| 10 | Strømshunts | 0,01 Ω Manganin, 100 Ω, 1 Ω |

## Format

- **KiCad 7+** (.kicad_sch-filer)
- Alle net skal have **labels** (f.eks. `I2C_SDA_ISO`, `RELAY_DIV10_CTRL`)
- Alle passive komponenter skal have **eksakte værdier + dielektrisk type**
- C_AVG skal angives som **3,3 µF Tantal/Film**

## Leverancekrav

- [ ] Top-level skema (`dmm-master.kicad_sch`)
- [ ] Separate sub-sheets for hver blok
- [ ] Netliste-eksport (`dmm-master.net`)
- [ ] PDF-eksport af alle sider
- [ ] Verifikation: 8 mm krybeafstand mellem Zone A og Zone B

# PCB — DMM-MASTER 16SRMS

> ❌ **STATUS: MANGLER**

Denne mappe skal indeholde PCB-layout og produktionsfiler.

## Leverancekrav

### Kildefiler

- [ ] KiCad PCB-projekt (`.kicad_pcb`, `.kicad_pro`)
- [ ] 3D-visning (`.step` eller `.wrl`)

### Produktionsfiler

- [ ] Gerber X2 (alle lag)
- [ ] NC Drill (boringer)
- [ ] CPL / Pick-and-Place
- [ ] PCB Stackup-dokumentation

## Designkrav

- **Lag:** 4-lags (anbefalet) — 1 oz indre, 2 oz ydre kobber
- **Materiale:** FR4, ≥1,6 mm
- **Isolation:** 8 mm krybeafstand mellem Zone A og Zone B på **alle lag**
- **Milling slots:** Under optokoblere og ISO1541
- **Guard-rings:** Omkring højimpedansbaner (Blok 4), frilagt fra loddemaske
- **Termisk:** Kobber-polygoner og vias under Manganin-shunt og LM7805/LM7905

## Verifikation

- [ ] Krybeafstand ≥8 mm verificeret i Gerber
- [ ] Milling slots verificeret i Gerber
- [ ] Guard-rings frilagt fra loddemaske
- [ ] Termisk analyse (simuleret eller målt)

# Bill of Materials (BOM) — DMM-MASTER 16SRMS

> ❌ **STATUS: MANGLER**

Denne mappe skal indeholde en komplet, produktionsklar BOM.

## Leverancekrav

| Kolonne | Beskrivelse |
| --- | --- |
| Designator | F.eks. R14, C2, U7 |
| Værdi | F.eks. 10 MΩ, 3,3 µF |
| Tolerance | F.eks. 0,1 %, ±25 ppm/°C |
| Rating | Spænding/effekt |
| Footprint | F.eks. SMD 1206, TO-220 |
| Manufacturer | Producent |
| MPN | Producent-partnummer |

## Strenge tolerancekrav

Følgende komponenter **skal** specificeres med høj præcision:

- **Spændingsdeler (Blok 4):** 0,1 % tolerance, ±25 ppm/°C metalfilm
- **Strømshunts (Blok 10):** 0,01 Ω Manganin (10A), 100 Ω og 1 Ω (0,1 %)

## Transformator

Den lineære nettransformer skal specificeres med et **konkret MPN** fra en
godkendt producent (Myrra, Hahn, Block eller tilsvarende) med:

- Primær: 230 V AC
- Sekundær 1: 9 V AC (min. 10 VA)
- Sekundær 2: 9V-0-9V AC center-tapped (min. 10 VA)

## Estimerede omkostninger

| Kategori | Estimeret pris (DKK) |
| --- | --- |
| Elektronik & chips | 65,00 |
| Passive komponenter, relæer, sikring | 45,00 |
| Strømforsyning | 40,00 |
| Skærm (5" TFT) | 75,00 |
| PCB + SMT/THT-montage | 35,00 |
| Kabinet | 30,00 |
| Frontpanel + bøsninger | 20,00 |
| **COGS i alt** | **310,00** |

# Enclosure (kabinet) — DMM-MASTER 16SRMS

> ❌ **STATUS: MANGLER**

Denne mappe skal indeholde 3D-modeller og produktionsfiler for kabinettet.

## Leverancekrav

- [ ] Kildemodel (FreeCAD, Fusion 360 eller OpenSCAD)
- [ ] `.STEP`-eksport (til CAD/CAM)
- [ ] `.STL`-eksport (til 3D-print af prototype)
- [ ] Monteringsvejledning (PDF eller Markdown)

## Designkrav

### Materiale

- **Brandhæmmende plast:** UL94 V-0 rating
- **Vægtykkelse:** Min. 2,5 mm i bærende strukturer

### Intern sikkerhed

- [ ] Fysiske plastbarrierer omkring 230 V-netindgang og HRC-sikringer
- [ ] Adskillelse mellem Zone A og Zone B
- [ ] Ingen mulighed for at plasma/gnister kan nå USB-port eller bruger

### Frontpanel

- [ ] 4 mm sikkerhedsbøsninger: V/Ω, COM, mA/µA, 10A
- [ ] Center-til-center afstand: **19 mm (0,75")** (standard for dual-bananstik)
- [ ] 5" TFT-vindue med bezel

### Montering

- [ ] Standoffs til hoved-PCB
- [ ] Integreret ramme (bezel) til TFT uden mekanisk stress
- [ ] Mulighed for at åbne kabinettet med værktøj (service)

## 3D-print-anbefalinger (prototype)

Til prototype-formål kan kabinettet 3D-printes i:

- **PETG** (anbefalet — bedre kemisk og termisk modstand end PLA)
- **ABS/ASA** (kræver enclosure og god ventilation)
- **PLA** (kun til mekanisk mock-up, ikke til faktisk brug)

**Bemærk:** 3D-printede prototyper opfylder **ikke** UL94 V-0 eller CAT III-kravene
til produktionsbrug. Til produktion skal kabinettet støbes i godkendt plast.

# Datasheets — DMM-MASTER 16SRMS

> ❌ **STATUS: MANGLER**

Denne mappe skal indeholde datablade (PDF) for alle kritiske komponenter.

## Kritiske komponenter

| Komponent | Producent | Formål |
| --- | --- | --- |
| ESP32-WROOM-32 | Espressif | MCU |
| ADS1115 | Texas Instruments | 16-bit ADC |
| AD8436 | Analog Devices | True RMS-konverter |
| ISO1541 | Texas Instruments | I2C-isolator |
| LM393 | Texas Instruments | Komparator (hov-fejl) |
| LT1006 | Analog Devices | Præcisions-OP-amp (Ohm) |
| LM336-2.5 | Texas Instruments | Spændingsreference |
| ADA4530 | Analog Devices | Elektrometer-buffer (valgfri) |
| LSK170 | Linear Systems | JFET-buffer (alternativ) |
| Omron G6K | Omron | Signalrelæ |
| PC817 | Sharp/Toshiba | Optokobler |
| 6N137 | Vishay | High-speed optokobler |

## Hvorfor gemme datablade lokalt?

Datablade kan forsvinde fra producenternes hjemmesider eller ændre URL.
Ved at gemme dem i repo'et sikres reproducerbarhed for fremtidige byg.

**Bemærk:** Datablade er ophavsretligt beskyttede. Kun datablade der
**eksplicit tillader** fri distribution, må inkluderes. Ellers henvises til
producentens URL i denne README.

# Python SCPI-værktøj — DMM-MASTER 16SRMS

Minimal Python-klient til at styre og streame data fra DMM-MASTER.

## Installation

```bash
pip install -r requirements.txt

# Læs identifikation
python dmm_master.py --port /dev/ttyACM0 --idn

# Tag en enkelt måling
python dmm_master.py --port /dev/ttyACM0 --measure

# Stream 1000 målinger og log til CSV
python dmm_master.py --port /dev/ttyACM0 --stream 1000 --log data.csv

# Send vilkårlig SCPI-kommando
python dmm_master.py --port /dev/ttyACM0 --command "SYST:REM"

##Hvad er SCPI?
SCPI (Standard Commands for Programmable Instruments) er en tekstbaseret
protokol, der bruges af professionelle måleinstrumenter. DMM-MASTER emulerer
en USB-CDC Virtual COM Port og svarer på IEEE 488.2 / SCPI-1999-kommandoer.

##Eksempel på Python-script

import serial

with serial.Serial("/dev/ttyACM0", 115200, timeout=1) as dmm:
    dmm.write(b"*IDN?\n")
    print(dmm.readline().decode())          # DMM-MASTER,16SRMS,SN1,V6.0

    dmm.write(b"CONF:VOLT:DC\n")
    dmm.write(b"READ?\n")
    value = float(dmm.readline().decode())
    print(f"Spænding: {value:.5f} V")
	
Kendte begrænsninger
Firmwaren er ikke verificeret på fysisk hardware endnu

SCPI-parseren er kun testet via firmware-simulering

Logfil-format kan ændre sig indtil v1.0


---

## `tools/calibration/README.md`

```markdown
# Kalibreringsværktøjer — DMM-MASTER 16SRMS

> ❌ **STATUS: MANGLER**

Denne mappe skal indeholde scripts og procedurer til at kalibrere DMM-MASTER.

## Kalibreringsmodel

Firmwaren bruger en lineær model for hvert måleområde:

V_display = (V_raw × Gain) + Offset


Kalibreringskonstanterne gemmes i NVS og kan opdateres via SCPI:

- **Zero-punkt:** `CAL:ZERO` — kortslut indgangen, gem nuværende raw-værdi som offset
- **Span-punkt:** `CAL:VAL <reference>` — påtryk kendt reference, gem ny gain

## Planlagte scripts

- [ ] `calibrate_dcv.py` — to-punkts kalibrering af alle DCV-områder
- [ ] `calibrate_acv.py` — ACV-kalibrering mod AC-reference
- [ ] `calibrate_resistance.py` — modstandskalibrering mod præcisionsmodstande
- [ ] `calibrate_current.py` — strømkalibrering mod præcisionsshunt
- [ ] `verify_all.py` — fuld verifikation mod referencemultimeter

## Udstyr til kalibrering

- Referencemultimeter (min. 6½ cifre, f.eks. Fluke 8846A eller Keysight 34461A)
- Præcisions-spændingsreference (f.eks. 5,0000 V)
- Præcisionsmodstande (f.eks. 100 Ω, 10 kΩ, 1 MΩ — 0,01 %)
- Præcisionsshunt til strømkalibrering

## Status

Kalibreringsscripts er ikke udviklet endnu, fordi den fysiske prototype
ikke er færdig. Se `hardware/README.md`.

# Arkitektur — DMM-MASTER 16SRMS

Dette dokument giver et overblik over systemets arkitektur på både hardware-
og software-niveau.

## Overordnet systemarkitektur

┌─────────────────────────────────────────────────────────────────┐
│ ZONE A (Digital) │
│ │
│ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│ │ ESP32 │──│ 5" TFT │ │ USB-CDC │ │
│ │ WROOM-32 │ │ 800x480 │ │ (SCPI) │──► PC │
│ └──────┬──────┘ └─────────────┘ └─────────────┘ │
│ │ │
│ ┌──────┴───────┐ │
│ │ ISO1541 │ (I2C-isolator) │
│ │ PC817/6N137 │ (optokoblere) │
│ └──────┬───────┘ │
└─────────┼───────────────────────────────────────────────────────┘
│ ═══ 8 mm isolation, >4000 V RMS ═══
┌─────────┼───────────────────────────────────────────────────────┐
│ │ ZONE B (Analog) │
│ ┌──────┴───────┐ │
│ │ ADS1115 │ │
│ │ 16-bit ADC │ │
│ └──────┬───────┘ │
│ │ │
│ ┌──────┴───────────────────────────────────────────┐ │
│ │ │ │
│ │ AIN0: DCV (via JFET-buffer) │ │
│ │ AIN1: ACV/ACI (via AD8436) │ │
│ │ AIN2/AIN3: DCI/Ohm (differentiel) │ │
│ │ │ │
│ └──────┬────────────────────────────────────────────┘ │
│ │ │
│ ┌──────┴──────┐ ┌──────────────┐ ┌──────────────┐ │
│ │ Auto-range │──│ AD8436 True │ │ Ohm-kilde │ │
│ │ (Omron G6K) │ │ RMS │ │ LT1006+LM336 │ │
│ └──────┬──────┘ └──────────────┘ └──────┬───────┘ │
│ │ │ │
│ ┌──────┴──────────────┐ ┌────────────────┴───────┐ │
│ │ Transient-beskyttelse│ │ 10A Manganin-shunt │ │
│ │ GDT + MOV + PTC │ │ + 10A HRC-sikring │ │
│ └─────────────────────┘ └───────────────────────┘ │
│ │
└─────────────────────────────────────────────────────────────────┘

## Software-arkitektur

### Core 1 — `Task_Metrology`

Høj prioritet. Uendelig løkke med `vTaskDelay(10)` → 100 Hz.

for (;;) {

Læs ADC (vælg kanal ud fra currentMode)

Konverter til volt

Buffer-flush hvis flag sat

Skriv til ringbuffer (100 samples)

Beregn moving average (50 Hz NMRR)

Auto-ranging hysterese (rate-limited til 200 ms)

Vælg kalibreringsslot

Anvend gain/offset

Send MeasurementPacket til dataQueue

vTaskDelay(10)
}

### Core 0 — `Task_GUI_SCPI`

Lav prioritet. Poller dataQueue og Serial.

for (;;) {

Tjek systemHalted → låst fejltilstand

xQueueReceive(dataQueue) med 50 ms timeout
a. Opdater TFT (anti-flicker)
b. Opdater statuslinje
c. Opdater dual-display (AC ripple)
d. Akkumulér i logBuffer
e. Hvis logBuffer fuld → bulk-skriv til LittleFS

Tjek Serial.available() → parse SCPI

vTaskDelay(5)
}

### IRAM-låst ISR — `hardwareFaultISR`

Trigges af LM393 → optokobler → GPIO 25 (RISING).


Sluk alle relæer (safe state)

Sæt systemHalted = true


## Dataflow mellem kerner

[ADC] → [Core 1: DSP+FSM] → dataQueue → [Core 0: GUI+SCPI+Log]
↓
[USB til PC]


`MeasurementPacket` er den eneste datastruktur der deles mellem kernerne
(ud over `systemHalted`-flaget og `relayGearIndex`, som er volatile).

## Tilstandsmaskine (FSM)

Hovedtilstande (`enum class MeasurementMode`):

- `MODE_DCV`, `MODE_ACV`, `MODE_DCI`, `MODE_ACI`, `MODE_OHM_2W`, `MODE_OHM_4W`

Ved skift mellem tilstande:

1. Tving alle relæer i safe state (1000 V range)
2. Vent 100 ms
3. Slå de nye relæer til
4. Vent 50 ms (settle-time)
5. Flush filterBuffer
6. Genoptag måling

## Auto-ranging hysterese

- **Up-range:** Trigges ved `filteredVoltage ≥ 4.000 V` (98 % af ADC full-scale)
- **Down-range:** Trigges ved `filteredVoltage ≤ 0.360 V` (9 % af aktuelt gear)
- **Rate-limit:** Maks. ét range-skift hvert 200 ms

Dette giver et massivt overlap og forhindrer relæ-chatter.



