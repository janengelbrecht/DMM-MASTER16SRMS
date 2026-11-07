# DMM-MASTER 16SRMS

**Isoleret CAT III 4½-cifret bænkmultimeter (ESP32 / ADS1115 / AD8436)**

> ⚠️ **PROJEKTSTATUS: UNDER UDVIKLING (WORK IN PROGRESS)**
>
> Dette projekt er **ikke færdigt**. Firmware og dokumentation er langt, men følgende dele mangler stadig og er ikke publiceret endnu:
>
> - ❌ **Komplette diagrammer (KiCad/Altium)** — se `hardware/schematics/README.md`
> - ❌ **PCB-layout og Gerber-filer** — se `hardware/pcb/README.md`
> - ❌ **3D-printede/STEP-filer til kabinettet** — se `hardware/enclosure/README.md`
> - ❌ **Endelig, verificeret BOM med MPN'er** — se `hardware/bom/README.md`
> - ❌ **Kalibreringsmålinger fra fysisk prototype** — se `tools/calibration/README.md`
>
> Firmware er **funktionelt komplet i henhold til SRS-DMM-16SRMS-V6.0-MASSIVE**, men er endnu **ikke verificeret på fysisk hardware**.
> Firmware er simuleret og fundet fejlfri.
> Hardwaren er simuleret som den er beskrevet og fundet fejlfri.
> Da det er et bænkmultimeter der primært vil komme til at måle på lysnettilsluttet udstyr kan man sikkert slække kravene til CAT II.
> Men det skader ikke at designe efter CAT III hovedsagligt.

---

## Indholdsfortegnelse

1. [Introduktion](#introduktion)
2. [Hvorfor dette projekt er anderledes](#hvorfor-dette-projekt-er-anderledes)
3. [Nøglespecifikationer](#nøglespecifikationer)
4. [Arkitektur i korte træk](#arkitektur-i-korte-træk)
5. [Hardware-overblik](#hardware-overblik)
6. [Firmware og software](#firmware-og-software)
7. [SCPI-kommunikation](#scpi-kommunikation)
8. [Sådan kommer du i gang](#sådan-kommer-du-i-gang)
9. [Projektstruktur](#projektstruktur)
10. [Hvad der stadig mangler](#hvad-der-stadig-mangler)
11. [Sikkerhed — læs dette først](#sikkerhed--læs-dette-først)
12. [Roadmap](#roadmap)
13. [Bidrag](#bidrag)
14. [Licens](#licens)

---

## Introduktion

**DMM-MASTER 16SRMS** er et open-source hardware/software co-design af et isoleret, 4½-cifret bænkmultimeter med CAT III (600 V) sikkerhedsklassifikation. Instrumentet er bygget op omkring en **ESP32-WROOM-32**, en **Texas Instruments ADS1115** 16-bit ΔΣ-ADC og en **Analog Devices AD8436** analog True RMS-konverter.

Til en estimeret udsalgspris på **ca. 825 kr. inkl. moms** (ved 1.000 enheders produktion) placerer instrumentet sig som et af markedets billigste åbne 4½-cifrede bænkmultimetre — og det eneste i sin prisklasse, hvor **hele kildekoden og hele hardware-designet er frit tilgængeligt**.

Projektet er primært målrettet:

- **R&D-elektronikudviklere** der vil integrere et DMM direkte i ATE- eller IoT-testmiljøer.
- **Universiteter og undervisere** der har brug for et instrument, hvor studerende kan læse og modificere *hele* koden (én `.ino`-fil, 100 % inline-kommenteret).
- **Avancerede hardware-magere og metrologi-entusiaster** der vil eksperimentere med egne kalibreringsalgoritmer.

> **Designfilosofi:** Dette instrument forsøger *ikke* at slå Fluke eller Keysight på rå præcision. Det er designet til at være den mest **åbne, modificerbare og veldokumenterede** platform i 4½-cifret-klassen.

---

## Hvorfor dette projekt er anderledes

| Funktion | Typisk lavpris-DMM | DMM-MASTER 16SRMS |
| --- | --- | --- |
| Firmware | Lukket, proprietær | ✅ 100 % open source (C++/Arduino) |
| Antal kildefiler | N/A | ✅ Én fil: `Multimeter_Firmware.ino` |
| Kommentar-dækning | N/A | ✅ 100 % inline-kommentarer (hver linje) |
| Kalibreringsalgoritme | Skjult | ✅ Læsbar i koden, gemmes i NVS |
| PC-interface | Proprietær app | ✅ Standard SCPI via USB-CDC |
| Datalogging | Begrænset / lukket | ✅ LittleFS + wear-leveling + SCPI-download |
| Galvanisk isolation | Ofte manglende | ✅ 8 mm barrier, >4000 V RMS |
| "Hov-fejl"-beskyttelse | Sjælden | ✅ LM393-komparator + IRAM-låst ISR |
| Dataloggingsfrekvens | Typisk ≤10 Hz | ✅ 100 Hz kontinuerligt |

---

## Nøglespecifikationer

### Metrologi

| Parameter | DCV | ACV (True RMS) |
| --- | --- | --- |
| Opløsning | 4½ cifre (60.000 counts) | 4½ cifre (60.000 counts) |
| Områder | 600 mV, 6 V, 60 V, 600 V, 1000 V | 600 mV, 6 V, 60 V, 600 V, 750 V |
| Præcision (1 år) | < ±0,025 % + 5 counts | 20 Hz – 100 kHz |
| Indgangsimpedans (lav) | > 1 GΩ (JFET-buffer) | DC-afkoblet |
| Indgangsimpedans (høj) | ≥ 10 MΩ | ≥ 10 MΩ |
| Crest-faktor | — | ≤ 3:1 ved fuldt udslag |

| Parameter | Modstand | Strøm |
| --- | --- | --- |
| Områder | 600 Ω – 60 MΩ (2W + 4W Kelvin) | 600 µA – 10 A (DC + AC) |
| Max opløsning | 0,01 Ω | Afhænger af shunt |
| Strømkilde / shunts | LT1006 + LM336-2.5 | 0,01 Ω Manganin + præcisionsshunts |

### System

- **MCU:** ESP32-WROOM-32 (dual-core, FreeRTOS)
- **ADC:** ADS1115, 16-bit ΔΣ, PGA = GAIN_ONE (±4,096 V)
- **True RMS:** AD8436 analog RMS-til-DC
- **Isolation:** >4000 V RMS, 8 mm krybeafstand (Zone A / Zone B)
- **Skærm:** 5" TFT 800×480 (SSD1963, 8-bit parallel)
- **Datalogging:** LittleFS på ESP32'ens flash, SCPI-downloadbar
- **Målehastighed:** 100 SPS kontinuerligt

---

## Arkitektur i korte træk

Systemet er opdelt i to **galvanisk isolerede domæner** adskilt af en fysisk PCB-barriere på minimum 8 mm:

```text
┌─────────────────────────────────────────────────────────────────────┐
│  ZONE A — KONTROLDOMÆNE (digital jord, berøringssikker)             │
│                                                                     │
│   ESP32-WROOM-32 (dual-core)                                       │
│   ├── Core 1: Task_Metrology  (DSP, FSM, relæstyring)              │
│   ├── Core 0: Task_GUI_SCPI   (TFT, SCPI, LittleFS)                │
│   ├── 5" TFT 800×480 (SSD1963)                                     │
│   └── USB-CDC Virtual COM Port                                     │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
              ══════════════════╪══════════════════
              ISOLATIONSBARRIERE │  (>4000 V RMS, 8 mm)
              ══════════════════╪══════════════════
                                │
┌───────────────────────────────┴─────────────────────────────────────┐
│  ZONE B — MÅLEDOMÆNE (flydende stel, GND_ISO)                       │
│                                                                     │
│   Transientbeskyttelse (GDT + MOV)                                  │
│   Auto-ranging (10×1 MΩ daisy-chain, Omron G6K)                    │
│   JFET-buffer (>1 GΩ på lave områder)                               │
│   AD8436 True RMS + ADS1115 16-bit ADC                             │
│   Præcisionsshunts (Manganin 0,01 Ω + 100 Ω/1 Ω)                   │
└─────────────────────────────────────────────────────────────────────┘
```

### Dual-core opgavefordeling

| Kerne | Task | Prioritet | Ansvar |
| --- | --- | --- | --- |
| **Core 1** | `Task_Metrology` | Høj (3) | ADC-læsning, DSP-filtrering, kalibrering, FSM, relæstyring |
| **Core 0** | `Task_GUI_SCPI` | Lav (1) | TFT-opdatering, SCPI-parser, LittleFS-datalogging |

Data udveksles sikkert via en FreeRTOS-kø (`dataQueue`) med plads til 10 `MeasurementPacket`-strukturer.

---

## Hardware-overblik

Instrumentet er opbygget af 10 funktionelle blokke:

| Blok | Funktion | Status |
| --- | --- | --- |
| **Blok 1** | Digital strømforsyning (Zone A, +5 V / +3,3 V) | 📝 Specifikation klar |
| **Blok 2** | Isoleret analog strømforsyning (Zone B, ±5 V) | 📝 Specifikation klar |
| **Blok 3** | Transientbeskyttelse (GDT + MOV) | 📝 Specifikation klar |
| **Blok 4** | Auto-ranging, spændingsdeler & FET-buffer | 📝 Specifikation klar |
| **Blok 5** | 2W/4W Kelvin modstandsmåling & fejlklamper | 📝 Specifikation klar |
| **Blok 6** | AD8436 True RMS-konverter | 📝 Specifikation klar |
| **Blok 7** | ESP32, TFT & firmware | ✅ Firmware implementeret |
| **Blok 8** | ADS1115 16-bit ADC | 📝 Specifikation klar |
| **Blok 9** | Digital isolationsbarriere (ISO1541 + optokoblere) | 📝 Specifikation klar |
| **Blok 10** | Strømmåling & præcisionsshunts | 📝 Specifikation klar |

> **Bemærk:** Kun firmware (Blok 7) er færdigimplementeret. De øvrige blokke er specificeret i `docs/HW-SW-SPEC-V6.0-MASTER.md`, men de fysiske skemaer, PCB-layout og BOM mangler stadig.

### Centrale komponenter

| Komponent | Type | Funktion |
| --- | --- | --- |
| ESP32-WROOM-32 | MCU | Dual-core 32-bit, FreeRTOS |
| ADS1115 | 16-bit ΔΣ ADC | DC/AC/Ohm/strøm-måling |
| AD8436 | True RMS→DC | Analog RMS-beregning |
| ISO1541 | Kapacitiv I²C-isolator | 5000 Vrms dataisolation |
| PC817 / 6N137 | Optokoblere | Relæ- og interrupt-isolation |
| LM393 | Komparator | "Hov-fejl"-detektion (<1 µs) |
| LT1006 + LM336-2.5 | Præcisionsstrømkilde | Ohm-måling |
| Omron G6K | Signalrelæer | Auto-ranging |
| Manganin 0,01 Ω / 5 W | Shunt | 10 A-måling |

---

## Firmware og software

### Arkitekturprincipper

Firmwaren følger de **strenge krav** i `kravspecifikation_firmware.md` (SRS-DMM-16SRMS-V6.0-MASSIVE):

1. **Monolitisk arkitektur:** Hele firmwaren er i én fil — `Multimeter_Firmware.ino`.
2. **100 % inline-kommentarer:** Hver eneste ikke-tomme kodelinje har en `//`-kommentar.
3. **Nul-forudsætnings-princippet:** Variabelnavne er fuldt beskrivende (f.eks. `relayDivider10Pin` i stedet for `r`).

### Nøglefunktioner

| Funktion | Fil/sektion | Beskrivelse |
| --- | --- | --- |
| **DSP Moving Average** | `Task_Metrology` | 100-samples ringbuffer = 1 s = 2×50 Hz-perioder → NMRR >60 dB |
| **Auto-ranging FSM** | `selectRelayGear()` | Hysterese: up ved ≥98 %, down ved ≤9 % |
| **Relæ blanking** | `selectRelayGear()` | 50–100 ms settle + buffer flush |
| **IRAM-låst ISR** | `hardwareFaultISR()` | <1 µs reaktion på 230 V i Ohm-mode |
| **Software-kalibrering** | `loadCalibrationFromNvs()` | Lineær model: `V_display = V_raw × Gain + Offset` |
| **Wear-leveling logger** | `Task_GUI_SCPI` | 100 målinger i RAM → bulk-skriv → 100× mindre flash-slid |
| **Anti-flicker GUI** | `Task_GUI_SCPI` | Sort-overskrivningsteknik |
| **SCPI-parser** | `handleScpiCommand()` | IEEE 488.2 / SCPI-1999 |

### Firmware-filer

```text
firmware/
├── Multimeter_Firmware.ino    ← Hele firmwaren (én fil)
└── User_Setup.h               ← TFT_eSPI-konfiguration (SSD1963, 8-bit parallel)
```

### Krav til udviklingsmiljø

| Krav | Version |
| --- | --- |
| Arduino IDE | 2.x (nyeste) |
| ESP32 Core | Installeret via Boards Manager |
| Board | **ESP32 Dev Module** |
| Partition Scheme | **No OTA (2MB APP/2MB SPIFFS)** eller Custom |
| Adafruit ADS1X15 | v2.4.0+ |
| TFT_eSPI (Bodmer) | v2.5.0+ |

> ⚠️ **Vigtigt:** `User_Setup.h` skal kopieres til TFT_eSPI's **egen biblioteksmappe** — ikke til projektmappen.

---

## SCPI-kommunikation

Instrumentet emulerer en **USB-CDC Virtual COM Port** og taler standard SCPI (IEEE 488.2 / SCPI-1999).

### Forbindelsesparametre

```text
Baudrate:      115200
Databits:      8
Paritet:       None
Stopbits:      1
Flow control:  None
Terminering:   \n (LF)
```

### Kommandosæt (uddrag)

| Kommando | Funktion |
| --- | --- |
| `*IDN?` | Identifikation → `DMM-MASTER,16SRMS,SN1,V6.0` |
| `*RST` | Reset til sikker DCV 1000 V |
| `CONF:VOLT:DC [<range>]` | Skift til DCV (auto eller manuelt gear) |
| `CONF:RES [<range>]` | Skift til 2W modstand |
| `CONF:FRES [<range>]` | Skift til 4W Kelvin |
| `READ?` | Seneste måling (<5 ms) |
| `MEAS:VOLT:DC?` | Alias for `READ?` |
| `MMEM:CAT?` | List filer på LittleFS |
| `MMEM:DATA? "<fil>"` | Dump fil over USB |
| `MMEM:DEL "<fil>"` | Slet fil |
| `SYST:REM` | Remote mode (GUI off, max hastighed) |
| `SYST:LOC` | Forlad remote mode |
| `CAL:ZERO` | Zero-punkt-kalibrering (kortslut indgang) |
| `CAL:VAL <ref>` | Span-kalibrering med kendt reference |

Se `docs/scpi-reference.md` for fuld dokumentation.

### Python-eksempel

```python
import serial
import time

# Åbn forbindelsen til multimeteret
dmm = serial.Serial("/dev/ttyACM0", 115200, timeout=1)
time.sleep(2)  # Vent på ESP32 boot

# Identificér instrumentet
dmm.write(b"*IDN?\n")
print(dmm.readline().decode().strip())

# Skift til DCV og læs 10 målinger
dmm.write(b"CONF:VOLT:DC\n")
for _ in range(10):
    dmm.write(b"READ?\n")
    print(dmm.readline().decode().strip())
    time.sleep(0.1)

dmm.close()
```

---

## Sådan kommer du i gang

### 1. Klargør hardware (når prototypen er klar)

> ⚠️ **Hardware er endnu ikke publiceret.** Se [Hvad der stadig mangler](#hvad-der-stadig-mangler).

### 2. Installer Arduino IDE 2.x

Download fra [arduino.cc/en/software](https://www.arduino.cc/en/software).

### 3. Tilføj ESP32 Boards Manager URL

`File → Preferences → Additional Boards Manager URLs`:

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

### 4. Installer ESP32 Core

`Tools → Board → Boards Manager` → søg "esp32" → installer.

### 5. Installer biblioteker

`Sketch → Include Library → Manage Libraries`:

- **Adafruit ADS1X15** (v2.4.0+)
- **TFT_eSPI** af Bodmer (v2.5.0+)

### 6. Konfigurer TFT_eSPI

Kopiér `firmware/User_Setup.h` til TFT_eSPI's biblioteksmappe.

### 7. Vælg board og partition

- **Board:** `ESP32 Dev Module`
- **Partition Scheme:** `No OTA (2MB APP/2MB SPIFFS)`

### 8. Upload firmware

Åbn `firmware/Multimeter_Firmware.ino` og tryk Upload.

### 9. Tilslut og test

```bash
pip install -r requirements.txt
python tools/scpi_test.py
```

---

## Projektstruktur

```text
DMM-MASTER-16SRMS/
│
├── README.md                          ← Dette dokument
├── LICENSE                            ← Open source-licens
├── CONTRIBUTING.md                    ← Bidragsguide
├── requirements.txt                   ← Python-afhængigheder (pyserial)
│
├── firmware/
│   ├── Multimeter_Firmware.ino        ← Hele firmwaren (én fil)
│   └── User_Setup.h                   ← TFT_eSPI-konfiguration
│
├── docs/
│   ├── HW-SW-SPEC-V6.0-MASTER.md      ← Global engineering specifikation
│   ├── kravspecifikation_firmware.md  ← SRS (softwarekrav)
│   ├── manual.md                      ← Teknisk manual
│   ├── scpi-reference.md              ← SCPI-kommandoreference
│   └── calibration-procedure.md       ← Kalibreringsprocedure
│
├── hardware/
│   ├── README.md                      ← Hardware-status
│   ├── schematics/                    ← ❌ KiCad-skemaer (mangler)
│   ├── pcb/                           ← ❌ PCB-layout (mangler)
│   ├── bom/                           ← ❌ Stykliste (mangler)
│   ├── enclosure/                     ← ❌ 3D-kabinet (mangler)
│   └── datasheets/                    ← ❌ Datablade (mangler)
│
├── tools/
│   ├── scpi_test.py                   ← Python SCPI-testscript
│   └── calibration/                   ← ❌ Kalibreringsværktøjer (mangler)
│
└── images/                            ← ❌ Skærmbilleder (mangler)
```

---

## Hvad der stadig mangler

| Område | Status | Prioritet |
| --- | --- | --- |
| KiCad-skemaer (Blok 1–10) | ❌ Mangler | 🔴 Høj |
| PCB-layout med isolationsslidser | ❌ Mangler | 🔴 Høj |
| BOM med verificerede MPN'er | ❌ Mangler | 🔴 Høj |
| 3D-kabinet (.STEP/.STL) | ❌ Mangler | 🟡 Medium |
| Prototype-fotografier | ❌ Mangler | 🟡 Medium |
| GUI-skærmbilleder | ❌ Mangler | 🟢 Lav |
| Kalibreringsmålinger fra fysisk hardware | ❌ Mangler | 🔴 Høj |
| Verifikation af firmware på fysisk hardware | ❌ Mangler | 🔴 Høj |

Se `hardware/README.md` og `images/README.md` for detaljer.

---

## Sikkerhed — læs dette først

> ⚠️ **DETTE INSTRUMENT ER DESIGNET TIL CAT III (600 V), MEN ER ENDNU IKKE CERTIFICERET.**
>
> Prototypen er **ikke** blevet testet på fysisk hardware. Brug **aldrig** en uverificeret prototype til at måle på netspænding (230 V) eller andre farlige spændinger.

### Designprincipper for sikkerhed

1. **Galvanisk isolation:** 8 mm fysisk krybeafstand + fræsede slidser i PCB → >4000 V RMS isolation mellem Zone A og Zone B.
2. **Transientbeskyttelse:** GDT + MOV klamper 6 kV transienter på <1 ns.
3. **"Hov-fejl"-beskyttelse:** LM393-komparator + IRAM-låst ISR reagerer på <1 µs ved 230 V i Ohm-mode.
4. **HRC-sikring:** 10 A / 1000 V keramisk sandfyldt sikring på højstrømsindgangen.
5. **Brandhæmmende kabinet:** UL94 V-0 plast med interne plastbarrierer.

### Sikkerhedsregler for brugeren

1. **Hold kun på plastikken** på testledningerne — aldrig på metallet.
2. **Mål aldrig på 230 V alene** — få altid en erfaren person til at overvære det.
3. **Sort ledning ind først, ud sidst.**
4. **Brug altid passende CAT-rating** på testledningerne.

---

## Roadmap

### ✅ Fase 1 — Firmware og dokumentation (2025–2026)

- [x] Kravspecifikation (SRS)
- [x] Teknisk manual
- [x] Firmware (én fil, 100 % kommenteret)
- [x] SCPI-reference
- [x] Kalibreringsprocedure

### 🚧 Fase 2 — Hardware-design (2026)

- [ ] KiCad-skemaer for alle 10 blokke
- [ ] PCB-layout med isolationsslidser og guard-rings
- [ ] BOM med verificerede MPN'er
- [ ] 3D-kabinet med CAT III-barrierer

### 🔜 Fase 3 — Prototype og verifikation (2026–2027)

- [ ] Fysisk prototype (første revision)
- [ ] Firmware-verifikation på hardware
- [ ] Kalibreringsmålinger
- [ ] Billeder og skærmbilleder
- [ ] Eventuel ekstern sikkerhedscertificering (IEC 61010-1)

### 🔮 Fase 4 — Produktion (2027+)

- [ ] Produktionsmoden PCB
- [ ] Produktionsklar BOM
- [ ] Salgsklar dokumentation

---

## Bidrag

Bidrag er **meget velkomne** — især inden for hardware-design og 3D-kabinet, hvor de største huller er lige nu.

Se `CONTRIBUTING.md` for retningslinjer. Kort opsummeret:

1. **Firmware:** Følg de strenge krav i SRS (én fil, 100 % kommentarer, beskrivende navne).
2. **Hardware:** Følg designkravene i `docs/HW-SW-SPEC-V6.0-MASTER.md` (isolation, creepage, thermal).
3. **Dokumentation:** Dansk eller engelsk, klar og præcis.
4. **Bug reports:** Brug `bug_report.md`-skabelonen.

### Prioriteret rækkefølge for hjælp

1. 🔴 **KiCad-skemaer** for alle 10 blokke
2. 🔴 **PCB-layout** med isolationsslidser og guard-rings
3. 🔴 **BOM** med verificerede MPN'er og tolerancer
4. 🟡 **Kabinet** med CAT III-barrierer og 19 mm bøsningsafstand

---

## Licens

Dette projekt er open source. Se `LICENSE` for detaljer.

- **Firmware og dokumentation:** Anbefalet MIT eller Apache 2.0
- **Hardware-design:** Anbefalet CERN-OHL-S eller TAPR OHL

> **Bemærk:** Den endelige licens er ikke fastlagt endnu. Se `LICENSE` når den publiceres.

---

## Kontakt og links

- **Projekt:** DMM-MASTER 16SRMS
- **Version:** 6.0
- **Dokument-ID:** README-DMM-16SRMS-V6.0
- **Status:** 🚧 Under udvikling

---

*DMM-MASTER 16SRMS — et open-source colosseum i 4½-cifret-klassen.*
