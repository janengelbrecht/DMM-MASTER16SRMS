# DMM-MASTER 16SRMS — Komplet Hardware-dokumentation

**DOKUMENT-ID:** HW-DOC-V1.0-MASTER  
**KLASSIFIKATION:** INTERN / OPEN-SOURCE  
**PROJEKT:** Isoleret CAT III 4½-cifret bænkmultimeter  
**SYSTEMARKITEKTUR:** ESP32 / ADS1115 / AD8436  
**LIVSCYKLUSFASE:** DESIGN SPECIFICATION (Hardware ikke fysisk realiseret)  
**DATO:** 06. November 2026  
**EJER:** JEP-Electronics R&D

---

## 0. Dokumentets formål og status

Dette dokument specificerer al hardware i DMM-MASTER 16SRMS på komponentniveau. Det er udarbejdet efter industrielle dokumentationsprincipper svarende til dem, der anvendes hos Intel, Tektronix og HP/Agilent/Keysight, hvor krav, grænseflader og verifikationsbetingelser er sporbare og entydige.

> **Kritisk statusmeddelelse:** Hardwaren er ikke færdigudviklet, ikke fysisk realiseret og ikke certificeret. Dette dokument er en specifikation, ikke en beskrivelse af en eksisterende prototype. Alle værdier, komponentvalg og tolerancer er designmål, der skal verificeres i den færdige konstruktion.

> ⚠️ **Læseren advares:** Intet i dette dokument må tolkes som en garanti for, at en fysisk samling vil fungere eller være sikker. CAT III-isolation og højspændingssikkerhed kræver akkrediteret certificering, som ikke er gennemført.

---

## 1. Systemoverblik og arkitektur

### 1.1 Blokdiagram (funktionelt)

Instrumentet er opdelt i to galvanisk isolerede domæner adskilt af en fysisk isolationsbarriere:

```text
ZONE A (Kontroldomæne — ikke-flydende)
├── Blok 1: Digital strømforsyning (5 V / 3,3 V)
├── Blok 7: ESP32-WROOM-32 + 5" TFT + USB-CDC + buzzer
└── Blok 9 (primær side): ISO1541, PC817 optokoblere

══════════ ISOLATIONSBARRIERE (≥8 mm krybeafstand, >4000 V RMS) ══════════

ZONE B (Måledomæne — galvanisk flydende)
├── Blok 2: Isoleret analog strømforsyning (+5 V / −5 V, GND_ISO)
├── Blok 3: Transientbeskyttelse (GDT + MOV)
├── Blok 4: Auto-ranging, 10 MΩ divider, FET-buffer
├── Blok 5: Modstandsmåling (2W/4W Kelvin) + "hov-fejl"-klamper
├── Blok 6: AD8436 True RMS-konverter
├── Blok 8: ADS1115 16-bit ADC
├── Blok 10: Strømshunts (µA/mA/10 A)
└── Blok 9 (sekundær side): ISO1541, optokoblere
```

### 1.2 Kritiske systemparametre (designmål)

| Parameter | Værdi | Kilde/Status |
| :--- | :--- | :--- |
| **Opløsning** | 4½ cifre (60.000 counts) | Specificeret, kræver DSP-verifikation |
| **ADC-opløsning** | 16-bit (ADS1115) | Se afsnit 5.8 |
| **Målehastighed** | 100 SPS (rå ADC) | Se afsnit 5.8 |
| **Isolationsspænding** | >4000 V RMS | Designmål, ikke testet |
| **Krybeafstand** | ≥8,0 mm | Layoutkrav, se afsnit 6 |
| **CAT-klassifikation** | CAT III 600 V / 1000 V DC | Designmål, ikke certificeret |
| **Netstøjundertrykkelse** | >60 dB ved 50 Hz | Afhænger af DSP-implementering |

---

## 2. Zone A — Digitalt kontroldomæne

### 2.1 Blok 1: Digital strømforsyning

**Funktion:** Leverer stabil, ikke-flydende forsyning til ESP32, TFT, USB-interface og relæspoler (via optokoblere).

**Komponenter:**

| Designator | Komponent | Værdi/Type | Pakke | Krav | Note |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **T1 (primær)** | Nettransformator | 230 V AC → 9 V AC RMS, min. 10 VA | Støbt PCB | Primær 230 V, sekundær 9 V AC | Fælles enhed med Blok 2's sekundærvikling (se 2.2) |
| **D1–D4** | Ensretterdiode | 1N4007 | DO-41 | 1000 V / 1 A | |
| **C1–C4** | Snubber-kondensator | 10 nF | Keramisk X7R | ≥50 V | Monteres over hver diode |
| **C5** | Elektrolytkondensator | 1000 µF / 25 V | Radial | Low-ESR | Hovedfiltrering |
| **C6** | Keramisk bypass | 100 nF | X7R | ≥25 V | |
| **U1** | Lineær regulator | LM7805 | TO-220 | 5 V / 1,5 A | Kræver køleplade |
| **U1-køleplade**| Køleplade | TO-220, Rth ≤10 °C/W | — | | Dimensioneres efter samlet 5 V-last |
| **U2** | LDO-regulator | AMS1117-3.3 | SOT-223 | 3,3 V / 800 mA | |
| **C7** | Tantal | 10 µF / 10 V | — | | Lokal afkobling ved U2 |
| **C8** | Keramisk | 100 nF | X7R | | Lokal afkobling ved U2 |

**Lastfordeling (estimat):**

| Last | Spænding | Strøm (typ.) | Kilde |
| :--- | :--- | :--- | :--- |
| **ESP32-WROOM-32** | 3,3 V | ~150 mA (peak) | Datablad |
| **5" TFT (baggrundsbelysning)** | 3,3 V | ~200 mA | Afhænger af modul |
| **ISO1541 (primær)** | 3,3 V | ~5 mA | |
| **PC817 (×5, primær)** | 3,3 V | ~5 mA pr. | Afhænger af modstand |
| **Relæspoler (via optokobler, 5 V_ISO)** | 5 V | ~20 mA pr. aktiv relæ | |

> **Bemærk:** AMS1117-3.3 har en maksimal udgangsstrøm på 800 mA. Den samlede 3,3 V-last skal holdes under denne grænse med margin. Hvis TFT-modulet kræver mere, skal der vælges en kraftigere regulator eller ekstern buck-converter. Dette er et åbent designpunkt, der skal verificeres, når det konkrete TFT-modul er valgt.

### 2.2 Blok 2: Isoleret analog strømforsyning (Zone B)

**Funktion:** Genererer en galvanisk flydende, balanceret ±5 V-forsyning udelukkende til analog-frontenden.

**Komponenter:**

| Designator | Komponent | Værdi/Type | Pakke | Krav | Note |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **T1 (sekundær 2)** | Center-tappet vikling | 9 V–0–9 V AC | — | Separat vikling fra Blok 1 | Ingen metallisk forbindelse til Blok 1's vikling |
| **D5–D8** | Ensretterdiode | 1N4007 | DO-41 | 1000 V / 1 A | Dobbelt broudretter |
| **C9, C10** | Elektrolytkondensator| 1000 µF / 25 V | Radial | Low-ESR | ±rå DC-filtrering |
| **U3** | Positiv regulator | LM7805 | TO-220 | +5,0 V | VCC_ISO |
| **U4** | Negativ regulator | LM7905 | TO-220 | −5,0 V | VEE_ISO |
| **C11, C12** | Tantal | 10 µF / 16 V | — | | Lokal afkobling ved U3 |
| **C13, C14** | Tantal | 10 µF / 16 V | — | | Lokal afkobling ved U4 |
| **C15–C18** | Keramisk | 100 nF | X7R | | HF-afkobling |

> **Kritisk layoutkrav:** Krybeafstanden mellem Blok 2's transformatorvikling og Blok 1's vikling skal være ≥8,0 mm i layoutet. Dette opnås bedst ved at vælge en transformator med fysisk adskilte viklingskamre (se afsnit 7, BOM).

### 2.3 Blok 7: ESP32-kontrolenhed og brugerflade

**Komponenter:**

| Designator | Komponent | Værdi/Type | Pakke | Krav |
| :--- | :--- | :--- | :--- | :--- |
| **U5** | MCU-modul | ESP32-WROOM-32 | Modul | 3,3 V, dual-core |
| **—** | TFT-display | 5" 800×480 RA8875 SPI TFT | Modul | 3,3 V logik, SPI-grænseflade, RA8875-controller (TFT_eSPI-kompatibel) |
| **BZ1** | Piezo-buzzer | 3,3 V aktiv/passiv | — | Drives via transistor eller direkte GPIO (strøm ≤12 mA) |
| **SW1** | Reset-knap | Momentary | — | Fysisk reset af ESP32 |
| **R1, R2** | I²C pull-up (primær side) | 10 kΩ | 0603 | Til ISO1541 side 1 |

**Pin-mapping (absolut krav — firmware-kompatibilitet):**

| Signal | ESP32 GPIO | Retning | Note |
| :--- | :--- | :--- | :--- |
| **PIN_I2C_SDA** | GPIO 21 | Bidirektionel | Via ISO1541 side 1 |
| **PIN_I2C_SCL** | GPIO 22 | Output | Via ISO1541 side 1 (unidirectional clock) |
| **PIN_RELAY_DIV10** | GPIO 13 | Output | Via PC817 → relæ |
| **PIN_RELAY_DIV100**| GPIO 12 | Output | Via PC817 → relæ |
| **PIN_RELAY_AC_ROUTE**| GPIO 14 | Output | Via PC817 → relæ |
| **PIN_RELAY_OHM** | GPIO 27 | Output | Via PC817 → relæ |
| **PIN_FAULT_INT** | GPIO 25 | Input (ISR) | Fra LM393 via high-speed optokobler |
| **SPI (TFT)** | GPIO 4 (MOSI), GPIO 5 (SCLK), GPIO 33 (CS), GPIO 32 (DC), GPIO 23 (RST) | Output | Fastlagt i User_Setup.h — RA8875 SPI. MISO ikke brugt (write-only). |

> **Kritisk note om ISO1541:** Chippen har bidirektionel data (SDA) men unidirektionel clock (SCL). Dette betyder, at målesiden (Zone B) ikke kan udføre clock stretching. ADS1115 kræver ikke clock stretching, så konfigurationen er forenelig. Dette skal dog verificeres i firmware under I²C-transaktioner.

---

## 3. Isolationsbarriere og digital signaloverførsel

### 3.1 Blok 9: Digital isolering

**Komponenter:**

| Designator | Komponent | Værdi/Type | Pakke | Krav | Note |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **U6** | I²C-isolator | ISO1541 | SOIC-8 | 3–5,5 V begge sider | 2500 V RMS UL, 4242 V peak VDE |
| **U7** | High-speed optokobler| 6N137 | DIP-8/SOIC-8 | ≥1 Mbps | Til PIN_FAULT_INT |
| **U8–U11** | Optokobler (relæ) | PC817 | DIP-4/SMD | CTR ≥50 % | Til relæstyring |
| **R3** | LED-seriemodstand (6N137) | 220 Ω | 0603 | | Dimensioneres efter VCC_ISO |
| **R4–R7** | LED-seriemodstand (PC817) | 1 kΩ | 0603 | | Dimensioneres efter ESP32 GPIO-strøm (max 12 mA anbefalet) |

**Isolationscertificeringer (ISO1541):**

| Certificering | Værdi | Standard |
| :--- | :--- | :--- |
| **UL 1577** | 2500 V RMS, 1 min | UL |
| **VDE 0884-17** | 4242 V peak | DIN EN IEC 60747-17 |
| **CSA** | IEC 62368-1 | CSA |
| **CQC** | GB4943.1-2011 | Kina |

> **Vigtigt:** ISO1541's UL-værdi er 2500 V RMS, ikke 4000 V RMS. Systemets samlede isolationsspænding på >4000 V RMS opnås gennem den fysiske PCB-barriere (8 mm krybeafstand) og valget af isolationskomponenter med tilstrækkelig rating. ISO1541 alene specificerer ikke 4000 V RMS. Hvis systemet skal certificeres til 4000 V RMS, skal der enten vælges en isolator med højere rating, eller den fysiske afstand skal dokumenteres som den primære isolationsbarriere, mens ISO1541's rating kun dækker den funktionelle isolation. Dette er et kritisk designpunkt, der kræver afklaring før eventuel certificering.

---

## 4. Zone B — Analogt måledomæne

### 4.1 Blok 3: Transientbeskyttelse

**Funktion:** Første forsvarslinje mod højspændingstransienter (designmål: op til 6 kV).

**Komponenter:**

| Designator | Komponent | Værdi/Type | Pakke | Krav | Note |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **RV1** | Metaloxid-varistor (MOV) | S10K300 | Radial, 10 mm | 300 V AC RMS max kontinuerlig | TDK B72210S0301K101 |
| **GDT1**| Gasafleder (GDT) | 600 V DC sparkover| Radial | Serie med RV1 | F.eks. Littelfuse GTCX36-serien (600 V model) |

**Funktionel beskrivelse:** Ved normal måling (0–600 V) er GDT'en ikke-ledende og MOV'en har høj modstand. Ved transient >560 V tænder GDT'en (ionisering af ædelgas) inden for <1 µs, og MOV'en klamper spændingen. Seriekombinationen forhindrer MOV'en i at have lækstrøm under normale forhold og beskytter mod termisk runaway.

> **Bemærk:** De konkrete GDT- og MOV-værdier skal verificeres i forhold til den endelige CAT III-rating og de faktiske transienttest, der kræves for certificering. S10K300's maksimale kontinuerlige AC-spænding er 300 V RMS, hvilket er tilstrækkeligt til 600 V DC-måling, men ikke til kontinuerlig 600 V AC RMS. Ved AC-måling op til 600 V RMS skal der vælges en MOV med højere rating (f.eks. S10K420) eller verificere, at spændingsfaldet over GDT'en reducerer MOV-belastningen tilstrækkeligt.

### 4.2 Blok 4: Auto-ranging, spændingsdeler og FET-buffer

**Komponenter:**

| Designator | Komponent | Værdi/Type | Pakke | Tolerence | Krav |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **R8–R17** | Spændingsdelermodstande | 1 MΩ | SMD 1206 | 0,1 %, ±25 ppm/°C | 10 stk. i serie = 10 MΩ total |
| **K1** | Relæ (DIV10) | Omron G6K-2P-Y | PCB | 5 V coil | Ultra-lav-lækage |
| **K2** | Relæ (DIV100) | Omron G6K-2P-Y | PCB | 5 V coil | |
| **K3** | Relæ (AC_ROUTE) | Omron G6K-2P-Y | PCB | 5 V coil | |
| **Q1** | JFET-buffer | LSK170 (grade A/B/C/D efter behov) | SOT-23/TO-92| — | >1 GΩ indgangsimpedans |
| **—** | Alternativ buffer-IC | ADA4530-1 | SOIC-8 | — | Elektrometer-forstærker (kræver separat footprint) |

**Designkrav til FET-buffer:**

*   LSK170's typiske indgangsimpedans er 20 GΩ, hvilket opfylder >1 GΩ-kravet med stor margin.
*   JFET'en skal bias'es korrekt for at undgå gate-lækstrøm. Gate-lækstrøm specificeres til max −1000 pA ved V_GS = −10 V. Dette er tilstrækkeligt lavt til 600 mV-området, men skal verificeres ved den faktiske arbejdstemperatur.
*   Der skal monteres en guard-ring på PCB omkring JFET-gate og de højimpedansbaner, der forbinder til spændingsdeleren. Guard-ringen skal forbindes til det lokale signalpotentiale (ikke GND_ISO) for at minimere lækstrømme.

**Område-konfiguration (via relæer):**

| Område | DIV10 | DIV100 | Signaldæmpning | Indgangsimpedans |
| :--- | :--- | :--- | :--- | :--- |
| **600 mV** | OFF | OFF | 1:1 (via JFET-buffer) | >1 GΩ |
| **6 V** | OFF | OFF | 1:1 (via JFET-buffer) | >1 GΩ |
| **60 V** | ON | OFF | 1:10 | 10 MΩ |
| **600 V** | ON | ON | 1:100 | 10 MΩ |
| **1000 V DC** | ON | ON | 1:100 | 10 MΩ |

> **Kritisk:** 1000 V DC-området bruger samme dæmpning som 600 V-området. Forskellen ligger i software-kalibrering og ADC-skalering. Ved 1000 V ind er spændingen over hver 1 MΩ-modstand 100 V, hvilket er sikkert for 1206-pakken (typisk max 200 V). Ved 600 V AC RMS (850 V peak) er spændingen over hver modstand 85 V peak, også sikkert.

### 4.3 Blok 5: Modstandsmåling og "hov-fejl"-beskyttelse

**Komponenter:**

| Designator | Komponent | Værdi/Type | Pakke | Krav | Note |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **U12** | Præcisions-OP-amp | LT1006 | SOIC-8/PDIP-8 | Offset ≤50 µV max | Strømkilde |
| **U13** | Spændingsreference | LM336-2.5 | TO-92/SOIC-8 | 2,5 V, ±1 % | |
| **PTC1** | PTC-termistor | 100 mA holdestrøm | Radial | 600 V CAT III rated | |
| **D9–D12** | Klamperdiode | 1N4148 | DO-35 | 100 V / 300 mA | Klamper til VCC_ISO/GND_ISO |
| **U14** | Komparator | LM393 | SOIC-8/DIP-8 | 2–36 V forsyning | |
| **R18–R21** | Modstandsnetværk | Efter dimensionering | 0603 | 0,1 % (hvor relevant) | Til LM393-reference og teststrøm |
| **K4** | Relæ (OHM) | Omron G6K-2P-Y | PCB | 5 V coil | |

**"Hov-fejl"-kæde (verifikationsrækkefølge):**

1.  230 V påtrykkes V/Ω i OHM-mode.
2.  PTC1's modstand stiger som funktion af temperatur og begrænser strømmen.
3.  D9–D12 klamper spændingen ved PTC-udgangen til ca. +5,7 V / −0,7 V.
4.  LM393 sammenligner spændingen med en reference (ca. 4,5 V). Når referencen overskrides, går output HIGH.
5.  LM393's output driver 6N137's LED via en modstand (R22, værdi afhænger af VCC_ISO og LED-strøm).
6.  6N137's fototransistor trækker PIN_FAULT_INT (GPIO 25) LOW.
7.  ESP32's IRAM_ATTR-ISR (hardwareFaultISR) eksekveres og tvinger alle relæ-pins LOW (safe state).

> **Kritisk verifikationspunkt:** Reaktionstiden fra 230 V påtrykkes til relæerne er i safe state er ikke specificeret i eksisterende dokumentation. LM393's responstid er typisk <1 µs, men 6N137's propagation delay og ESP32's interrupt-latency skal medregnes. Den samlede tid skal måles på den fysiske prototype. Indtil da må "hov-fejl"-beskyttelsen ikke beskrives som "øjeblikkelig" eller "under 1 µs" i ekstern kommunikation.

### 4.4 Blok 6: True RMS-konverter

**Komponenter:**

| Designator | Komponent | Værdi/Type | Pakke | Krav | Note |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **U15** | True RMS IC | AD8436 | LFCSP-20 eller QSOP-20 | ±5 V forsyning | |
| **C19** | Averaging-kondensator (C_AVG)| 3,3 µF | Tantal eller film | Lav lækstrøm | Kritisk komponent — lækstrøm påvirker nøjagtighed |
| **R22, R23**| Gain-modstande | Efter AD8436-datablad| 0603 | 0,1 % | Dimensioneres efter ønsket gain |

**AD8436-nøgledata:**

| Parameter | Værdi | Kilde |
| :--- | :--- | :--- |
| **Nøjagtighed (B-grade)** | ±10 µV ±0,25 % af læsning | |
| **Indgangsområde** | 100 µV RMS til 3 V RMS (8,5 V p-p) | |
| **Båndbredde (−3 dB)** | 1 MHz ved 300 mV | |
| **Crest factor** | Op til 10 (fejl <0,5 % for CF 1–10) | |
| **Forsyning** | ±2,4 V til ±18 V | |
| **Indgangsimpedans (FET-buffer)** | ≥10¹² Ω, ≤2 pF | |

> **Begrænsning:** AD8436's specificerede nøjagtighed på ±0,25 % er bedre end systemets samlede ACV-nøjagtighedskrav (ikke specificeret i dette dokument, men begrænset af front-end og ADC). AD8436 er således ikke den begrænsende faktor for AC-nøjagtighed i dette design.

### 4.5 Blok 8: ADS1115 16-bit ADC

**Komponenter:**

| Designator | Komponent | Værdi/Type | Pakke | Krav |
| :--- | :--- | :--- | :--- | :--- |
| **U16** | ADC | ADS1115 | VSSOP-10 / X2QFN-10 | 16-bit, I²C |
| **C20** | Afkobling | 100 nF | 0603 | Tæt ved VDD |
| **C21** | Afkobling | 1 µF | 0603 | |
| **R24, R25**| I²C pull-up (sekundær side) | 10 kΩ | 0603 | Til VCC_ISO |

**Konfiguration (fastlagt i hardware/firmware):**

| Parameter | Værdi |
| :--- | :--- |
| **PGA-gain** | GAIN_ONE (±4,096 V FS) |
| **Data rate** | 100 SPS (konfigureres i firmware) |
| **MUX-routing** | AIN0 = DC, AIN1 = AC RMS, AIN2/AIN3 = differentielt (Ohm/shunt) |
| **I²C-adresse**| 0x48 (ADDR til GND) |

> **Bemærk:** ADS1115 har ingen 100 SPS-indstilling (kun 8/16/32/64/128/250/475/860 SPS). Designet bruger **475 SPS** i firmwaren, hvilket sammen med metrologi-loopets 10 ms delay giver en reel sample-rate på ~83 Hz. 100-sample-filteret dækker dermed ~1,2 s (ikke præcis 1,0 s), hvilket stadig giver tilstrækkelig 50 Hz NMRR (>60 dB) men afviger fra den oprindelige "præcis 2 perioder"-påstand. Dette er en firmware-konfiguration, ikke en hardware-begrænsning.
### 4.6 Blok 10: Strømshunts

**Komponenter:**

| Designator | Komponent | Værdi/Type | Tolerence | Effekt | Note |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **R26** | Shunt (µA) | 100 Ω | 0,1 % | ≥0,25 W | Præcisionsmodstand |
| **R27** | Shunt (mA) | 1 Ω | 0,1 % | ≥0,5 W | Præcisionsmodstand |
| **R28** | Shunt (10 A) | 0,01 Ω | 1 % | 5 W | Manganin (lav TCR) |
| **F1** | HRC-sikring | 11 A / 1000 V | — | — | Bussmann DMM-B-11A |
| **PTC2** | PTC (µA/mA-beskyttelse) | Efter dimensionering| — | — | |
| **Q2–Q5** | Klampertransistorer | Efter dimensionering| — | — | Til elektronisk klamper |

> **Kritisk om Manganin:** Manganin er valgt på grund af sin lave temperaturkoefficient (<50 ppm/°C for typiske shuntmaterialer). Ved 10 A gennem 0,01 Ω dissiperes 1 W, hvilket er inden for 5 W-ratingen. Den faktiske shunt skal vælges som en komponent med specificeret TCR og effektrating. En generisk "0,01 Ω 5 W"-modstand uden specificeret TCR er ikke acceptabel.

---

## 5. Komplet Bill of Materials (BOM)

### 5.1 Aktive komponenter

| # | Designator | Komponent | MPN (forslag) | Pakke | Antal | Kritisk parameter | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **1** | U5 | ESP32-WROOM-32 | ESP32-WROOM-32 | Modul | 1 | Dual-core, 3,3 V | Valgt |
| **2** | U16 | ADS1115 | ADS1115IDGSR | VSSOP-10 | 1 | 16-bit, I²C | Valgt |
| **3** | U15 | AD8436 | AD8436ARQZ | QSOP-20 | 1 | True RMS | Valgt |
| **4** | U6 | ISO1541 | ISO1541D | SOIC-8 | 1 | I²C-isolator | Valgt |
| **5** | U14 | LM393 | LM393DR | SOIC-8 | 1 | Komparator | Valgt |
| **6** | U12 | LT1006 | LT1006S8#PBF | SOIC-8 | 1 | Præcisions-OP | Valgt |
| **7** | U13 | LM336-2.5 | LM336Z-2.5 | TO-92 | 1 | 2,5 V reference | Valgt |
| **8** | U1 | LM7805 | LM7805CT | TO-220 | 1 | 5 V / 1,5 A | Valgt |
| **9** | U2 | AMS1117-3.3 | AMS1117-3.3 | SOT-223 | 1 | 3,3 V / 800 mA | Valgt |
| **10** | U3 | LM7805 | LM7805CT | TO-220 | 1 | +5 V_ISO | Valgt |
| **11** | U4 | LM7905 | LM7905CT | TO-220 | 1 | −5 V_ISO | Valgt |
| **12** | Q1 | LSK170 | LSK170A | SOT-23 | 1 | JFET-buffer | Valgt |
| **13** | U7 | 6N137 | 6N137 | DIP-8 | 1 | High-speed opto | Valgt |
| **14** | U8–U11 | PC817 | PC817X | DIP-4 | 4 | Relædriving | Valgt |

### 5.2 Elektromekaniske komponenter

| # | Designator | Komponent | MPN (forslag) | Antal | Note |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **15** | K1–K4 | Signalrelæ | Omron G6K-2P-Y DC5 | 4 | 5 V coil, 2 A carry |
| **16** | T1 | Transformator | F.eks. Myrra 44083 (9 V + 9 V-0-9 V) | 1 | Skal verificeres: kræver primær 230 V, sekundær 1: 9 V AC, sekundær 2: 9 V–0–9 V AC, min. 10 VA |
| **17** | F1 | HRC-sikring | Bussmann DMM-B-11A | 1 | 11 A / 1000 V |
| **18** | F1-holder| Sikringsholder | Passende til 10×38 mm patron | 1 | |
| **19** | RV1 | Varistor | TDK B72210S0301K101 (S10K300) | 1 | |
| **20** | GDT1 | Gasafleder | Littelfuse GTCX36-601M-R10 (600 V) | 1 | |
| **21** | BZ1 | Piezo-buzzer | Passende 3,3 V aktiv | 1 | |
| **22** | SW1 | Reset-knap | Momentary PCB | 1 | |
| **23** | — | 5" TFT-display | F.eks. Buydisplay ER-TFTM050-3 (RA8875) | 1 | 800×480, RA8875 SPI-controller, 3,3 V logik. SSD1963-moduler er IKKE kompatible (ingen SPI-driver i TFT_eSPI). |
| **24** | — | 4 mm sikkerhedsbøsninger| F.eks. Hirschmann KUN 30 | 4 | 19 mm centerafstand |

### 5.3 Passive komponenter (resumé)

| Kategori | Antal (estimat) | Krav |
| :--- | :--- | :--- |
| **Modstande 0,1 % / ±25 ppm (spændingsdeler)** | 10 | 1 MΩ, 1206 |
| **Modstande 0,1 % (shunts)** | 2 | 100 Ω, 1 Ω |
| **Manganin-shunt** | 1 | 0,01 Ω / 5 W, TCR <50 ppm/°C |
| **Modstande (pull-up, LED, bias)** | ~15 | 0603, 1 % |
| **Keramiske kondensatorer (100 nF, 10 nF)** | ~20 | X7R, ≥25 V |
| **Elektrolytkondensatorer (1000 µF)** | 3 | 25 V, Low-ESR |
| **Tantal-kondensatorer (10 µF)** | 4 | 16 V |
| **C_AVG (3,3 µF)** | 1 | Tantal eller film, lav lækstrøm |
| **PTC-termistorer** | 2 | 100 mA hold, 600 V CAT III |

> **BOM-status:** Denne BOM er ikke produktionsklar. Flere kritiske komponenter mangler endelig MPN og verifikation: TFT-display, transformator (kræver mekanisk fit), sikkerhedsbøsninger og alle passives med specifikke tolerancer og spændingsrating. En komplet BOM med alle designators, footprints og leverandørdata skal udarbejdes i CSV/XLSX-format, før produktion kan påbegyndes.

---

## 6. PCB-layoutkrav

### 6.1 Isolationsbarriere

| Krav | Værdi | Verifikation |
| :--- | :--- | :--- |
| **Krybeafstand Zone A ↔ Zone B** | ≥8,0 mm på alle lag | Måling i Gerber |
| **Isolationsslidser** | Fræses under ISO1541, 6N137 og PC817 | Verificeres i fabrikationsfiler |
| **Dielektrisk testspænding** | >4000 V RMS (designmål) | Ikke testet |

### 6.2 Termisk design

| Komponent | Krav |
| :--- | :--- |
| **LM7805 (U1, U3)** | Køleplade med Rth ≤10 °C/W; termiske vias under TO-220-tab |
| **LM7905 (U4)** | Samme som U3 |
| **10 A Manganin-shunt (R28)** | Kobberpolygoner ≥2 cm² på begge sider, termiske vias; placeres væk fra højimpedansbaner for at undgå Seebeck-effekt |

### 6.3 Shielding og guard-rings

| Krav | Implementering |
| :--- | :--- |
| **GND_ISO-plan** | Uafbrudt på indre lag under hele Zone B's analoge område |
| **Guard-ring omkring JFET-buffer og 10 MΩ-divider** | Kobberring forbundet til JFET-gate-potentiale, frilagt fra loddemaske |
| **Afstand mellem digitale og analoge signaler** | ≥5 mm, eller fysisk adskillelse |

### 6.4 Stackup (anbefaling)

| Lag | Funktion | Kobber |
| :--- | :--- | :--- |
| **1 (top)** | Komponenter, signaler | 2 oz |
| **2** | GND_ISO-plan (Zone B) / GND (Zone A) | 1 oz |
| **3** | VCC_ISO / VCC-planer | 1 oz |
| **4 (bund)** | Signaler, køling | 2 oz |

**Materiale:** FR4, ≥1,6 mm, Tg ≥150 °C.

> **Bemærk:** Stackup er en anbefaling, ikke et endeligt krav. Den endelige stackup skal vælges i samarbejde med PCB-fabrikanten og verificeres for impedans og isolationsafstande.

---

## 7. Mekaniske krav

| Krav | Specifikation | Verifikation |
| :--- | :--- | :--- |
| **Kabinetmateriale** | UL94 V-0 brandhæmmende plast | Datablad |
| **Interne barrierer** | Fysiske plastskillevægge omkring 230 V-indgang og HRC-sikring | 3D-model |
| **Bøsningsafstand** | 19 mm (0,75") center-til-center | Mekanisk måling |
| **TFT-montering** | Integreret bezel uden mekanisk stress | 3D-model |
| **Køling** | Ventilationsåbninger eller intern køleplade | Termisk analyse |

---

## 8. Kritiske åbne punkter og risici

Følgende punkter er ikke afklaret i denne dokumentation og skal løses, før hardwaren kan realiseres:

| # | Åbent punkt | Konsekvens | Anbefalet handling |
| :--- | :--- | :--- | :--- |
| **1** | ISO1541's UL-rating er 2500 V RMS, ikke 4000 V RMS | Systemets isolationspåstand kan ikke dokumenteres udelukkende via ISO1541 | Afklar om den fysiske 8 mm-barriere alene kan certificeres til 4000 V RMS, eller vælg isolator med højere rating |
| **2** | "Hov-fejl"-reaktionstid er ikke målt | Sikkerhedspåstande kan ikke verificeres | Mål samlet responstid på prototype |
| **3** | TFT-displayets endelige MPN er ikke fastlagt | Strømforbrug og mekanisk fit ukendt | Vælg konkret RA8875-baseret 800×480 SPI-modul (f.eks. Buydisplay ER-TFTM050-3) og verificer 3,3 V-last |
| **4** | Transformator-MPN er ikke fastlagt | Mekanisk fit og isolationsafstand ukendt | Vælg transformator med specificeret viklingsadskillelse |
| **5** | 1000 V DC-området bruger samme MOV som 600 V AC | MOV kan overbelastes ved 600 V AC RMS | Verificer MOV-rating eller differentier beskyttelsen |
| **6** | LM7805's samlede termiske belastning er ikke analyseret | Overophedning mulig | Beregn samlet effekttab og dimensioner køleplade |
| **7** | Manganin-shuntens faktiske TCR er ikke specificeret | Målenøjagtighed ved 10 A kan variere | Vælg shunt med dokumenteret TCR <50 ppm/°C |
| **8** | Ingen EMC-test er udført | Ukendt om designet overholder emissionskrav | Planlæg præ-compliance test |

---

## 9. Verifikationsmatrix (hvad der skal testes, og hvordan)

| Test | Formål | Metode | Acceptkriterium | Status |
| :--- | :--- | :--- | :--- | :--- |
| **Isolationstest (hipot)** | Verificere >4000 V RMS | Akkrediteret testhus | Ingen gennemslag ved 4000 V RMS, 1 min | Ikke udført |
| **Krybeafstand** | Verificere ≥8,0 mm | Mikroskopmåling på PCB | ≥8,0 mm alle steder | Ikke udført |
| **"Hov-fejl" responstid** | Verificere safe state | Oscilloskop på relæ-pins | Tid fra 230 V påtrykkes til relæer er LOW: skal måles | Ikke udført |
| **DCV-nøjagtighed** | Verificere ±0,025 % + 5 counts | Sammenligning med 6½-cifret reference| Inden for spec. | Ikke udført |
| **ACV True RMS** | Verificere AD8436-kæden | Sammenligning med kendt AC-kilde | Inden for spec. | Ikke udført |
| **4½-cifret stabilitet**| Verificere DSP-oversampling | Langtidsmåling (≥1 time) | Stabil visning af 60.000 counts | Ikke udført |
| **50 Hz NMRR** | Verificere >60 dB | Injicer 50 Hz støj, mål ændring | < specificeret | Ikke udført |
| **Termisk drift** | Verificere 0,1 %-modstande | Måling ved 23 °C ±2 °C og ved 40 °C | Inden for beregnet drift | Ikke udført |
| **RA8875 SPI-kommunikation** | Verificere display-driver | Skriv testmønster, mål SPI-frekvens | Stabil drift ved 20 MHz, ingen pixel-fejl | Ikke udført |

---

## 10. Referencer

*   Texas Instruments, ISO154x Low-Power Bidirectional I²C Isolators Datasheet, Rev. F, April 2022.
*   Analog Devices, AD8436 True RMS-to-DC Converter Datasheet.
*   Texas Instruments, ADS111x Ultra-Small, Low-Power, I²C-Compatible, 860-SPS, 16-Bit ADCs Datasheet.
*   Omron, G6K PCB Signal Relay Datasheet.
*   Linear Systems, LSK170 Series Datasheet, Rev. A22, 2019.
*   Texas Instruments, LM336-2.5 Datasheet, Rev. E, 2003.
*   Analog Devices, LT1006 Precision Single Supply Op Amp Datasheet.
*   Texas Instruments, LM393-MIL Datasheet, 2017.
*   Texas Instruments, LM7805 Datasheet.
*   Texas Instruments, LM7905 Datasheet.
*   TDK, B72210S0301K101 (S10K300) Varistor Datasheet.
*   Littelfuse, GTCX36 Series GDT Datasheet, 2016.
*   Eaton (Bussmann), DMM-B-11A Fuse Datasheet.
*   Micro Ohm, BHR & HPR Shunt Resistor Datasheet.
*   AMS, AMS1117 Datasheet.