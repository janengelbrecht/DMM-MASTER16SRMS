# GLOBAL ENGINEERING SPECIFICATION CONTROL DOCUMENT (SCD)

**DOCUMENT ID:** HW-SW-SPEC-V6.0-MASTER  
**CLASSIFICATION:** STRICTLY CONFIDENTIAL / PROPRIETARY  
**PROJECT NAME:** ISOLATED CAT III 4.5-DIGIT BENCHTOP MULTIMETER  
**SYSTEM ARCH:** ESP32 (MCU) / ADS1115 (ADC) / AD8436 (True RMS)  

---

**DISCIPLINE:** HW / FW CO-DESIGN  
**LIFECYCLE PHASE:** PRODUCTION RELEASE (RELEASED)  
**AUTHOR(S):** R&D Department  
**EFFECTIVE DATE:** NOVEMBER 05, 2026  
**OWNER / DEPT:** JEP-Electronics  
**CHANGE ORDER:** ECO-2026-1105-B  

---

# 1.0 ELEKTRISKE SPECIFIKATIONER OG SYSTEMYDELSE
Dette afsnit definerer systemets instrumentelle formåen. Den underliggende 16-bit hardware forstærkes via avanceret digital signalbehandling (DSP) for at opnå professionel laboratoriepræcision.

### Opløsning og Hastighed
* **Base Opløsning:** 4½ cifre (60.000 counts) realiseret via 16-bit ADC.
* **Målehastighed:** Op til 100 målinger pr. sekund (SPS) direkte fra ADC'en til hurtig data-logging.

### DC Spænding (DCV)
* **Præcision (1 år):** < ±0,025% af aflæsningen + 5 counts (baseret på ultra-stabil spændingsreference og 0.1% modstandsnetværk).
* **Områder:** 600 mV, 6 V, 60 V, 600 V, 1000 V.
* **Inputimpedans:** ≥ 10 MΩ på alle områder. >1 GΩ på 600 mV og 6 V områderne opnået via dedikeret FET-buffer (f.eks. JFET LSK170 eller ADA4530).
* **Max Opløsning:** 10 µV på 600 mV området.

### AC Spænding (ACV - True RMS)
* **Konvertering:** Rent analog via True RMS IC (AD8436).
* **Områder:** 600 mV, 6 V, 60 V, 600 V, 750 V.
* **Frekvensområde:** 20 Hz til 100 kHz.
* **Crest Faktor:** ≤ 3:1 ved fuldt udslag (op til 5:1 ved halvt udslag).

### Strømmåling (DCI / ACI)
* **Områder:** 600 µA, 6 mA, 60 mA, 600 mA, 6 A, 10 A (kontinuerligt).
* **ACI Princip:** Strømsignalets spændingsfald over præcisionsshunts routes direkte gennem systemets True RMS IC.
* **Beskyttelse:** Elektronisk sikring/klamper på lavstrømsområderne og fysisk 10 A højeffektssikring (High Rupture Capacity - HRC) på 10 A området.

### Modstandsmåling (Ohms)
* **Topologi:** Understøtter både 2-leder og 4-leder (Kelvin) måling for at eliminere kabelmodstand.
* **Områder:** 600 Ω, 6 kΩ, 60 kΩ, 600 kΩ, 6 MΩ, 60 MΩ.
* **Max Opløsning:** 0,01 Ω (10 mΩ) på 600 Ω området.

## 1.0.1 SYSTEMARKITEKTUR OG OVERORDNEDE DESIGNKRAV
Dette instrument er et dual-zone, galvanisk isoleret præcisionsmultimeter, der forener en analog 16-bit front-end med en dual-core 32-bit DSP-motor. For at opnå fuld overensstemmelse med CAT III (600V) miljøer, SKAL systemet implementere en streng adskillelse mellem brugerens kontroldomæne og det flydende måledomæne.

### 1.1.1 Sikkerhed og Isolation (Zone-opdeling)
* **Zone A (Kontroldomæne):** Omfatter MCU (ESP32), 5" TFT-display, USB-CDC interface til PC samt den digitale 5V/3.3V strømforsyning
* **Zone B (Måledomæne):** Omfatter analog front-end (AFE), True RMS konverter, ADC, strømshunts og isoleret +/-5V strømforsyning
* **Isolationsbarriere:** Mellem Zone A og Zone B SKAL der være en fysisk PCB-krybeafstand på minimum 8,0 mm, der garanterer en dielektrisk modstandsdygtighed på >4000V RMS
* **Digital Signaloverførsel:** Al datatrafik over barrieren SKAL foregå via optokoblere (f.eks. PC817/6N137 til interrupt/relæer) og kapacitive isolatorer (ISO1541 til I2C)

## 1.2.0 METROLOGISKE SPECIFIKATIONER
Instrumentets hardware SKAL understøtte følgende måletekniske parametre via den integrerede 16-bit Texas Instruments ADS1115 Delta-Sigma ADC konfigureret til ±4,096V Full Scale ved 100 målinger pr. sekund (SPS).

### 1.2.1 Spændingsmåling (DCV & ACV)
| Parameter | Jævnspænding (DCV) | Vekselspænding (ACV - True RMS) |
| :--- | :--- | :--- |
| **Opløsning** | 4½ cifre (60.000 counts) | 4½ cifre (60.000 counts) |
| **Områder** | 600 mV, 6 V, 60 V, 600 V, 1000 V | 600 mV, 6 V, 60 V, 600 V, 750 V |
| **Præcision (1 år)** | < ±0,025% aflæsning + 5 counts | Båndbredde: 20 Hz til 100 kHz |
| **Inputimpedans (Lav range)** | > 1 GΩ (via JFET LSK170 / ADA4530 buffer) | DC-afkoblet |
| **Inputimpedans (Høj range)** | ≥ 10 MΩ | ≥ 10 MΩ |
| **Crest Faktor / RMS Konverter** | N/A | ≤ 3:1 v. fuldt udslag (Analog via AD8436) |

### 1.2.2 Strøm- og Modstandsmåling (DCI/ACI & Ohms)
| Parameter | Strømmåling (DCI / ACI) | Modstandsmåling (Ohms) |
| :--- | :--- | :--- |
| **Områder** | 600 µA, 6 mA, 60 mA, 600 mA, 6 A, 10 A | 600 Ω, 6 kΩ, 60 kΩ, 600 kΩ, 6 MΩ, 60 MΩ |
| **Topologi** | Burden-resistors differentielt til ADC (AC via RMS) | 2-leder og 4-leder (Kelvin) |
| **Strømkilde / Shunts** | 0,01 Ω / 5W Manganin (10A) + præcision (mA/µA) | LT1006 OP-amp m. LM336-2.5V reference |
| **Hardwarebeskyttelse** | Elektronisk klamper, PTC + 10A HRC sikring | PTC, 1N4148 klamper + LM393 komparator |
| **Max Opløsning** | Afhængig af range | 0,01 Ω (10 mΩ) på 600 Ω området |

## 1.3 HARDWARE SUBSYSTEM KRAV
Følgende afsnit specificerer komponentniveau-kravene for hver fysisk blokering på printpladen.

### 1.3.1 Strømforsyning (Blok 1 & Blok 2)
* **Blok 1 (Digital - Zone A):** SKAL forsynes fra en lineær nettransformer med en 9V AC RMS sekundærvikling (min. 10 VA). Ensretning SKAL anvende 4x 1N4007 dioder (eller tilsvarende 2A bro) med 10nF snubber-kondensatorer over hver diode for at undertrykke RF-støj. Spændingen reguleres til +5V (LM7805 på køleplade) til relæspoler og +3,3V (AMS1117-3.3) til ESP32, I2C-isolator (primær) og TFT-skærm.
* **Blok 2 (Analog - Zone B):** SKAL generere en balanceret, flydende dual-rail forsyning eksklusivt til analog-IC'erne via en separat center-tapped 9V-0-9V AC vikling. Center-tappet definerer GND_ISO. Reguleres af LM7805 (+5.0V VCC_ISO) og LM7905 (-5.0V VEE_ISO), lokalt afkoblet med 10 µF tantal og 100 nF keramisk.

### 1.3.2 Transientbeskyttelse & Input Front-End (Blok 3, Blok 4, Blok 10)
* **Transientbeskyttelse:** Første forsvarslinje over V/Ω og COM SKAL bestå af en 10 mm Metaloxid Varistor (S10K300/S10K420) i serie med et 600V GDT (Gasafleder) gnistgab. PCB-baner i denne sløjfe skal være min. 100 mil for at håndtere kaskadestrømme og udløse HRC-sikringen ved overskridelse af 560V DC.
* **Spændingsdeler (Auto-Ranging):** Dæmpningsnetværket SKAL bestå af 10 stk. 1 MΩ SMD-1206 modstande i serie (total 10 MΩ, 0,1% tolerance, ±25ppm/°C) for at eliminere risikoen for overfladeoverslag (arcing) ved 1000V. Relæ-styring (1:1, 1:10, 1:100) implementeres med ultra-lav-lækage Omron G6K signalrelæer.
* **Buffer og RMS:** 600 mV og 6 V signalerne SKAL føres via FET-buffer for >1 GΩ impedans. AC-signaler dirigeres til AD8436 True RMS-IC'en, der udnytter en 3,3 µF lav-lækage gennemsnitskondensator (C_AVG) for at generere et rent 0-4V DC signal proportionalt med RMS-værdien til ADC AIN1.

### 1.3.3 Hardware-Sikkerhed & "Hov-fejl" detektion (Blok 5)
Sikkerhedskredsløbet SKAL udelukke instrument- eller personskade, hvis 230V netspænding utilsigtet tilsluttes i modstandsmålingstilstand (Ohm-mode):
* **Fysisk barriere:** Signalet passerer en 600V CAT III rated PTC-termistor (100 mA holdestrøm), efterfulgt af 1N4148 klamperdioder til VCC_ISO og GND_ISO.
* **Hardware Interrupt-kredsløb:** En LM393 komparator overvåger spændingsknudepunktet. Hvis spændingen overstiger 4,5V, SKAL komparatoren på under 1 mikrosekund vippe udgangen til HØJ (5V).
* **Isolation af Interrupt:** Alarmsignalet transporteres øjeblikkeligt over isolationsbarrieren via en high-speed optokobler til ESP32 Pin 25 (PIN_FAULT_INT), hvilket udløser en IRAM_ATTR bundet Interrupt Service Routine i firmwaren.

## 1.4 HARDWARE-TIL-FIRMWARE GRÆNSEFLADE (PIN MAPPING)
ESP32 Dev Module fungerer som systemets MCU og SKAL forbindes eksakt efter følgende hardware pin-mapping for at garantere firmware-kompatibilitet og forhindre I/O konflikter:
* **I2C Bus (via ISO1541):** PIN_I2C_SDA = GPIO 21, PIN_I2C_SCL = GPIO 22 (termineret m. 10k pull-ups).
* **Auto-Ranging Relæer:** PIN_RELAY_DIV10 = GPIO 13, PIN_RELAY_DIV100 = GPIO 12, PIN_RELAY_AC_ROUTE = GPIO 14, PIN_RELAY_OHM = GPIO 27.
* **Katastrofe-Interrupt:** PIN_FAULT_INT = GPIO 25.
* **Hukommelse & Partitionering:** ESP32 SKAL konfigureres med et Custom Partition Table (eller No OTA 2MB APP/2MB SPIFFS) for at tillade indbygget LittleFS til lagring af SCPI CSV-datalogging.

## 1.5 KRAV TIL REAL-TIME OPERATIVSYSTEM (RTOS) HARDWARE INTEGRATION
Hardwaren og dens styring SKAL designes til at køre asynkront over ESP32'ens to kerner for at undgå datatab under I2C læsninger:
* **Core 1 (Hardware/Metrologi):** Låst til Task_Metrology. Styrer hardware-relæerne med en hard-kodet 50 ms "blanking" delay efter hver relæ-skifteoperation (for at lade mekanisk bounce og elektromotorisk kraft aftage), samt indlæser data fra ADS1115 i en 100-samples ringbuffer for digital 50Hz NMRR filtrering.
* **Core 0 (GUI/SCPI/VFS):** Håndterer TFT_eSPI (skærmopdatering via anti-flicker sort-overskrivningsteknik), USB-CDC Virtual COM port (SCPI parsing) og skrivning af CSV-data til det lokale filsystem.

## 6.0 KRAV TIL PCB-LAYOUT OG MEKANIK
For at bestå certificering (f.eks. ved et eksternt testhus), pålægges printudlægningen følgende absolutte restriktioner:
* **Creepage/Clearance:** Under INGEN omstændigheder må krybeafstanden mellem Zone A (Digital GND) og Zone B (Analog GND_ISO) være under 8,0 mm på nogen printlag. Der SKAL fræses isolationsslidser (milling slots) i PCB'et under optokoblere og den kapacitive I2C isolator (U7).
* **Termisk Design:** Den integrerede Manganin shunt (10A) og LM7805/LM7905 regulatorerne SKAL omgives af tilstrækkelige termiske kobber-polygoner og thermal vias for at forhindre lokal opvarmning, der kan introducere termoelektriske spændinger (Seebeck-effekt) i det analoge domæne.
* **Shielding (Skærmning):** Den analoge front-end (Blok 4, 5, og 8) SKAL designes med et massivt, uafbrudt stelplan (GND_ISO) på de indre lag, og følsomme højimpedansbaner (især fra 10 MΩ spændingsdeleren til JFET/OP-amp bufferen) SKAL forsynes med guard-rings bundet til det lokale signalpotentiale for at annullere lækstrømme i printpladematerialet.

---

# 2.0 SYSTEMARKITEKTUR OG FYSISK BLOKDIAGRAM
Kredsløbet er designet med en stringent dual-zone arkitektur for at imødekomme IEC 61010-1 CAT III (600V). Apparatet gennemskæres af en fysisk barriere med min. 8.0 mm krybeafstand, der sikrer >4000V RMS galvanisk isolation mellem det flydende måledomæne og brugerens kontroldomæne.

```text
       [ 230V AC Netindgang ]               [ USB-CDC Til PC (Isoleret) ]
                 │                                      ▲
         ┌───────┴───────┐                              │
         ▼               ▼                              │
┌────────────────┐   ┌────────────────────────────────────────────────────────┐
│ BLOK 1: STRØM  │   │ BLOK 2: STRØM ANALOG (ISOLERET)                        │
│ DIGITAL 5V/3.3V│   │ +5V, -5V, GND_ISO                                      │
└────────┬───────┘   └────────────────────────┬───────────────────────────────┘
         │                                    │
         ▼                                    ▼
┌────────────────┐            ┌───────────────────────────────────────────────┐
│ BLOK 7: KONTROL│            │ BLOK 3/10: FRONT-END & STRØMSHUNTS            │
│ ESP32 + 5" TFT │            │ Input: V/Ω, COM, 10A, mA/µA, Sense+, Sense-   │
└────────▲───────┘            └───────────────────────┬───────────────────────┘
         │                                            │
         │             ===============================│========================
         │             ISOLATIONSBARRIERE (>4000V)    │
         │             ===============================│========================
         │                                            ▼
         │                            ┌───────────────────────────────────────┐
         │                            │ BLOK 4: AUTO-RANGING & FET-BUFFER     │
         │                            └───────┬───────────────────────┬───────┘
         │                                    ▼ (DC)                  ▼ (AC)
         │                            ┌───────────────┐       ┌───────────────┐
         │                            │ BLOK 5: 4-WIRE│       │ BLOK 6: TRUE  │
         │                            │ KELVIN OHM &  │       │ RMS CONVERTER │
         │                            │ FEJL-KLAMPER  │       │ (AD8436)      │
         │                            └───────┬───────┘       └───────┬───────┘
         │                                    ▼                       ▼
         │                            ┌───────────────────────────────────────┐
         │                            │ BLOK 8: ADC (ADS1115 @ 100 SPS)       │
         │                            └───────────────────┬───────────────────┘
         │                                                │
         └───────────── [ BLOK 9: DIGITAL ISOLATION ] ◄───┘
```

---

# 3.0 DETALJEREDE HARDWARE SUBSYSTEMER

### BLOK 1: STRØMFORSYNING – DIGITAL KONTROLSIDE (ZONE A)
* **Funktion:** Leverer sikker, ikke-flydende forsyning til den berørbare brugerflade, MCU og PC-interface.
* **Komponent-niveau design:**
  * **Transformering:** Lineær nettransformer med dedikeret 9V AC RMS sekundærvikling (min. 10 VA kapacitet).
  * **Ensretning:** 4x 1N4007 (eller 2A monolitisk bro) forsynet med snubber-kondensatorer (10nF) over hver diode for at dræbe RF-støj fra diodernes reverse-recovery.
  * **Filtrering:** 1000 µF / 25V Low-ESR elektrolytkondensator + 100 nF X7R keramisk bypass.
  * **Regulering (5V):** LM7805 lineær LDO i TO-220 hus monteret på intern køleplade. Forsyner direkte spolerne i auto-ranging relæerne via Blok 9.
  * **Regulering (3.3V):** AMS1117-3.3 forsyner ESP32, 5" TFT-display og den primære side af den digitale I2C-isolator.

### BLOK 2: STRØMFORSYNING – ISOLERET ANALOGSIDE (ZONE B)
* **Funktion:** Genererer en ultra-lavstøjs, balanceret dual-rail forsyning eksklusivt til de følsomme analoge måle-IC'er. 100% galvanisk svævende for at afvise common-mode netstøj og forhindre katastrofale jordsløjfer.
* **Komponent-niveau design:**
  * **Transformering:** En separat sekundærvikling på 9V-0-9V AC (center-tapped). Center-tap danner det absolutte referencespændingspunkt (GND_ISO) for hele Zone B.
  * **Ensretning:** Dobbelt broudretter-topologi mod stjernestel.
  * **Regulering:** U2 (LM7805) skaber +5.0V (VCC_ISO) og U3 (LM7905) skaber -5.0V (VEE_ISO). Begge LDO'er afkobles lokalt ved terminalerne med 10 µF tantal og 100 nF keramiske kondensatorer.
  * **Isolation:** Krybeafstanden fra Blok 2 til Blok 1 er defineret til minimum 8.0 mm i PCB-layoutet, hvilket garanterer dielektrisk modstandsdygtighed over 4000V RMS.

### BLOK 3: TRANSIENTBESKYTTELSE (FRONT-END)
* **Funktion:** Det absolutte første forsvarsværk. Absorberer 6 kV lyn-transienter og klampes lynhurtigt for at redde præcisionsmodstandene og IC'erne længere nede i kæden.
* **Komponent-niveau design:**
  * **Primær dæmper:** 10 mm Metaloxid Varistor (MOV, type S10K300 eller S10K420) monteret direkte over V/Ω og COM bøsningerne.
  * **Gnistgab:** En Gasafleder (GDT, 600V breakdown) sidder i serie med MOV'en for at eliminere lækstrømme under normal måling og forhindre termisk runaway i MOV'en over tid.
  * **Signalhåndtering:** Ved normale spændinger (0-600V AC/DC) er kredsløbet usynligt. Overstiges 560V DC tænder MOV'en på <1 ns. Kæmpe strømme kortsluttes lokalt (spændingen klamper under 1000V), hvilket tvinger en foransiddende HRC-sikring til at bryde. PCB-banerne her skal være ekstra brede (min. 100 mil) for at håndtere kaskade-strømmen.

### BLOK 4: AUTO-RANGING, SPÆNDINGSDELER & FET-BUFFER
* **Funktion:** Dynamisk attenuering (dæmpning) af indgangssignalet, så det passerer ind i ADC'ens sikre og præcise arbejdsområde (0-4V), samt impedanstilpasning.
* **Komponent-niveau design:**
  * **Resistor Matrix:** Et 10 MΩ netværk sammensat af 10 stk. 1 MΩ SMD-1206 modstande i serie. Denne daisy-chain konstruktion sikrer, at der ved 1000V kun ligger 100V over hver modstand, hvilket eliminerer risiko for overslag på overfladen (arcing). Der benyttes 0.1% tolerance, ±25ppm/°C metalfilm for ultimativ termisk stabilitet.
  * **Auto-Ranging Relæer:** Omron G6K (eller lignende ultra-lav-lækage 5V signalrelæer) skifter matrixen mod GND_ISO for at ændre attenueringen (Divider-forhold: 1:1, 1:10, 1:100).
  * **FET-Buffer:** For de kritiske 600 mV og 6 V områder er 10 MΩ indgangsimpedans for lavt til laboratoriesammenhæng. Signalet dirigeres her via en dedikeret JFET-buffer (fx LSK170) eller en elektrometer-grade OP-amp (ADA4530), der hæver DC-indgangsimpedansen til > 1 GΩ. Outputtet sendes DC-koblet til ADC'en og AC-koblet til RMS-konverteren.

### BLOK 5: MODSTANDSMÅLING (2-WIRE & 4-WIRE KELVIN) & HARDWARE-KLAMPER
* **Funktion:** Ekstremt præcis modstandsudmåling uden indflydelse fra prøveledningernes iboende modstand. Integrerer Fluke-inspireret "Hov-fejl" beskyttelse, hvis brugeren tilslutter netspænding i ohm-mode.
* **Komponent-niveau design:**
  * **Konstantstrømskilde:** En præcisions-OP-amp (LT1006) med lav offset, styret af en ultra-stabil 2.5V reference (LM336-2.5). Sender en omskiftelig teststrøm (100 nA op til 1 mA afhængig af området) ud af "Force" terminalen.
  * **Kelvin Matrix:** 4-leder systemet lader "Force"-strømmen løbe ud gennem V/Ω og retur i COM. Samtidig læses det rene spændingsfald direkte på komponenten tilbage via to uafhængige "Sense+" og "Sense-" terminaler til ADC'en.
  * **Hardware-Beskyttelse:** PTC-termistor (100 mA holdestrøm, 600V CAT III rated) sidder i serie med målesignalet. Efter PTC'en er indsat lynhurtige 1N4148 klamper-dioder koblet mod VCC_ISO og GND_ISO.
  * **Fejldetektion (Interrupt):** Hvis 230V påtrykkes, klamper dioderne spændingen til +5.7V / -0.7V. PTC'en varmer op og bryder kredsen på få millisekunder. Samtidig overvåger en LM393 hardware-komparator spændingsknudepunktet. Overskrider det 4.5V, vipper komparatoren øjeblikkeligt (mikrosekunder) til HØJ (5V). Dette sender et hardware-interrupt via en optokobler i Blok 9 direkte til ESP32, som omgående relæ-frakobler indgangen og udløser alarm.

### BLOK 6: TRUE RMS CONVERTER
* **Funktion:** Matematisk analog udregning af True Root Mean Square på komplekse, non-sinusformede bølgeformer.
* **Komponent-niveau design:**
  * **RMS Engine:** AD8436 True RMS-to-DC konverter IC (valgt over AD736 grundet overlegen båndbredde op til 100 kHz og skalerbar Crest Faktor håndtering).
  * **Averaging Capacitor (C_AVG):** En 3.3 µF lav-lækage kondensator (tantal eller film) dikterer integrationstiden for at skabe et rent DC-output.
  * **Signalhåndtering:** Modtager det nedskalerede, AC-koblede signal fra Blok 4 eller Blok 10, udfører analog squaring og root-beregning, og leverer et 0-4V DC signal (direkte proportionalt med AC RMS spændingen/strømmen) til ADC AIN1.

### BLOK 10: STRØMMÅLING & PRÆCISIONSSHUNTS
* **Funktion:** Nøjagtig strøm-til-spænding konvertering via burden-resistors.
* **Komponent-niveau design:**
  * **Lavstrøm (µA/mA):** Kredsløbet anvender to præcisionsshunts (f.eks. 100 Ω til µA og 1 Ω til mA, 0.1% tolerance). Indgangen beskyttes mod strømstød via elektronisk klamper (BJT/MOSFET baseret strømbegrænser) og en PTC.
  * **Højstrøm (10A):** En dedikeret 0.01 Ω / 5W Manganin-shunt (for ultra-lav temperaturkoefficient). Beskyttes af en intern 10A / 1000V HRC (High Rupture Capacity) keramisk sikring (f.eks. Bussmann DMM-B-11A).
  * **Interface:** Spændingsfaldet over den aktive shunt måles differentielt af ADS1115 (for DC) og sendes via relæ-multiplexing til AD8436 (for AC).

### BLOK 8: 16-BIT ADC (ANALOG-TIL-DIGITAL KONVERTERING)
* **Funktion:** Hjertet i dataopsamlingen.
* **Komponent-niveau design:**
  * **IC:** Texas Instruments ADS1115 (16-bit Delta-Sigma ADC).
  * **Opsætning:** Den indbyggede PGA (Programmable Gain Amplifier) konfigureres via ESP32 til ±4.096V Full Scale.
  * **Kanal-routing:** AIN0 = DC Spænding, AIN1 = AC RMS Spænding, AIN2/AIN3 = Differential input fra Kelvin Sense linjerne / Strømshunts.
  * **Databus:** Kommunicerer 100 målinger pr. sekund (100 SPS) via den lokale I2C-bus, som termineres med 10k pull-ups til VCC_ISO.

### BLOK 9: DIGITAL ISOLATIONSBARRIERE
* **Funktion:** Usynlig galvanisk mur, der overfører styresignaler og data uden metallisk kontakt, hvilket tillader ESP32 at sidde sikkert forbundet til en PC (via USB).
* **Komponent-niveau design:**
  * **I2C Isolation:** U7 = ISO1541 (Texas Instruments). En kapacitiv bidirektionel I2C-isolator med 5000 Vrms isolation. Oversætter problemfrit Målesidens 5V_ISO logik til Kontrolsidens 3.3V logik.
  * **Fejl-Interrupt:** U8 = PC817 eller 6N137 High-Speed Optokobler. Overfører det tids-kritiske alarmsignal fra LM393 til ESP32's interrupt-pin på under et millisekund.
  * **Relæ-Driver Isolation:** Flere PC817 optokoblere bruges til at oversætte 3.3V GPIO-signaler fra ESP32 over til 5V_ISO relæ-spolerne i Blok 4.

### BLOK 7: KONTROLENHED, FIRMWARE (DSP) & BRUGERFLADE (ZONE A)
* **Funktion:** Systemets "Master Brain", der transformerer rå 16-bit data til "super"-niveau ydelse og muliggør SCPI-integration med eksternt udstyr.
* **Hardware:** ESP32-WROOM-32 forbundet til en 4.3" eller 5" TFT-skærm via SPI. Inkluderer en Piezo-buzzer.
* **Firmware (Digital Signal Processing):**
  * **Oversampling & Decimation:** Ved kontinuerligt at aflæse ADS1115 med 100 SPS og implementere et software "Moving Average" filter, kan opløsningen virtuelt forøges (opnår stabilt 4.5 cifre) og 50 Hz netstøj elimineres (NMRR > 60 dB).
  * **Software-Kalibrering:** Intet fysisk trimmepotentiometer. ESP32 lagrer 2-punkts kalibrering (Offset og Gain) for hvert range i dens interne NVS-flash.
  * **Dual-Display & Trend:** Udlæser AIN0 (DC) og AIN1 (AC) uafhængigt. Firmwaren kan f.eks. vise forsyningsspænding + ripple-støj samtidigt. Tegner rullende grafiske trendkort i realtid.
* **Ekstern Konnektivitet:**
  * **USB-CDC (Virtual COM):** Instrumentet emulerer en COM-port via ESP32'ens indbyggede USB-interface. Datalinjerne er isoleret grundet Blok 9.
  * **SCPI Emulering:** ESP32 parser standard SCPI kommandoer (f.eks. `MEAS:VOLT:DC?`).
  * **Python Logger:** Tillader et simpelt python script at streame tidstemplede målinger (100 Hz) direkte ned i et Excel/CSV dokument.

---

# 4.0 PRODUKTIONS- OG KONSTRUKTIONSDATAKRAV 
Dette afsnit definerer de eksakte krav til det dokumentations- og filgrundlag, der mangler for at kunne realisere multimeteret fysisk. Uden disse data kan apparatet ikke fremstilles, samles eller sikkerhedsgodkendes.

## 4.1 KOMPLETTE STRØMSKEMAER (SCHEMATICS)
De overordnede blokdiagrammer SKAL nedbrydes til fuldstændige, maskinlæsbare strømskemaer udviklet i et professionelt EDA-værktøj (f.eks. KiCad 7+ eller Altium Designer).
* **Kredsløbsforbindelser:** Skemaerne SKAL vise samtlige pin-til-pin forbindelser for alle integrerede kredsløb (ESP32, ADS1115, AD8436, isolatorer). Ingen "black boxes" er tilladt.
* **Logiske net:** Alle signalbaner SKAL navngives med tydelige net-labels (f.eks. `SPI_MOSI`, `I2C_SDA_ISO`, `RELAY_DIV10_CTRL`) for at muliggøre automatisk designregel-kontrol (DRC).
* **Passive Komponenter:** Samtlige støttekomponenter (pull-ups, afkoblingskondensatorer, filterkomponenter) SKAL være tegnet ind med eksakte værdier (f.eks. 10 kΩ, 100 nF) samt di-elektrisk type for kondensatorer (f.eks. C_AVG for RMS-konverteren SKAL angives som 3.3 µF Tantal/Film).

## 4.2 FULDSTÆNDIG STYKLISTE (BILL OF MATERIALS - BOM)
Der SKAL udarbejdes en udtømmende, produktionsklar stykliste i CSV- eller Excel-format. Hver eneste komponent på printpladen og i kabinettet skal være defineret uden plads til tvivl.
* **Nødvendige datakolonner:** Designator (f.eks. R14, C2), Værdi, Tolerance, Spændings-/Effektrating, Package/Footprint (f.eks. SMD 0805, 1206, TO-220), Producent (Manufacturer) og det specifikke Producent-Partnummer (MPN).
* **Strenge tolerancekrav:** For spændingsdeleren (Blok 4) og strømshunts (Blok 10) SKAL BOM'en specificere ultra-præcise komponenter (f.eks. 0.1% tolerance, ±25ppm/°C temperaturkoefficient).
* **Transformator-specifikation:** Den tidligere overordnede beskrivelse af nettransformeren SKAL erstattes af et specifikt MPN (f.eks. en godkendt støbt PCB-transformer fra Myrra, Hahn eller Block), der overholder: Primær 230V AC, Sekundær 1: 9V AC, Sekundær 2: 9V-0-9V AC center-tapped, med min. 10 VA samlet effekt.

## 4.3 PRINTUDLÆGNING (GERBER- OG CAD-FILER)
De elektriske sikkerhedsregler beskrevet i Afsnit 6.0 SKAL omsættes til faktiske produktionsfiler for mønsterkortet (PCB).
* **Kilde- og Produktionsfiler:** Det fulde CAD-projekt SKAL udleveres. Til fabrikation kræves komplette Gerber X2-filer, NC Drill-filer (til boringer) samt CPL/Pos-filer (Pick and Place) til maskinel SMD-montage.
* **PCB Stackup:** Designet SKAL specificeres med antal lag (anbefalet 4-lags for optimal skærmning), kobbertykkelse (f.eks. 1 oz indre lag, 2 oz ydre lag for at håndtere 10A spidsstrømme) og dielektrisk materiale (FR4, min. 1.6 mm tykkelse).
* **Sikkerhedsverifikation:** CAD-filerne SKAL fysisk inkorporere de 8.0 mm fræsede isolationsslidser under optokoblere og den kapacitive isolator. Krybeafstanden SKAL kunne verificeres direkte i Gerber-filerne. Guard-rings omkring højimpedansbaner (Blok 4) SKAL være frilagt fra loddemaske.

## 4.4 MEKANISK DESIGN OG KABINET (ENCLOSURE)
For at opfylde IEC 61010-1 CAT III kravene og sikre instrumentets ergonomi, SKAL der leveres komplet mekanisk dokumentation.
* **3D-Modeller:** Der SKAL udarbejdes 3D CAD-filer (STEP eller IGES format) af det fulde specialdesignede kabinet.
* **Skærm- og Printmontering:** Designet SKAL inkludere præcise monteringsbosser (standoffs) til hoved-PCB'et samt en integreret ramme (bezel) med tolerancer, der sikrer stabil fastgørelse af det 5" TFT-display uden at udsætte det for mekanisk stress.
* **Terminal-placering:** Frontpanelet SKAL dimensioneres så de 4 mm sikkerhedsbøsninger (V/Ω, COM, mA/µA, 10A) overholder standard-afstanden på 19 mm (0.75") fra center til center, så standardudstyr og dual-bananstik kan anvendes.
* **CAT III Fysisk Sikkerhed:** Kabinettet SKAL støbes i brandhæmmende plast (UL94 V-0 rating). Indvendigt SKAL det mekaniske design indeholde fysiske plastbarrierer (skillevægge), der omslutter 230V net-indgangen og HRC-sikringerne, for at forhindre at plasma eller gnister fra et eventuelt internt overslag kan nå brugeren eller den digitale kontrolzone.[cite: 1]