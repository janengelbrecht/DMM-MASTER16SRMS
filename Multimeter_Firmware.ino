// =========================================================================
// =========================================================================
//                                                                         
//   ██████╗ ███╗   ███╗███╗   ███╗    ███╗   ███╗ █████╗ ███████╗████████╗
//   ██╔══██╗████╗ ████║████╗ ████║    ████╗ ████║██╔══██╗██╔════╝╚══██╔══╝
//   ██║  ██║██╔████╔██║██╔████╔██║    ██╔████╔██║███████║███████╗   ██║   
//   ██║  ██║██║╚██╔╝██║██║╚██╔╝██║    ██║╚██╔╝██║██╔══██║╚════██║   ██║   
//   ██████╔╝██║ ╚═╝ ██║██║ ╚═╝ ██║    ██║ ╚═╝ ██║██║  ██║███████║   ██║   
//   ╚═════╝ ╚═╝     ╚═╝╚═╝     ╚═╝    ╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝   ╚═╝   
//                                                                         
//   ISOLERET CAT III 4½-CIFRET BÆNKMULTIMETER — 16SRMS                  
//   ESP32-WROOM-32 / ADS1115 16-bit ADC / AD8436 True RMS Frontend      
//                                                                         
// =========================================================================
//
// FILNAVN:          Multimeter_Firmware.ino
// FIRMWARE TYPE:    DMM-16SRMS-MASSIVE
// FIRMWARE VERSION: 6.0.0
// DOKUMENT-ID:      SRS-DMM-16SRMS-V6.0-MASSIVE
// PROJEKT:          DMM-MASTER 16SRMS
// PROJEKTKODE:      DMM-MASTER-16SRMS-2026
//
// -------------------------------------------------------------------------
// FIL- OG GODKENDELSESINFORMATION
// -------------------------------------------------------------------------
// DATO FOR AFSLUTNING:      5. november 2026
// DATO FOR GODKENDELSE:     5. november 2026
// GODKENDT AF:              R&D & Teknisk Dokumentation
// FIRMA:                    JEP-Electronics
// KLASSIFIKATION:           OFFENTLIG / OPEN SOURCE
// LIVSCYKLUS:               MASTER SPECIFICATION — RELEASE 6.0
// MÅLGRUPPE:                UDVIKLERE
//
// -------------------------------------------------------------------------
// 1. HVAD KAN DETTE MULTIMETER?
// -------------------------------------------------------------------------
// Dette firmware-system implementerer et fuldt funktionelt, isoleret CAT III
// bænkmultimeter med 4½ cifre (dvs. op til 19999 tællinger) på en ESP32-
// platform med ekstern 16-bit ADC (ADS1115) og en dedikeret True RMS-
// frontend (AD8436). Firmwaren er designet til at være den eneste kilde-
// kodefil i projektet og dækker hele kæden fra rå ADC-sampling til
// grafisk brugerflade, SCPI-kommunikation og datalogging.
//
// MÅLEFUNKTIONER (alle med automatisk og manuel ranging):
//
//   * DCV — Jævnspænding (DC Voltage)
//       - 5 gear: 600 mV, 6 V, 60 V, 600 V, 1000 V
//       - Måles single-ended på AIN0 via JFET-buffer
//       - 100-sample moving average giver 50 Hz NMRR
//
//   * ACV — Vekselspænding (AC Voltage, True RMS)
//       - 5 gear: 600 mV, 6 V, 60 V, 600 V, 750 V (RMS)
//       - Måles på AIN1 via AD8436 analog RMS-DC konverter
//       - Ingen software-RMS — ægte analog RMS med høj crest factor
//
//   * DCI — Jævnstrøm (DC Current)
//       - 6 gear: 600 µA, 6 mA, 60 mA, 600 mA, 6 A, 10 A
//       - Måles differentielt over præcisionsshunt (AIN2/AIN3)
//       - 10 A-shunt er 0,01 Ω Manganin med HRC-sikring
//
//   * ACI — Vekselstrøm (AC Current, True RMS)
//       - 6 gear: 600 µA, 6 mA, 60 mA, 600 mA, 6 A, 10 A
//       - Måles via AD8436 efter shunt
//
//   * OHM_2W — 2-leder modstandsmåling
//       - 6 gear: 600 Ω, 6 kΩ, 60 kΩ, 600 kΩ, 6 MΩ, 60 MΩ
//       - Teststrøm genereres af LT1006 + LM336-2.5 reference
//       - Måles differentielt over DUT (AIN2/AIN3)
//
//   * OHM_4W — 4-leder Kelvin-modstandsmåling
//       - 6 gear: samme som OHM_2W
//       - Eliminerer lednings- og kontaktmodstand
//       - Egne kalibreringsslots (28-33) adskilt fra 2W
//
//   * AC RIPPLE (parallelt dual-display)
//       - Når DCV er aktiv, polles AIN1 hvert 100 ms
//       - Viser AC-ripple på DC-signalet i Zone 3
//       - Skaleres korrekt med gearets dæmpningsfaktor
//
//   * TREND-GRAF (rullende graf-mode)
//       - 400-punkts cirkulær buffer i RAM
//       - Tegnes med tft.drawLine() i cyan på sort baggrund
//       - Aktiveres/deaktiveres via SCPI: DISP:GRAPH ON/OFF
//
//   * DATALOGGING (LittleFS CSV)
//       - Bulk-skrivning af 100 målinger ad gangen (wear leveling)
//       - Automatisk log-rotation ved 1 MB (log.csv → log_002.csv → ...)
//       - CSV-format: <timestamp_ms>,<value>
//
//   * SCPI-INTERFACE (USB-CDC Virtual COM Port)
//       - IEEE 488.2 / SCPI-1999 kommandoer
//       - *IDN?, *RST, READ?, SYST:REM, SYST:LOC
//       - CONF:VOLT:DC/AC, CONF:CURR:DC/AC, CONF:RES, CONF:FRES
//       - MMEM:CAT?, MMEM:DATA?, MMEM:DEL
//       - CAL:VAL, CAL:ZERO (kalibrering)
//       - DISP:GRAPH ON/OFF
//
// -------------------------------------------------------------------------
// 2. BEGRÆNSNINGER
// -------------------------------------------------------------------------
// Det er vigtigt at forstå, hvad dette instrument IKKE kan, og hvor grænsen
// for dets præcision og anvendelse ligger:
//
//   * 4½ CIFRE — IKKE 5½ ELLER 6½
//       Instrumentet giver op til 19999 tællinger. Det er tilstrækkeligt
//       til hobby-, undervisnings- og let professionelt brug, men det kan
//       IKKE erstatte et 6½-cifret laboratoriemultimeter til metrologi.
//
//   * INGEN FYSISK KNAP TIL TREND-GRAF
//       Trend-graf-mode aktiveres udelukkende via SCPI (DISP:GRAPH ON).
//       En fysisk knap er planlagt, men ikke implementeret i denne version.
//
//   * KALIBRERING ER IKKE FABRIKSVERIFICERET
//       Standardværdierne er gain=1.0, offset=0.0. Instrumentet SKAL
//       kalibreres mod en kendt reference (min. 6½ cifre) før brug i
//       målinger, hvor absolut nøjagtighed er kritisk.
//
//   * ADC'ENS 100 SPS-KRAV ER EN TILNÆRMELSE
//       ADS1115 har ingen 100 SPS-indstilling. Firmwaren bruger 475 SPS
//       (2,1 ms konvertering) + 10 ms delay = ~12 ms loop-tid → ~83 Hz.
//       100-sample-filteret dækker dermed ~1,2 s i stedet for præcis 1,0 s,
//       hvilket stadig giver tilstrækkelig 50 Hz NMRR, men ikke perfekt
//       afstemning til 50 Hz-perioden.
//
//   * SPI-DISPLAY BEGRÆNSER OPDATERINGSFREKVENS
//       TFT_eSPI kører SPI på 20 MHz. Ved fuld skærmopdatering (fillScreen)
//       kan opdateringsfrekvensen falde mærkbart. Anti-flicker-teknikken
//       (sort-overskrivning) og zone-baseret opdatering afhjælper dette,
//       men grafen kan stadig "hakke" ved høj sample-rate.
//
//   * INGEN FYSISK KNAP TIL RECOVERY
//       Efter en hardware-fejl (systemHalted) kræves *RST via USB.
//       Et fysisk reset-knap er planlagt, men ikke implementeret.
//
//   * ISOLATION ER KUN SÅ GOD SOM HARDWAREN
//       Firmwaren forudsætter, at ISO1541 og optokoblere giver >4000 V RMS
//       isolation. Hvis hardwaren ikke opfylder dette, er CAT III-ratingen
//       ikke gyldig — uanset hvad firmwaren gør.
//
//   * GPIO.out_w1tc VIRKER KUN FOR GPIO 0-31
//       ISR'ens register-skrivning er optimeret til de nuværende pins.
//       Hvis pin-mapping ændres til GPIO 32+, skal ISR omskrives til
//       GPIO.out1_w1tc. Dette er dokumenteret i koden.
//
//   * INGEN HUKOMMELSE TIL KALIBRERINGSHISTORIK
//       NVS gemmer kun de nyeste gain/offset-værdier. Der er ingen
//       versionshistorik eller mulighed for at rulle tilbage til en
//       tidligere kalibrering.
//
// -------------------------------------------------------------------------
// 3. FORDELE
// -------------------------------------------------------------------------
// På trods af begrænsningerne har dette design en række markante fordele,
// der gør det attraktivt til både undervisning, prototyping og let
// professionelt brug:
//
//   * 100 % OPEN SOURCE OG MONOLITISK
//       Hele firmwaren er én fil med 100 % inline-kommentarer. Det gør
//       koden let at læse, modificere og undervise ud fra. Ingen skjulte
//       headere eller binære blobs.
//
//   * TO-KERNE RTOS-ARKITEKTUR
//       Core 1 kører udelukkende metrologi (ADC, DSP, FSM) med høj
//       prioritet. Core 0 kører GUI, SCPI og datalogging med lav prioritet.
//       Det giver stabil sample-rate og responsiv brugerflade samtidig.
//
//   * ÆGTE TRUE RMS (IKKE SOFTWARE-RMS)
//       AD8436 konverterer AC til DC RMS analogt. Det giver korrekt
//       måling af ikke-sinusformede signaler (firkant, trekant, PWM)
//       med høj crest factor — noget software-RMS ofte fejler på.
//
//   * HARDWARE-SIKKERHED MED IRAM-ISR
//       LM393-komparatoren fanger 230 V i Ohm-mode og udløser en
//       IRAM-låst ISR, der slukker alle relæer på under 1 µs. Det er
//       hurtigere end nogen software-polling kan klare.
//
//   * WEAR-LEVELING DATALOGGING
//       Bulk-skrivning af 100 målinger ad gangen reducerer flash-slid
//       med en faktor 100 sammenlignet med naive per-sample skrivninger.
//       Log-rotation ved 1 MB forhindrer, at partitionen løber fuld.
//
//   * KALIBRERING PR. GEAR I NVS
//       Hvert eneste gear har sit eget gain/offset-par gemt i NVS.
//       Det giver mulighed for at kompensere for relæ- og modstands-
//       tolerancer individuelt — noget mange billige multimetre ikke gør.
//
//   * FULD SCPI-IMPLEMENTERING
//       Instrumentet kan styres fra LabVIEW, Python, MATLAB eller ethvert
//       andet SCPI-værktøj. Det gør det velegnet til automatiserede
//       testopstillinger og dataindsamling.
//
//   * DIFFERENTIEL MÅLING PÅ STRØM OG OHM
//       AIN2/AIN3 læses differentielt, hvilket fjerner stelstøj og
//       common-mode-fejl. Det er især vigtigt ved små strømme og
//       højimpedansmodstande.
//
//   * DUAL-DISPLAY MED AC RIPPLE
//       Når DCV er aktiv, vises AC-ripple parallelt i Zone 3. Det giver
//       brugeren et øjeblikkeligt billede af, hvor "ren" DC-spændingen er
//       — nyttigt ved fejlsøgning i strømforsyninger.
//
//   * ZONE-BASERET ANTI-FLICKER GRAFIK
//       Skærmen opdeles i zoner, og kun de berørte områder opdateres.
//       Det giver en rolig brugerflade uden det flicker, man ser på
//       naive fillScreen-baserede implementationer.
//
// -------------------------------------------------------------------------
// 4. SPECIELLE SIKKERHEDSFUNKTIONER
// -------------------------------------------------------------------------
// Sikkerhed er ikke en eftertanke i dette design — det er en central
// arkitektonisk prioritet. Følgende mekanismer beskytter både bruger
// og hardware:
//
//   * HARDWARE-KOMPARATOR MED IRAM-ISR (REQ-SAF-5.01)
//       LM393 overvåger, om der opstår farlig spænding i Ohm-mode.
//       Ved detektion udløses hardwareFaultISR(), der er placeret i
//       IRAM og bruger direkte GPIO-registeradgang (ikke digitalWrite).
//       Den tvinger alle relæer i safe state på under 1 µs.
//
//   * SPINLOCK-BESKYTTELSE AF RELÆER
//       ESP32 har dual-core, så en almindelig mutex virker ikke i ISR.
//       En portMUX_TYPE spinlock beskytter mod race mellem ISR og
//       Task_Metrology, når begge vil ændre relætilstand.
//
//   * SAFE STATE VED OPSTART OG FEJL
//       Ved opstart og ved enhver fejl tvinges relæerne til 1000 V-
//       tilstanden (DIV10+DIV100 aktive = 1:100 dæmpning). Det er den
//       mest beskyttede konfiguration og forhindrer, at brugeren
//       udsættes for farlig spænding ved en fejl.
//
//   * 100 ms RELÆ-BLANKING EFTER GEARSKIFT
//       Efter hvert relæskift ventes 100 ms, så kontakterne falder til
//       ro. Derefter fyldes filterbufferen med friske målinger (ikke
//       nuller), så relæstøj ikke forurener gennemsnittet.
//
//   * LÅST RECOVERY-TILSTAND (REQ-ERR-16.02)
//       Efter en hardware-fejl låses systemet i en uendelig løkke, der
//       kun reagerer på *RST via USB. Det forhindrer, at instrumentet
//       automatisk genoptager målingen, mens fejlen stadig er til stede.
//
//   * VISUEL KATASTROFE-ALARM (REQ-ERR-16.01)
//       Ved systemHalted farves skærmen blodrød, og en stor hvid
//       advarsel vises: "HIGH VOLTAGE OVERLOAD — REMOVE LEADS!".
//       Piezo-buzzeren aktiveres kontinuerligt med 2 kHz.
//
//   * CAT III-ADVARSEL VED >30 V
//       Når den målte spænding overstiger 30 V, vises "!CAT III!" i
//       rødt i Zone 1. Det advarer brugeren om, at der er tale om
//       farlig spænding, selvom instrumentet er designet til det.
//
//   * FILNAVNS-VALIDERING MOD PATH-TRAVERSAL
//       SCPI-kommandoer som MMEM:DATA? og MMEM:DEL validerer filnavne
//       mod "..", "/" og "\". Det forhindrer, at en ondsindet eller
//       fejlbehæftet PC-kommando kan overskrive eller slette filer
//       uden for LittleFS-partitionen.
//
//   * KALIBRERINGS-MUTEX BESKYTTER NVS
//       calibrationTable[] beskyttes af en mutex, så SCPI-kalibrering
//       og Task_Metrology ikke kan læse/skrive samtidig. Det forhindrer
//       korruption af kalibreringsdata under drift.
//
//   * LOG-ROTATION FORHINDRER PARTITIONS-FULDT
//       Når en logfil når 1 MB, oprettes automatisk en ny fil
//       (log_002.csv, log_003.csv, ...). Hvis alle 100 rotationsforsøg
//       mislykkes, sættes loggingEnabled = false, og GUI viser "LOG FULL".
//
//   * NON-BLOCKING SCPI MED KORT TIMEOUT
//       Serial.setTimeout(10) sikrer, at readStringUntil ikke blokerer
//       GUI-tasken i op til 1 sekund. Det holder brugerfladen responsiv,
//       selv når PC'en sender en ufuldstændig kommando.
//
// =========================================================================
// DENNE FIL ER DEN ENESTE KILDEKODEFIL I PROJEKTET (REQ-DOC-1.01).
// HVER IKKE-TOM LINJE HAR EN // KOMMENTAR (REQ-DOC-1.02).
// ALLE VARIABELNAVNE ER BESKRIVENDE OG IKKE FORKORTEDE (REQ-DOC-1.03).
// -------------------------------------------------------------------------
// VIGTIGT: TFT_eSPI kræver en User_Setup.h i biblioteksmappen med de
// fysiske display-pins. Det er et bibliotekskrav og kan ikke ligge her.
//
// VIGTIGT: Koden kræver Arduino-ESP32 core v3.x. RETTELSE (buglist punkt 2):
// LEDC-API'et er core 3.x-kompatibelt (ledcAttach + ledcWriteTone(pin, freq)),
// fordi ledcSetup() er fjernet i core 3.x, og ledcWriteTone() tager pin-nummer.
//
// VIGTIGT: GPIO.out_w1tc virker KUN for GPIO 0-31 — hvis pin-mapping
// ændres til GPIO 32+, skal ISR omskrives til GPIO.out1_w1tc.
//
// VIGTIGT: Displayet bruger SPI-interface (User_Setup.h), ikke 8-bit
// parallel. Controlleren er RA8875 (TFT_eSPI-kompatibel 800×480 SPI),
// da TFT_eSPI ikke har nogen SSD1963 SPI-driver. SPI kører 20 MHz.
// =========================================================================

// =========================================================================
// SEKTION 1: BIBLIOTEKER (INCLUDES) — REQ-STR-3.01
// Her hentes færdigskrevet kode ind, så vi ikke opfinder den dybe tallerken.
// =========================================================================
#include <Arduino.h>          // Henter grundlæggende funktioner til ESP32 processoren.
#include <Wire.h>             // Aktiverer I2C-kommunikation (bruges til at tale med ADC-chippen).
#include <SPI.h>              // Aktiverer SPI-kommunikation (bruges til lynhurtig grafik på TFT-skærmen).
#include <Adafruit_ADS1X15.h> // Henter Adafruits kode, så vi nemt kan læse volt fra ADS1115 chippen.
#include <TFT_eSPI.h>         // Henter grafik-motoren til vores display.
#include <LittleFS.h>         // Aktiverer ESP32'ens indbyggede "harddisk" så vi kan gemme CSV filer.
#include <Preferences.h>      // Aktiverer en lille hukommelse, der overlever strømafbrydelser (til kalibrering).
#include <math.h>             // Bruges til fabs(), isnan() og andre matematikfunktioner.
#include <esp_attr.h>         // Giver adgang til IRAM_ATTR og andre ESP32-attributter.
#include <esp_rom_sys.h>      // Giver adgang til esp_rom_delay_us() — IRAM-sikker forsinkelse i ISR.
#include <soc/gpio_reg.h>     // Giver adgang til GPIO-registre, så ISR kan sætte pins lynhurtigt (IRAM-sikkert).
#include <freertos/FreeRTOS.h> // Giver adgang til FreeRTOS mutex og spinlock funktioner.
#include <freertos/task.h>    // Giver adgang til vTaskDelay og task-håndtering.

// =========================================================================
// SEKTION 2: HARDWARE PINS OG DEFINITIONER — REQ-STR-3.02
// Her fortælles ESP32'eren, hvilke fysiske ben der er forbundet til hvad.
// =========================================================================

// -- I2C BUS TIL ADS1115 (Isoleret via ISO1541 - Blok 9) --
#define PIN_I2C_SDA 21              // Pin 21 fungerer som data-linje (Serial Data).
#define PIN_I2C_SCL 22              // Pin 22 fungerer som tidsgiver (Serial Clock).

// -- RELÆ STYRING TIL AUTO-RANGING (Blok 4 og Blok 5) --
#define PIN_RELAY_DIV10 13          // Når denne sættes HØJ (3.3V), aktiveres 1:10 spændingsdeleren via optokobleren.
#define PIN_RELAY_DIV100 12         // Når denne sættes HØJ, aktiveres 1:100 spændingsdeleren (op til 1000V).
#define PIN_RELAY_AC_ROUTE 14       // Når denne sættes HØJ, sendes signalet forbi AD8436 True RMS chippen (Blok 6).
#define PIN_RELAY_OHM 27            // Når denne sættes HØJ, udsendes en teststrøm til at måle modstand.

// -- SIKKERHED OG INTERRUPTS --
#define PIN_FAULT_INT 25            // Lytter på hardware-komparatoren (LM393 via optokobler). Bliver LAV (aktiv LOW) ved fejl.
#define PIN_BUZZER 26               // Lydgiver til at advare brugeren ved højspændingsfejl.

// -- LEDC OPSÆTNING TIL BUZZER (ESP32 core 3.x API) --
// RETTELSE (buglist punkt 2): I core 3.x er ledcSetup() fjernet, og LEDC-funktioner
// tager pin-nummer i stedet for kanal. BUZZER_LEDC_CHANNEL er derfor fjernet.
#define BUZZER_LEDC_RESOLUTION 8    // 8-bit opløsning giver 256 niveauer og tillader toner op til ~40 kHz.

// -- GPIO-BITMASKS TIL BRUG I ISR (IRAM-SIKKERT) --
// Disse konstanter gør det muligt at slukke flere pins på én gang med et enkelt register-skriv.
// BEMÆRK: Virker kun for GPIO 0-31. Hvis pin-mapping ændres til 32+, skal der bruges GPIO.out1_w1tc.
#define GPIO_BIT_RELAY_DIV10   (1UL << PIN_RELAY_DIV10)   // Bit-maske for 1:10 relæ-pin.
#define GPIO_BIT_RELAY_DIV100  (1UL << PIN_RELAY_DIV100)  // Bit-maske for 1:100 relæ-pin.
#define GPIO_BIT_RELAY_AC      (1UL << PIN_RELAY_AC_ROUTE) // Bit-maske for AC-route relæ-pin.
#define GPIO_BIT_RELAY_OHM     (1UL << PIN_RELAY_OHM)     // Bit-maske for Ohm-teststrøm relæ-pin.
#define GPIO_BITS_ALL_RELAYS   (GPIO_BIT_RELAY_DIV10 | GPIO_BIT_RELAY_DIV100 | GPIO_BIT_RELAY_AC | GPIO_BIT_RELAY_OHM) // Samlet maske til ISR safe state.
#define GPIO_BIT_RELAY_DIV10_AND_DIV100 (GPIO_BIT_RELAY_DIV10 | GPIO_BIT_RELAY_DIV100) // Bit-maske for begge spændingsdæmpere (bruges i ISR safe state).

// -- SPINLOCK TIL BESKYTTELSE AF RELÆER MOD SAMTIDIG ADGANG FRA ISR OG TASK --
// ESP32 har dual-core, så en almindelig mutex virker ikke i ISR. En spinlock gør.
static portMUX_TYPE relaySpinlock = portMUX_INITIALIZER_UNLOCKED; // Initialiserer spinlock i unlock-tilstand.

// =========================================================================
// SEKTION 3: GLOBALE DATASTRUKTURER OG VARIABLER — REQ-STR-3.03 / REQ-CAL-13.02
// Her oprettes de kasser og variabler, systemet bruger til at dele data.
// =========================================================================

// Struct til at sende den færdige måling fra den matematiske kerne til grafik-kernen.
// calibrationSlot og rawVoltage sendes SAMMEN, så CAL:VAL/CAL:ZERO altid bruger samme sample.
struct MeasurementPacket {
    double finalValue;              // Den matematiske, kalibrerede værdi der skal udskrives (Volt, Ampere eller Ohm).
    double rawVoltage;              // Den RÅ filtrerede spænding FØR kalibrering — bruges af CAL:VAL (REQ-CAL-13.03).
    double acRippleValue;           // Den separate AC-ripple-måling fra AIN1 (REQ-UI-14.02), skaleret til indgangsniveau.
    uint8_t currentRange;           // Fortæller hvilket gear vi er i (0=1000V safe, 1=600V, 2=60V, 3=6V, 4=600mV).
    uint8_t calibrationSlot;        // Den kalibreringsslot der blev brugt til DENNE pakke.
    bool isOverload;                // Bliver 'true' hvis spændingen overstiger, hvad elektronikken kan klare.
    bool isAcMode;                  // Bliver 'true' hvis vi kigger på vekselstrøm.
    bool isStale;                   // Bliver 'true' under gearskift, så PC'en ved at værdien er "stale".
    uint32_t timeStamp;             // Antal millisekunder siden apparatet blev tændt (bruges til log-filen).
};

// Struct til kalibreringsdata som beskrevet i REQ-CAL-13.02 (Gain og Offset pr. område).
struct CalibrationData {
    double gain;                    // Multiplikationsfaktor for at rette hældningsfejl (span-punkt).
    double offset;                  // Nul-punkts justering for at fjerne parasitisk spænding (zero-punkt).
};

// Enum til styring af hovedtilstandsmaskinen REQ-FSM-11.01.
enum class MeasurementMode {
    MODE_DCV,                       // Jævnspændingsmåling.
    MODE_ACV,                       // Vekselspændingsmåling (True RMS).
    MODE_DCI,                       // Jævnstrømsmåling.
    MODE_ACI,                       // Vekselstrømsmåling (True RMS).
    MODE_OHM_2W,                    // 2-leder modstandsmåling.
    MODE_OHM_4W                     // 4-leder Kelvin modstandsmåling.
};

// Antal understøttede måleområder (DCV 5 + ACV 5 + DCI 6 + ACI 6 + Ohm 6 + Ohm4W 6 = 34, rundet op til 40).
// UDVIDET fra 32 til 40, så OHM_4W får sine egne 6 slotte uden modulo-overlap (REQ-CAL-13.02).
#define CALIBRATION_SLOT_COUNT 40   // Fast antal kalibreringspladser i NVS, så indeksering er sikker.
#define FILTER_BUFFER_SIZE 100      // 100 samples ved 100 SPS = 1 sekund = 2 hele 50Hz-perioder (REQ-DSP-6.02).
#define TREND_BUFFER_SIZE 400       // Antal punkter i den rullende trend-graf (REQ-UI-14.03).
#define LOG_BUFFER_TARGET 100       // Antal målinger der samles i RAM før bulk-skrivning (REQ-MEM-15.01).
#define LOG_FILE_MAX_SIZE 1000000UL // Maksimal filstørrelse i bytes før log-rotation (REQ-MEM-15.02).
#define FILE_DUMP_CHUNK_SIZE 512    // Antal bytes der sendes pr. chunk under MMEM:DATA? (for at undgå GUI-blokering).
#define SCPI_READ_TIMEOUT_MS 10     // Maksimal tid readStringUntil må blokere, så GUI ikke fryser (REQ-SCPI-12.03).
#define LOG_BUFFER_MAX_LENGTH 4096  // Maksimal længde på RAM-log-buffer før tvungen flush (undgår OOM).
#define HIGH_VOLTAGE_WARNING_THRESHOLD 30.0 // Tærskel i volt for CAT III-advarselstrekant (REQ-UI-14.01).

QueueHandle_t dataQueue;            // Opretter et rør (en kø), hvorigennem vi sikkert kan sende data mellem de to kerner.
SemaphoreHandle_t relayMutex;       // Mutex der beskytter relæ-tilstand mod race mellem kerner (ikke ISR).
SemaphoreHandle_t calibrationMutex; // Mutex der beskytter calibrationTable[] mod race mellem kerner.
SemaphoreHandle_t trendMutex;       // Mutex der beskytter trend-buffer mod race mellem Core 0 og Core 1.
TFT_eSPI tft = TFT_eSPI();          // Starter et objekt, så vi kan styre TFT skærmen.
Preferences preferences;            // Starter et objekt til at læse/skrive kalibreringstal i flash-hukommelsen.
Adafruit_ADS1115 adc;               // Global ADC-instans, så både setup og metrologi-task kan tilgå den.
volatile MeasurementMode currentMode = MeasurementMode::MODE_DCV; // 'volatile' fordi GUI-tasken ændrer den, mens metrologi læser.
volatile bool systemHalted = false; // "volatile" betyder at variablen kan ændres pludseligt udefra af et interrupt.
volatile bool isRemoteMode = false; // Styrer om skærmen skal slukkes for at maksimere SCPI målehastighed.
volatile uint8_t relayGearIndex = 0; // 0=1000V safe, 1=600V, 2=60V, 3=6V, 4=600mV (global, deles mellem kerner).
volatile bool isStaleReading = false; // Bliver true under gearskift, så SCPI kan advare PC'en.
volatile bool externalRangeChangePending = false; // Sættes af SCPI ved gearskift, så Core 1 kan flushe filterbufferen (REQ-FSM-11.03).
volatile bool autoRangeEnabled = true; // RETTELSE (buglist punkt 4): Auto-ranging kan slås fra ved manuelt gear via SCPI.
CalibrationData calibrationTable[CALIBRATION_SLOT_COUNT]; // RAM-kopi af alle kalibreringskonstanter for lynhurtig adgang.
MeasurementPacket latestPacket = {0.0, 0.0, 0.0, 0, 0, false, false, true, 0}; // Seneste pakke; isStale=true indtil første gyldige måling.
volatile bool graphModeActive = false; // Styrer om trend-graf-mode er aktiv (REQ-UI-14.03); kan sættes via SCPI.
volatile bool remoteRescueRequested = false; // Sættes af SCPI-parseren, når *RST modtages under systemHalted.
volatile bool loggingEnabled = true; // Bliver false hvis LittleFS er fuld, så GUI kan vise "LOG FULL".
char logBuffer[LOG_BUFFER_MAX_LENGTH]; // Fast RAM-buffer til CSV-linjer (undgår dynamisk String-fragmentering).
int logBufferLength = 0;            // Antal tegn der aktuelt er i logBuffer.
volatile bool buzzerActive = false; // Husker om buzzeren hyler, så vi ikke gentager ledcWriteTone kald.

// =========================================================================
// SEKTION 4: HJÆLPEFUNKTIONER TIL NVS-KALIBRERING — REQ-CAL-13.01/13.02
// =========================================================================
//
// -------------------------------------------------------------------------
// 4.1 FORMÅL MED DENNE SEKTION
// -------------------------------------------------------------------------
// Denne sektion indeholder de to eneste funktioner i firmwaren, der taler
// direkte med ESP32'ens Non-Volatile Storage (NVS) — den lille flash-
// partition, der overlever strømafbrydelser og genstart. NVS bruges
// udelukkende til at gemme kalibreringskonstanter (gain og offset) for
// hvert eneste måleområde (gear). Uden denne sektion ville instrumentet
// altid måle med fabriksstandard (gain = 1.0, offset = 0.0), hvilket i
// praksis betyder, at relæ-, modstands- og dæmper-tolerancer aldrig
// bliver kompenseret, og at målingerne ville være upræcise.
//
// Der er præcis to funktioner i sektionen, og de har hvert sit ansvar:
//
//   * loadCalibrationFromNvs()  — LÆSER alle 40 slots fra NVS ind i RAM
//                                 ved opstart. Kaldes én gang fra setup().
//
//   * saveCalibrationToNvs()    — SKRIVER ét slot (gain+offset) fra RAM
//                                 til NVS. Kaldes fra SCPI-parseren, når
//                                 brugeren har gennemført en CAL:VAL eller
//                                 CAL:ZERO-kalibrering.
//
// -------------------------------------------------------------------------
// 4.2 HVILKE KRAV I KRAVSPECIFIKATIONEN OPF YLDES HER?
// -------------------------------------------------------------------------
// Denne sektion er direkte implementering af følgende krav fra
// SRS-DMM-16SRMS-V6.0-MASSIVE (kravspecifikation_firmware.md):
//
//   * [REQ-CAL-13.01] DEN MATEMATISKE MODEL
//     Kravet specificerer: "Hvert eneste gear (hver relækombination) skal
//     have sit eget sæt af kalibreringskonstanter. Koden bruger en lineær
//     transformation: V_display = (V_raw * Gain) + Offset".
//     OPFYLDELSE: Denne sektion gemmer og indlæser netop det gain/offset-
//     par, som den lineære model i Sektion 12 (Task_Metrology, trin 10)
//     bruger. Uden denne sektion ville modellen altid køre med gain=1.0
//     og offset=0.0, hvilket ikke opfylder kravet om "eget sæt af
//     kalibreringskonstanter pr. gear".
//
//   * [REQ-CAL-13.02] DATASTRUKTUR I FLASH
//     Kravet specificerer: "Der SKAL oprettes en struct CalibrationData
//     bestående af to double variabler (Gain og Offset) for de cirka 25
//     forskellige måleområder (...). Denne struct loades ind i RAM i
//     setup() for lynhurtig adgang under drift."
//     OPFYLDELSE: loadCalibrationFromNvs() er præcis den funktion, der
//     "loades ind i RAM i setup()". Struct'en CalibrationData er defineret
//     i Sektion 3, og RAM-kopien calibrationTable[CALIBRATION_SLOT_COUNT]
//     fyldes her. Kravet om "lynhurtig adgang under drift" opfyldes ved,
//     at Task_Metrology kun læser fra RAM — aldrig fra flash.
//
//   * [REQ-CAL-13.03] KALIBRERINGSPROCEDURE (KODENIVEAU) — delvist
//     Kravet specificerer to punkter: Zero-punkt (CAL:ZERO) og Span-punkt
//     (CAL:VAL). Selve SCPI-parsingen ligger i Sektion 11, men NVS-
//     lagringen af resultatet sker via saveCalibrationToNvs() i denne
//     sektion. Uden denne funktion ville CAL:VAL/CAL:ZERO kun ændre RAM
//     og ikke overleve en genstart — hvilket ville gøre kalibreringen
//     meningsløs i praksis.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet specificerer: "Hver eneste kodelinje (undtagen tomme linjer
//     til formatering) SKAL have en kommentar indledt med //."
//     OPFYLDELSE: Hver ikke-tom kodelinje i begge funktioner har en
//     //-kommentar, der forklarer HVAD og HVORFOR.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet specificerer: "En variabel må ikke hedde int r;, den skal
//     hedde int relayDivider10Pin;. Al logik skal kunne læses næsten som
//     almindeligt engelsk/dansk."
//     OPFYLDELSE: Variabelnavne som calibrationIndex, gainKey, offsetKey,
//     calibrationTable og CALIBRATION_SLOT_COUNT er alle beskrivende og
//     ikke-forkortede.
//
//   * [REQ-STR-3.03] GLOBALE DATASTRUKTURER
//     Kravet specificerer, at MeasurementPacket og de globale datastruk-
//     turer skal defineres i Sektion 3. CalibrationData og calibration-
//     Table er netop defineret der, og denne sektion er den eneste, der
//     initialiserer dem fra persistent lager.
//
//   * [REQ-RTOS-4.02] OPSÆTNING I SETUP()
//     Kravet specificerer, at setup() skal konfigurere RTOS og de
//     nødvendige datastrukturer. loadCalibrationFromNvs() kaldes fra
//     setup() (Sektion 15) som en del af denne opsætning, før de to
//     tasks startes — hvilket sikrer, at Task_Metrology aldrig ser
//     uinitialiserede kalibreringsdata.
//
//   * [REQ-MEM-15.01] SKRIVEBUFFER (WEAR LEVELING) — indirekte
//     Kravet handler om at minimere flash-slid. Denne sektion bidrager
//     ved KUN at skrive til NVS, når brugeren eksplicit kalibrerer
//     (CAL:VAL/CAL:ZERO) — aldrig under normal drift. Det er samme
//     filosofi som bulk-skrivning i datalogging: flash skal røres så
//     sjældent som muligt.
//
//   * [REQ-SAF-5.01] IRAM-LÅST INTERRUPT SERVICE ROUTINE — indirekte
//     Kravet handler om, at ISR'en skal kunne tvinge relæer i safe state
//     uden at afhænge af langsomme kald. Denne sektion påvirker ikke
//     ISR'en direkte, men ved at holde kalibreringsdata i RAM (ikke
//     flash) sikres det, at Task_Metrology aldrig blokerer på flash-
//     adgang, når ISR'en har prioritet.
//
//   * [REQ-ERR-16.02] LÅST RECOVERY-TILSTAND — indirekte
//     Efter en hardware-fejl og *RST genstartes målingen med de
//     kalibreringsdata, der ligger i RAM. Fordi loadCalibrationFromNvs()
//     kørte ved opstart, er data stadig gyldige efter recovery — uden
//     at der skal læses fra flash igen.
//
// -------------------------------------------------------------------------
// 4.3 HVORFOR NVS OG IKKE BARE KONSTANTER I KODEN?
// -------------------------------------------------------------------------
// Man kunne fristes til at hardcode gain/offset som konstanter i kilden.
// Det ville være en katastrofe af tre grunde:
//
//   1. HVER ENHED ER FORSKELLIG.
//      Selv med 0,1 % modstande i spændingsdeleren vil to fysiske
//      eksemplarer af DMM-MASTER have forskellige gain-værdier. NVS gør
//      det muligt at kalibrere hver enhed individuelt efter produktion.
//
//   2. KALIBRERING SKAL KUNNE GENTAGES.
//      Komponenter ældes (modstande driver, referencer ændrer sig).
//      Brugeren skal kunne gen-kalibrere uden at flashe ny firmware.
//      NVS gør det muligt via SCPI (CAL:VAL / CAL:ZERO).
//
//   3. DER ER 40 SLOTS.
//      Hvert gear (DCV 5 + ACV 5 + DCI 6 + ACI 6 + Ohm2W 6 + Ohm4W 6
//      = 34, rundet op til 40) har sit eget gain/offset-par. At hardcode
//      80 doubles i kilden ville være uoverskueligt og umuligt at
//      vedligeholde.
//
// -------------------------------------------------------------------------
// 4.4 HVORFOR EN RAM-KOPI (calibrationTable) OG IKKE DIREKTE NVS-LÆSNING?
// -------------------------------------------------------------------------
// NVS er flash-baseret og relativt langsom (hundreder af mikrosekunder pr.
// læsning). Task_Metrology læser kalibreringskonstanter ~83 gange i
// sekundet (én gang pr. sample-loop). Hvis hver læsning gik til flash,
// ville det både dræbe sample-raten og slide flash-hukommelsen op.
//
// Derfor indlæses HELE tabellen én gang ved opstart ind i RAM-variablen
// calibrationTable[CALIBRATION_SLOT_COUNT] (defineret i Sektion 3). Under
// drift læser Task_Metrology kun fra RAM — det tager nanosekunder og
// slider ikke flash. NVS bruges kun ved opstart (load) og ved kalibrering
// (save).
//
// -------------------------------------------------------------------------
// 4.5 HVAD KALDER DISSE FUNKTIONER? (KALDE-GRAF)
// -------------------------------------------------------------------------
// loadCalibrationFromNvs():
//   Kaldes ÉN gang fra setup() — se Sektion 15 (Opsætning og initiering).
//   Kaldes EFTER LittleFS.begin() og FØR ADC'en initialiseres, så
//   Task_Metrology allerede har gyldige kalibreringsdata, når den starter.
//   Kaldes ALDRIG fra Task_Metrology eller Task_GUI_SCPI under drift.
//
// saveCalibrationToNvs(calibrationSlot):
//   Kaldes fra handleScpiCommand() i Sektion 11 — specifikt fra:
//     * CAL:VAL <reference>  — efter ny gain er beregnet og skrevet til RAM.
//     * CAL:ZERO             — efter nyt offset er beregnet og skrevet til RAM.
//   Kaldes ALDRIG automatisk af systemet selv. Kun en eksplicit SCPI-
//   kommando fra brugeren kan udløse en NVS-skrivning. Det forhindrer
//   utilsigtet flash-slid (jf. REQ-MEM-15.01's filosofi).
//
// -------------------------------------------------------------------------
// 4.6 NVS-NØGLENAVNE OG DERES FORMAT
// -------------------------------------------------------------------------
// Hvert slot gemmes under to nøgler i NVS-navnerummet "calibration":
//
//   * "gain<N>"  — f.eks. "gain0", "gain7", "gain39"
//   * "offs<N>"  — f.eks. "offs0", "offs7", "offs39"
//
// Hvor <N> er calibrationIndex (0..39). Både gain og offset gemmes som
// double (8 bytes hver). NVS-navnerummet "calibration" er valgt, fordi
// Preferences.h kræver et navn på maks. 15 tegn, og "calibration" er
// kort nok og beskrivende.
//
// VIGTIGT: Nøglenavnene er BEVIDST korte ("offs" i stedet for "offset"),
// fordi NVS-nøgler maks. må være 15 tegn. "offset39" er 8 tegn og ville
// også passe, men "offs" er valgt for konsistens med den oprindelige kode
// og for at undgå forvirring, hvis nogen søger efter "offset" i NVS.
//
// -------------------------------------------------------------------------
// 4.7 HVAD SKER DER, HVIS NVS ER TOMT ELLER KORRUPT?
// -------------------------------------------------------------------------
// Preferences.getDouble(key, default) returnerer default-værdien, hvis
// nøglen ikke findes. Derfor:
//
//   * Første opstart (jomfruelig NVS): Alle 40 slots får gain=1.0,
//     offset=0.0. Instrumentet måler med fabriksstandard — upræcist,
//     men sikkert og funktionelt.
//
//   * Korrupt NVS: Samme som jomfruelig — default-værdier bruges. Der
//     vises ingen fejlmeddelelse, fordi default-værdier er sikre at
//     bruge (de giver blot dårligere præcision).
//
//   * NVS kan ikke åbnes: preferences.begin() returnerer false, men
//     koden ignorerer returværdien. Det betyder, at hvis NVS er fuldt
//     eller beskadiget, kører instrumentet videre med default-værdier
//     i stedet for at crashe. Det er et bevidst valg: et multimeter
//     med dårlig præcision er bedre end et multimeter, der ikke starter.
//
// -------------------------------------------------------------------------
// 4.8 TRÅDSIKKERHED — HVORFOR ER DER INGEN MUTEX HER?
// -------------------------------------------------------------------------
// loadCalibrationFromNvs() kaldes kun fra setup(), FØR de to RTOS-tasks
// er startet. Der er derfor ingen samtidig adgang, og ingen mutex er
// nødvendig.
//
// saveCalibrationToNvs() kaldes kun fra SCPI-parseren (Task_GUI_SCPI på
// Core 0). Den læser fra calibrationTable[], som beskyttes af
// calibrationMutex — men selve NVS-skrivningen sker EFTER mutex er
// frigivet i handleScpiCommand(). Det er bevidst: NVS-skrivning er
// langsom (op til 10 ms), og vi vil ikke holde calibrationMutex så
// længe, at Task_Metrology blokeres. Risikoen for race er minimal,
// fordi kun én SCPI-kommando ad gangen behandles (Serial er seriel),
// og Task_Metrology kun LÆSER tabellen (den skriver aldrig).
//
// -------------------------------------------------------------------------
// 4.9 YDELSESOVERVEJELSER
// -------------------------------------------------------------------------
// loadCalibrationFromNvs() tager ca. 40 × 2 × ~100 µs = ~8 ms ved opstart.
// Det er ubetydeligt i forhold til den samlede boot-tid (LittleFS mount,
// TFT init, ADC init tager tilsammen ~500 ms).
//
// saveCalibrationToNvs() tager ca. 2 × ~5 ms = ~10 ms pr. kald. Det sker
// kun, når brugeren eksplicit kalibrerer, så det påvirker ikke drift.
//
// =========================================================================

// -------------------------------------------------------------------------
// 4.10 FUNKTION: loadCalibrationFromNvs()
// -------------------------------------------------------------------------
// Formål:    Indlæs alle 40 kalibreringsslots fra NVS til RAM.
// Kaldes af: setup() — én gang ved opstart, før RTOS-tasks startes
//            (Sektion 15, jf. REQ-RTOS-4.02).
// Kalder:    Preferences.begin/end/getDouble (Arduino-ESP32 core).
// Effekt:    calibrationTable[0..39].gain og .offset fyldes med enten
//            gemte værdier eller default (1.0 / 0.0).
// Returværdi: Ingen (void).
// Side-effekter: Åbner og lukker NVS-navnerummet "calibration" i
//            read-only mode. Skriver INTET til flash.
// Opfylder:  REQ-CAL-13.01 (lineær model — leverer konstanterne),
//            REQ-CAL-13.02 (struct loades i RAM ved setup),
//            REQ-RTOS-4.02 (del af setup's initialisering),
//            REQ-DOC-1.02 (hver linje kommenteret),
//            REQ-DOC-1.03 (beskrivende variabelnavne).
// -------------------------------------------------------------------------.
void loadCalibrationFromNvs() {
    preferences.begin("calibration", true); // Åbn NVS-området i read-only mode, så vi ikke utilsigtet skriver.
    for (int calibrationIndex = 0; calibrationIndex < CALIBRATION_SLOT_COUNT; calibrationIndex++) { // Løb alle pladser igennem.
        String gainKey = "gain" + String(calibrationIndex);     // Byg nøglenavn for gain, f.eks. "gain7".
        String offsetKey = "offs" + String(calibrationIndex);   // Byg nøglenavn for offset, f.eks. "offs7".
        calibrationTable[calibrationIndex].gain = preferences.getDouble(gainKey.c_str(), 1.0);   // Standard gain = 1.0 hvis intet gemt.
        calibrationTable[calibrationIndex].offset = preferences.getDouble(offsetKey.c_str(), 0.0); // Standard offset = 0.0 hvis intet gemt.
    }
    preferences.end();                  // Luk NVS-området, så andre kan åbne det.
}

// -------------------------------------------------------------------------
// 4.11 FUNKTION: saveCalibrationToNvs(int calibrationSlot)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Gemmer ÉN kalibreringspost — bestående af et gain og et offset — for
//   et givet kalibreringsslot (0..39) fra RAM-tabellen calibrationTable[]
//   ned i ESP32'ens Non-Volatile Storage (NVS). NVS er den lille flash-
//   partition, der overlever strømafbrydelser og genstart. Efter kaldet
//   vil en efterfølgende genstart af instrumentet indlæse præcis disse
//   værdier via loadCalibrationFromNvs() — og dermed bevare kalibreringen.
//
//   Funktionen skriver PRÆCIS to nøgler til NVS-navnerummet "calibration":
//     * "gain<N>"  — gain-værdien (double) for slot <N>.
//     * "offs<N>"  — offset-værdien (double) for slot <N>.
//   Hvor <N> = calibrationSlot (0..39). Begge skrives som 8-byte double.
//
// BEGRUNDELSE (HVORFOR):
//   Uden denne funktion ville CAL:VAL og CAL:ZERO kun ændre RAM-værdierne
//   i calibrationTable[]. De ville forsvinde ved næste strømafbrydelse,
//   og brugeren skulle kalibrere forfra hver gang instrumentet tændes.
//   Det ville gøre kalibrering meningsløs i praksis. NVS-skrivningen er
//   derfor det, der gør kalibreringen PERSISTENT.
//
//   Funktionen er bevidst adskilt fra loadCalibrationFromNvs() (som kun
//   læser) og fra SCPI-parseren (som kun beregner). Det giver ét klart
//   ansvar pr. funktion: SCPI beregner → denne funktion gemmer → load
//   indlæser ved næste opstart. Ingen af funktionerne blander roller.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   saveCalibrationToNvs() kaldes UDELUKKENDE fra handleScpiCommand() i
//   Sektion 11 — og kun fra to eksakte steder:
//
//     1) Efter CAL:VAL <reference> er behandlet:
//        - SCPI-parseren har verificeret, at referenceValue > 0 og at
//          snapshot.rawVoltage er ikke-nul.
//        - Den har beregnet ny gain = reference / rawVoltage.
//        - Den har skrevet den nye gain til calibrationTable[slot].gain
//          UNDER calibrationMutex.
//        - Derefter kaldes saveCalibrationToNvs(activeSlot) for at gøre
//          ændringen persistent.
//
//     2) Efter CAL:ZERO er behandlet:
//        - SCPI-parseren har beregnet nyt offset = -(raw * gain).
//        - Den har skrevet det nye offset til calibrationTable[slot].offset
//          UNDER calibrationMutex.
//        - Derefter kaldes saveCalibrationToNvs(activeSlot) for at gøre
//          ændringen persistent.
//
//   Funktionen kaldes ALDRIG:
//     * Fra setup() (det er load-funktionens domæne).
//     * Fra Task_Metrology (den må kun LÆSE kalibreringsdata).
//     * Automatisk af systemet selv ved opstart, fejl eller recovery.
//     * Fra nogen form for periodisk eller tidsstyret logik.
//   Det betyder, at flash KUN skrives, når brugeren eksplicit beder om
//   det via en SCPI-kommando — hvilket minimerer flash-slid (jf. samme
//   filosofi som REQ-MEM-15.01's wear leveling for datalogging).
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-CAL-13.01] DEN MATEMATISKE MODEL
//     Kravet kræver, at hvert gear har sit eget sæt kalibrerings-
//     konstanter, og at modellen V_display = (V_raw * Gain) + Offset
//     bruges. OPFYLDELSE: Denne funktion er det, der gør de beregnede
//     Gain/Offset-værdier PERSISTENTE pr. gear. Uden den ville modellen
//     kun have fabriksdefault (1.0/0.0) ved hver opstart, og kravet om
//     "eget sæt af kalibreringskonstanter pr. gear" ville kun være
//     opfyldt i RAM — ikke over tid.
//
//   * [REQ-CAL-13.02] DATASTRUKTUR I FLASH
//     Kravet kræver en struct CalibrationData (Gain + Offset) for hvert
//     måleområde, loadet ind i RAM i setup() for lynhurtig adgang.
//     OPFYLDELSE: saveCalibrationToNvs() er den funktion, der skriver
//     netop denne struct's to felter (gain og offset) til flash, så
//     loadCalibrationFromNvs() kan indlæse dem ved næste opstart.
//     Funktionen er den "skrive-side" af REQ-CAL-13.02, som load-
//     funktionen er "læse-siden" af.
//
//   * [REQ-CAL-13.03] KALIBRERINGSPROCEDURE (KODENIVEAU)
//     Kravet specificerer to punkter — Zero-punkt (CAL:ZERO) og Span-
//     punkt (CAL:VAL) — og kræver, at MCU'en GEM MER resultatet.
//     OPFYLDELSE: Denne funktion er præcis den "gem"-mekanisme, som
//     kravet refererer til. CAL:VAL/CAL:ZERO beregner værdierne; denne
//     funktion gemmer dem. Uden den ville kravet kun være halvt opfyldt
//     (beregning uden persistens).
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar,
//     der forklarer HVAD og HVORFOR. OPFYLDELSE: Hver linje i funktionen
//     er kommenteret, og denne indledning forklarer yderligere HVORFOR
//     og HVEM der kalder.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: calibrationSlot, gainKey, offsetKey, calibrationTable
//     og CALIBRATION_SLOT_COUNT er alle beskrivende og ikke-forkortede.
//
//   * [REQ-MEM-15.01] SKRIVEBUFFER (WEAR LEVELING) — indirekte
//     Kravet kræver, at flash kun skrives i bulk for at minimere slid.
//     OPFYLDELSE: Denne funktion følger samme filosofi — den skriver
//     KUN, når brugeren eksplicit kalibrerer, aldrig under drift. Dermed
//     røres flash så sjældent som muligt.
//
//   * [REQ-RTOS-4.02] OPSÆTNING I SETUP() — indirekte
//     Kravet kræver, at setup() konfigurerer datastrukturerne.
//     OPFYLDELSE: saveCalibrationToNvs() er den funktion, der gør det
//     muligt for setup()'s loadCalibrationFromNvs() at finde gemte
//     værdier ved næste opstart — dvs. den er forudsætningen for, at
//     setup()'s indlæsning giver mening over tid.
//
//   * [REQ-ERR-16.02] LÅST RECOVERY-TILSTAND — indirekte
//     Kravet kræver, at systemet efter *RST går tilbage til sikker
//     tilstand uden at miste kalibrering. OPFYLDELSE: Fordi kalibreringen
//     ligger i NVS (skrevet af denne funktion), overlever den både
//     hardware-fejl, *RST og strømafbrydelse. Recovery behøver ikke at
//     kalibrere igen.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR VALIDERES calibrationSlot FØRST?
//     Hvis kalderen ved en fejl sender et slot uden for 0..39, ville
//     calibrationTable[calibrationSlot] læse uden for arrayets grænser
//     — det er udefineret adfærd i C++ og kan korrumpere RAM. Derfor
//     returneres øjeblikkeligt uden at skrive noget, hvis slot er
//     ugyldigt. Funktionen fejler "lydløst" (ingen fejlmeddelelse),
//     fordi det kun kan ske ved en programmeringsfejl — ikke ved
//     normal brug.
//
//   * HVORFOR ÅBNES NVS I SKRIVE-MODE (false)?
//     preferences.begin("calibration", false) åbner navnerummet i
//     read-write mode. Det er nødvendigt for putDouble(). Hvis man
//     åbnede i read-only (true), ville putDouble() fejle lydløst, og
//     kalibreringen ville ikke blive gemt.
//
//   * HVORFOR LUKKES NVS MED preferences.end()?
//     preferences.end() committer ændringerne til flash og frigiver
//     navnerummet, så andre funktioner (f.eks. en senere load) kan
//     åbne det. Uden end() ville data ligge i en buffer og muligvis
//     gå tabt ved strømafbrydelse.
//
//   * HVORFOR ER DER INGEN MUTEX HER?
//     Funktionen kaldes kun fra SCPI-parseren (Task_GUI_SCPI på Core 0),
//     og kun én SCPI-kommando behandles ad gangen (Serial er seriel).
//     Task_Metrology læser kun calibrationTable[] — den skriver aldrig.
//     Der er derfor ingen samtidig skrivning til samme slot, og ingen
//     mutex er nødvendig. Selve NVS-skrivningen sker desuden EFTER
//     calibrationMutex er frigivet i handleScpiCommand(), så Task_Metrology
//     ikke blokeres i op til 10 ms under flash-skrivningen.
//
//   * HVORFOR FEJLHÅNDTERES PUTDOUBLE IKKE?
//     preferences.putDouble() returnerer antal bytes skrevet (0 ved fejl).
//     Koden ignorerer returværdien bevidst: Hvis NVS er fuld eller
//     beskadiget, vil kalibreringen ikke blive gemt — men instrumentet
//     fortsætter med den nye værdi i RAM indtil næste genstart. Det er
//     et bevidst kompromis: en manglende NVS-skrivning er ikke kritisk
//     nok til at stoppe instrumentet eller vise en fejl til brugeren.
//
//   * YDELSE:
//     Funktionen tager ca. 2 × ~5 ms = ~10 ms pr. kald (to putDouble-
//     operationer plus begin/end). Det sker kun ved eksplicit kalibrering,
//     så det påvirker ikke driftens sample-rate.
//
//   * SIDE-EFFEKTER:
//     - Skriver to nøgler til NVS-navnerummet "calibration".
//     - Øger flash-slid med én skrivecyklus pr. kald (ubetydeligt, da
//       kaldet er sjældent).
//     - Ændrer IKKE calibrationTable[] i RAM (det er kalderens ansvar).
//     - Ændrer IKKE relayGearIndex, systemHalted eller andre globals.
//
//   * RETURVÆRDI:
//     Ingen (void). Funktionen signalerer ikke succes eller fejl.
//     Kalderen (handleScpiCommand) antager, at kaldet lykkedes, og
//     svarer "OK" til PC'en. Hvis NVS-skrivningen fejlede, vil
//     kalibreringen stadig virke indtil næste genstart — hvorefter
//     loadCalibrationFromNvs() indlæser den GAMLE værdi. Det er en
//     kendt og accepteret begrænsning.
//
// -------------------------------------------------------------------------
void saveCalibrationToNvs(int calibrationSlot) {
    if (calibrationSlot < 0 || calibrationSlot >= CALIBRATION_SLOT_COUNT) return; // Afvis ugyldigt slot for at undgå hukommelseskorruption.
    preferences.begin("calibration", false);                    // Åbn NVS-området i skrive-mode.
    String gainKey = "gain" + String(calibrationSlot);          // Byg nøglenavn for gain.
    String offsetKey = "offs" + String(calibrationSlot);        // Byg nøglenavn for offset.
    preferences.putDouble(gainKey.c_str(), calibrationTable[calibrationSlot].gain);     // Skriv gain til NVS.
    preferences.putDouble(offsetKey.c_str(), calibrationTable[calibrationSlot].offset); // Skriv offset til NVS.
    preferences.end();                                          // Luk NVS-området, så data committes.
}

// =========================================================================
// SEKTION 5: RELÆ-STYRING OG SAFE STATE — REQ-FSM-11.01 / REQ-AFE-7.03
// =========================================================================

// -------------------------------------------------------------------------
// 5.5 FUNKTION: forceRelaysToSafeState()
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Tvinger ALLE fire relæer på printpladen til deres sikre udgangspunkt
//   — den såkaldte "1000V safe state" — uanset hvilken tilstand de stod i
//   før kaldet. Konkret betyder det:
//
//     * AC-ruten (AD8436 True RMS) FRAKOBLES  → kun DC-vejen er aktiv.
//     * Ohm-teststrømmen AFBRYDES øjeblikkeligt → ingen strøm ud til DUT.
//     * 1:10-dæmperen (DIV10) TÆNDES          → del af 1:100-konfigurationen.
//     * 1:100-dæmperen (DIV100) TÆNDES        → sikrer ægte 1:100 dæmpning.
//     * relayGearIndex sættes til 0            → global markering af safe state.
//
//   Resultatet er, at indgangen er dæmpet 1:100 (maksimal beskyttelse),
//   at der ikke sendes teststrøm ud, og at AC-signalvejen er afbrudt.
//   Det er den MINDST farlige tilstand, instrumentet kan indtage.
//
// BEGRUNDELSE (HVORFOR — inkl. RETTELSE buglist C/E):
//   Den oprindelige kode satte ALLE relæer LOW i den tro, at det var
//   den sikreste tilstand. Det var en farlig fejl. Ifølge hardware-
//   dokumentationen (HW-DOC tabel 4.2) er:
//
//     - DIV10 LOW  + DIV100 LOW   → 1:1 via JFET-buffer (MINDST beskyttet!)
//     - DIV10 HIGH + DIV100 HIGH  → 1:100 (MEST beskyttet, dokumenteret safe)
//
//   Ved at sætte alt LOW ramte den gamle kode altså 1:1-tilstanden, hvor
//   indgangen er direkte forbundet til FET-bufferen. Ved 1000V ville det
//   ødelægge bufferen og potentielt udgøre en sikkerhedsrisiko for brugeren.
//
//   RETTELSEN (buglist C/E) er derfor at tænde DIV10+DIV100 eksplicit, så
//   vi rammer den dokumenterede 1:100-tilstand — den tilstand, hvor
//   indgangsspændingen dæmpes mest muligt, før den når følsom elektronik.
//
//   Funktionen bruges ved opstart og ved katastrofe, men IKKE ved normale
//   gearskift i Ohm-mode. Ved Ohm-mode bevares teststrømmen (se
//   selectRelayGear i Sektion 5), fordi et gearskift midt i en måling
//   ellers ville afbryde målingen unødigt.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   forceRelaysToSafeState() (denne ikke-ISR variant) kaldes fra:
//
//     1) setup() i Sektion 15:
//        - Kaldes umiddelbart efter pinMode-konfigurationen og FØR
//          attachInterrupt(). Det sikrer, at relæerne er i safe state,
//          allerede inden den første interrupt kan forekomme.
//        - Formål: garantere sikker tilstand fra første øjeblik.
//
//     2) Task_GUI_SCPI i Sektion 14 — to steder:
//        a) I katastrofe-recovery-blokken (efter *RST under systemHalted):
//           - Kaldes efter systemHalted er ryddet, buzzer er stoppet, og
//             currentMode er sat til MODE_DCV.
//           - Formål: bringe relæerne tilbage til en kendt, sikker tilstand
//             efter en fejl, før målingen genoptages.
//        b) I *RST-blokken uden for fejltilstand:
//           - Kaldes når en *RST modtages, mens systemet kører normalt.
//           - Formål: SCPI-standarden kræver, at *RST bringer instrumentet
//             til en veldefineret reset-tilstand — her 1000V safe state.
//
//   Funktionen kaldes ALDRIG:
//     * Fra Task_Metrology (den bruger selectRelayGear til gearskift).
//     * Fra ISR'en (den bruger forceRelaysToSafeStateISR, se nedenfor).
//     * Fra SCPI-kommandoer som CONF:VOLT:DC, CONF:RES osv. (de bruger
//       selectRelayGear, som bevarer Ohm-teststrømmen hvor relevant).
//     * Periodisk eller tidsstyret.
//
//   VIGTIGT — FORHOLDET TIL ISR-VARIANTEN:
//   Denne funktion bruger digitalWrite(), som IKKE er IRAM-sikker og
//   derfor IKKE må kaldes fra en ISR. Til ISR'en findes en separat
//   funktion forceRelaysToSafeStateISR() (Sektion 5), der bruger direkte
//   GPIO-registeradgang (GPIO.out_w1tc / GPIO.out_w1ts) og dermed er
//   IRAM-sikker. De to funktioner gør det SAMME — men med forskellige
//   implementationsmidler, fordi de kaldes fra forskellige kontekster.
//
//   Denne ikke-ISR variant forudsætter, at kalderen har taget relayMutex
//   (hvis den kaldes fra en task, der kan sameksistere med Task_Metrology).
//   ISR-varianten forudsætter, at kalderen har taget relaySpinlock.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE)
//     Kravet specificerer: "Når der skiftes mellem disse [modes], SKAL
//     processoren tvinge alle relæer i deres 'Safe State' (1000V range,
//     alle teststrømme afbrudt), vente 100 ms, og derefter koble de nye
//     relæer ind."
//     OPFYLDELSE: forceRelaysToSafeState() er præcis den funktion, der
//     etablerer "Safe State" (1000V range, teststrøm afbrudt). Den kaldes
//     ved opstart og ved *RST — de to situationer, hvor FSM'en skal
//     bringes til en kendt udgangstilstand. Selve 100 ms-ventetiden og
//     indkoblingen af nye relæer håndteres af selectRelayGear().
//
//   * [REQ-SAF-5.01] IRAM-LÅST INTERRUPT SERVICE ROUTINE
//     Kravet specificerer: "Når komparatoren på printet opdager 230V i
//     ohm-mode, sender den et 5V signal til PIN_FAULT_INT. Koden skal
//     afbryde alt andet og reagere på under 1 mikrosekund."
//     OPFYLDELSE: Denne funktion er "søsterfunktionen" til ISR-varianten.
//     Sammen sikrer de to funktioner, at relæerne kan tvinges i safe state
//     fra BÅDE task-kontekst (denne funktion) OG ISR-kontekst (ISR-varianten).
//     Uden denne funktion ville setup() og *RST ikke kunne etablere safe
//     state uden at duplikere logik.
//
//   * [REQ-SAF-5.02] AKTIVERING
//     Kravet specificerer, hvordan ISR'en tilkobles. OPFYLDELSE: Denne
//     funktion kaldes i setup() FØR attachInterrupt(), så relæerne er i
//     safe state, inden den første interrupt kan forekomme. Det er en
//     forudsætning for, at REQ-SAF-5.02's aktivering er sikker.
//
//   * [REQ-AFE-7.03] RELÆ BLANKING — indirekte
//     Kravet specificerer, at der skal ventes 50 ms efter relæskift, og
//     at filterbufferen skal fyldes op. OPFYLDELSE: Denne funktion sætter
//     relæerne i safe state, hvorefter kalderen (setup eller *RST) kan
//     tillade, at Task_Metrology genstarter med en frisk buffer. Selve
//     blanking-tiden håndteres af selectRelayGear(), men denne funktion
//     er forudsætningen for, at blanking sker fra en kendt tilstand.
//
//   * [REQ-ERR-16.02] LÅST RECOVERY-TILSTAND
//     Kravet specificerer: "Først her [efter *RST] må systemet gå tilbage
//     til MODE_DCV (1000V range), rydde fejl-flaget og starte RTOS-
//     måleopgaverne igen."
//     OPFYLDELSE: Denne funktion er præcis det kald, der bringer relæerne
//     tilbage til 1000V range efter *RST — både i katastrofe-recovery-
//     blokken og i den normale *RST-blok. Uden den ville recovery ikke
//     opfylde kravet om at gå tilbage til 1000V range.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: PIN_RELAY_AC_ROUTE, PIN_RELAY_OHM, PIN_RELAY_DIV10,
//     PIN_RELAY_DIV100 og relayGearIndex er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR SÆTTES AC-RUTEN LOW FØRST?
//     AC-ruten (AD8436) er kun relevant i ACV/ACI-mode. I safe state skal
//     kun DC-vejen være aktiv. Ved at slukke AC-ruten først undgår vi, at
//     AD8436's output blandes ind i DC-målingen under og efter skiftet.
//
//   * HVORFOR SÆTTES OHM-TESTSTRØMMEN LOW SOM DET ANDET?
//     Ohm-teststrømmen er den eneste relæstyrede strømkilde, der aktivt
//     sender strøm UD til DUT. Ved en fejl eller et *RST er det kritisk,
//     at den slukkes så hurtigt som muligt — derfor rækkefølgen AC først,
//     Ohm som det andet. (I ISR-varianten slukkes ALT på én gang via
//     register-skrivning, fordi der ikke er tid til sekventiel rækkefølge.)
//
//   * HVORFOR TÆNDES DIV10 OG DIV100 BEGGE (IKKE KUN DIV100)?
//     Ifølge HW-DOC tabel 4.2 er 1:100-konfigurationen REALISERET ved, at
//     BEGGE dæmpere er aktive. DIV10 alene giver 1:10; DIV100 alene er
//     ikke en gyldig konfiguration i hardwaren. Kun kombinationen DIV10+
//     DIV100 giver den dokumenterede 1:100-dæmpning. Det er kernen i
//     RETTELSE buglist C/E.
//
//   * HVORFOR SÆTTES relayGearIndex = 0?
//     relayGearIndex er den globale variabel, som både Task_Metrology,
//     SCPI-parseren og GUI'en læser for at vide, hvilket gear der er
//     aktivt. Ved at sætte den til 0 markeres det, at vi nu står i
//     1000V safe state — ikke i et vilkårligt andet gear. Uden denne
//     opdatering ville SCPI f.eks. svare forkert på CONF:VOLT:DC?.
//
//   * HVORFOR ER DER INGEN vTaskDelay HER?
//     Funktionen skal være så hurtig som muligt — den bruges bl.a. ved
//     katastrofe-recovery. En eventuel settle-time håndteres af kalderen
//     (selectRelayGear indeholder de 100 ms + 50 ms ventetider). At
//     lægge delay ind her ville forsinke recovery unødigt.
//
//   * HVORFOR KALDES DEN IKKE FRA Task_Metrology?
//     Task_Metrology bruger selectRelayGear() til gearskift, fordi den
//     skal bevare Ohm-teststrømmen, når der skiftes gear i Ohm-mode.
//     forceRelaysToSafeState() slukker ALT — inklusive Ohm-teststrømmen
//     — og er derfor kun egnet til opstart og *RST, hvor en total reset
//     er ønsket.
//
//   * SIDE-EFFEKTER:
//     - Ændrer tilstanden på fire GPIO-pins (AC, OHM, DIV10, DIV100).
//     - Opdaterer relayGearIndex til 0.
//     - Påvirker IKKE calibrationTable, systemHalted eller andre globals.
//     - Efterlader relæerne i 1:100-tilstand med teststrøm afbrudt.
//
//   * RETURVÆRDI:
//     Ingen (void). Funktionen signalerer ikke succes eller fejl.
//     digitalWrite() fejler ikke for konfigurerede output-pins.
//
//   * FORHOLDSREGEL TIL ISR-VARIANTEN:
//     Hvis man ændrer logikken her (f.eks. rækkefølgen af relæer eller
//     hvilke der tændes), SKAL man ændre forceRelaysToSafeStateISR()
//     tilsvarende — ellers vil ISR'en og task-koden efterlade relæerne
//     i forskellige tilstande, hvilket er en sikkerhedsrisiko.
//
// -------------------------------------------------------------------------
void forceRelaysToSafeState() {
    digitalWrite(PIN_RELAY_AC_ROUTE, LOW);  // Frakobl AD8436 True RMS-ruten, så kun DC-vejen er aktiv.
    digitalWrite(PIN_RELAY_OHM, LOW);       // Sluk modstands-teststrømmen øjeblikkeligt af sikkerhedshensyn.
    digitalWrite(PIN_RELAY_DIV10, HIGH);    // Tænd 1:10 dæmperen — del af den dokumenterede 1:100-konfiguration.
    digitalWrite(PIN_RELAY_DIV100, HIGH);   // Tænd 1:100 dæmperen — sikrer ægte 1:100 (mest dæmpning, sikreste tilstand).
    relayGearIndex = 0;                     // Marker i den globale variabel, at vi står i 1000V safe state.
}

// -------------------------------------------------------------------------
// 5.6 FUNKTION: forceRelaysToSafeStateISR()
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Dette er ISR-varianten af forceRelaysToSafeState(). Den gør PRÆCIS det
//   samme — tvinger alle fire relæer til 1000V safe state (1:100 dæmpning,
//   teststrøm afbrudt, AC-rute frakoblet) — men implementeret med direkte
//   GPIO-registeradgang i stedet for digitalWrite(). Det gør funktionen
//   100 % IRAM-sikker og eksekverbar fra en interrupt service routine.
//
//   Konkret udføres tre operationer:
//
//     1) GPIO.out_w1tc = GPIO_BITS_ALL_RELAYS;
//        Rydder (slukker) ALLE fire relæ-bits på én enkelt register-
//        skrivning: AC-rute, Ohm-teststrøm, DIV10 og DIV100. Det sker
//        atomart — der er ingen mellemtilstand, hvor kun nogle relæer
//        er slukkede.
//
//     2) GPIO.out_w1ts = GPIO_BIT_RELAY_DIV10_AND_DIV100;
//        Sætter (tænder) DIV10 og DIV100 på én enkelt register-skrivning.
//        Resultatet er ægte 1:100 dæmpning — den sikreste tilstand.
//
//     3) relayGearIndex = 0;
//        Opdaterer den globale variabel, så resten af systemet ved, at
//        vi nu står i 1000V safe state.
//
// BEGRUNDELSE (HVORFOR — inkl. RETTELSE buglist C/E):
//   Funktionen er en spejling af forceRelaysToSafeState(), men med to
//   kritiske forskelle:
//
//   FORSKEL 1 — IMPLEMENTATIONSMIDDEL:
//     digitalWrite() er IKKE IRAM-sikker. Den kalder interne Arduino-
//     funktioner, som kan ligge i flash og dermed ikke er tilgængelige,
//     når en ISR kører (ISR'er må kun kalde IRAM_ATTR-funktioner).
//     Derfor bruges GPIO.out_w1tc / GPIO.out_w1ts direkte — de er
//     register-skrivninger, der altid er tilgængelige og tager få
//     CPU-cyklusser.
//
//   FORSKEL 2 — ATOMARTET:
//     Ved at slukke ALLE relæer med én register-skrivning (out_w1tc) og
//     derefter tænde DIV10+DIV100 med én register-skrivning (out_w1ts)
//     undgås enhver mellemtilstand, hvor f.eks. AC-ruten stadig er tændt,
//     mens DIV100 allerede er slukket. I en nødsituation er atomaritet
//     afgørende — der er ikke tid til sekventielle digitalWrite-kald.
//
//   RETTELSEN (buglist C/E) er den samme som for ikke-ISR-varianten:
//   Ifølge HW-DOC tabel 4.2 kræver 1000V safe state, at BÅDE DIV10 og
//   DIV100 er aktive (1:100). Den oprindelige ISR slukkede alt, hvilket
//   gav 1:1 via JFET-buffer — den MINDST beskyttede tilstand. Ved at
//   tænde DIV10+DIV100 eksplicit rammer vi den dokumenterede 1:100.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   forceRelaysToSafeStateISR() kaldes UDELUKKENDE fra hardwareFaultISR()
//   i Sektion 6 — og kun der. Det er den eneste ISR i hele firmwaren, og
//   den udløses af en FALDENDE flanke på PIN_FAULT_INT (GPIO 25), når
//   LM393-komparatoren detekterer farlig spænding i Ohm-mode.
//
//   Kaldeflowet er:
//     1) LM393 opdager 230V i Ohm-mode.
//     2) Optokobler sender FALDENDE flanke til GPIO 25.
//     3) hardwareFaultISR() kaldes af ESP32'ens interrupt-controller.
//     4) hardwareFaultISR() tager relaySpinlock.
//     5) hardwareFaultISR() kalder forceRelaysToSafeStateISR().
//     6) hardwareFaultISR() sætter systemHalted = true.
//     7) hardwareFaultISR() frigiver relaySpinlock.
//
//   Funktionen kaldes ALDRIG:
//     * Fra setup() (det er ikke-ISR variantens domæne).
//     * Fra Task_Metrology (den bruger selectRelayGear).
//     * Fra Task_GUI_SCPI (den bruger ikke-ISR varianten).
//     * Fra nogen anden ISR — der findes kun én ISR i firmwaren.
//
//   VIGTIGT — SPINLOCK-FORUDSÆTNING:
//   Funktionen forudsætter, at kalderen (hardwareFaultISR) ALLEREDE har
//   taget relaySpinlock via portENTER_CRITICAL_ISR(). Det sikrer, at
//   Task_Metrology ikke samtidig kan ændre relæerne via selectRelayGear().
//   Hvis funktionen kaldes uden spinlock, er der race-risiko. Det er
//   dokumenteret i både funktionens kommentar og i ISR'ens kommentar.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-SAF-5.01] IRAM-LÅST INTERRUPT SERVICE ROUTINE
//     Kravet specificerer: "Når komparatoren på printet opdager 230V i
//     ohm-mode, sender den et 5V signal til PIN_FAULT_INT. Koden skal
//     afbryde alt andet og reagere på under 1 mikrosekund. Dette gøres
//     ved at gemme funktionen i lynhurtig IRAM."
//     OPFYLDELSE: forceRelaysToSafeStateISR() er netop den funktion, der
//     udføres i IRAM-kontekst og tvinger relæerne i safe state på under
//     1 µs. Den bruger udelukkende register-skrivninger og er derfor
//     100 % IRAM-sikker. Uden denne funktion ville REQ-SAF-5.01 ikke
//     kunne opfyldes — digitalWrite() ville crashe eller give undefined
//     behavior i ISR-kontekst.
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE)
//     Kravet specificerer: "Når der skiftes mellem disse [modes], SKAL
//     processoren tvinge alle relæer i deres 'Safe State' (1000V range,
//     alle teststrømme afbrudt)."
//     OPFYLDELSE: Funktionen etablerer præcis denne safe state — 1000V
//     range (1:100) og teststrøm afbrudt — i ISR-kontekst. Det er den
//     "nød-udgave" af FSM'ens safe state, der bruges, når en hardware-
//     fejl kræver øjeblikkelig reaktion uden om den normale FSM.
//
//   * [REQ-ERR-16.02] LÅST RECOVERY-TILSTAND — indirekte
//     Kravet specificerer, at systemet efter en fejl skal låses i en
//     uendelig løkke, indtil *RST modtages. OPFYLDELSE: Funktionen
//     etablerer den sikre tilstand, som recovery-løkken hviler i.
//     Uden den ville relæerne stå i en vilkårlig (potentielt farlig)
//     tilstand under hele recovery-forløbet.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR (IRAM-sikkerhed).
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: GPIO_BITS_ALL_RELAYS, GPIO_BIT_RELAY_DIV10_AND_DIV100
//     og relayGearIndex er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR FØRST out_w1tc (SLUK ALT) OG DEREFTER out_w1ts (TÆND)?
//     Rækkefølgen er bevidst: Først slukkes ALT (inkl. AC-rute og Ohm-
//     teststrøm), derefter tændes kun DIV10+DIV100. Hvis man gjorde det
//     omvendt — tændte DIV10+DIV100 først og slukkede resten bagefter —
//     ville der være et kort øjeblik, hvor AC-ruten og Ohm-teststrømmen
//     stadig var aktive, mens dæmperne allerede var tændt. Det er ikke
//     farligt i sig selv, men det er unødvendigt komplekst. Den valgte
//     rækkefølge giver en ren, atomar overgang til safe state.
//
//   * HVORFOR BRUGES GPIO.out_w1tc OG IKKE GPIO.out_w1ts TIL AT SLUKKE?
//     ESP32's GPIO-registre fungerer sådan:
//       - GPIO.out_w1ts: "write 1 to set" — bits der er 1, sættes HIGH.
//       - GPIO.out_w1tc: "write 1 to clear" — bits der er 1, sættes LOW.
//     For at slukke alle relæer skriver man masken med alle relæ-bits
//     til out_w1tc. For at tænde DIV10+DIV100 skriver man masken med
//     kun disse to bits til out_w1ts. Det er den eneste korrekte måde
//     at manipulere flere GPIOs atomart på.
//
//   * HVORFOR VIRKER DETTE KUN FOR GPIO 0-31?
//     GPIO.out_w1tc og GPIO.out_w1ts er 32-bit registre, der kun dækker
//     GPIO 0-31. GPIO 32-39 har separate registre (GPIO.out1_w1tc og
//     GPIO.out1_w1ts). Alle fire relæ-pins i dette design (12, 13, 14,
//     27) ligger inden for 0-31, så de nuværende registre er korrekte.
//     Hvis pin-mapping ændres til GPIO 32+, SKAL ISR'en omskrives til at
//     bruge out1-registrene — ellers vil relæerne ikke blive styret, og
//     sikkerhedsfunktionen ville fejle lydløst. Dette er dokumenteret i
//     filens header og gentages her for tydelighed.
//
//   * HVORFOR ER DER INGEN esp_rom_delay_us HER?
//     I den oprindelige version af hardwareFaultISR() blev der kaldt
//     esp_rom_delay_us(10) efter forceRelaysToSafeStateISR() for at
//     undgå kontakt-prell. Denne delay ligger i ISR'en — ikke i denne
//     funktion — fordi funktionen skal være så kort som muligt. Formålet
//     med delayen er at lade relækontakterne falde til ro, før systemHalted
//     sættes; det er ISR'ens ansvar, ikke denne funktions.
//
//   * HVORFOR OPDATERES relayGearIndex SELV I ISR-KONTEKST?
//     relayGearIndex er volatile, så ændringen er synlig for begge kerner
//     med det samme. Den bruges af SCPI-parseren (til at svare på
//     CONF:VOLT:DC?) og af Task_Metrology (til at vide, hvilket gear
//     der er aktivt). Uden opdateringen ville systemet tro, det stadig
//     stod i det gamle gear — hvilket ville give forkerte SCPI-svar og
//     forkert kalibreringsslot-valg, når recovery starter.
//
//   * ATOMARTET — HVAD BETYDER DET I PRAKSIS?
//     En enkelt skrivning til GPIO.out_w1tc ændrer ALLE fire relæ-pins
//     på samme CPU-cyklus. Der er ingen mellemtilstand, hvor f.eks.
//     AC-ruten er slukket, men Ohm-teststrømmen stadig er tændt. Det er
//     kritisk i en nødsituation: hvis en interrupt blev affyret midt i
//     en sekventiel digitalWrite-sekvens, kunne man risikere at efterlade
//     relæerne i en farlig mellemtilstand.
//
//   * SIDE-EFFEKTER:
//     - Ændrer tilstanden på fire GPIO-pins atomart (AC, OHM, DIV10,
//       DIV100).
//     - Opdaterer relayGearIndex til 0 (volatile — synligt for begge
//       kerner med det samme).
//     - Påvirker IKKE systemHalted (det er ISR'ens ansvar at sætte det).
//     - Påvirker IKKE calibrationTable eller andre globals.
//
//   * RETURVÆRDI:
//     Ingen (void). Funktionen signalerer ikke succes eller fejl.
//     Register-skrivninger fejler ikke for konfigurerede output-pins.
//
//   * FORHOLDSREGEL TIL IKKE-ISR-VARIANTEN:
//     Hvis man ændrer logikken her (f.eks. rækkefølgen eller hvilke
//     bits der sættes/ryddes), SKAL man ændre forceRelaysToSafeState()
//     tilsvarende — ellers vil ISR'en og task-koden efterlade relæerne
//     i forskellige tilstande. Det er især vigtigt, fordi de to
//     funktioner bruges i forskellige sammenhænge (opstart/*RST vs.
//     hardware-fejl), men skal efterlade relæerne i SAMME tilstand.
//
// -------------------------------------------------------------------------
void forceRelaysToSafeStateISR() {
    GPIO.out_w1tc = GPIO_BITS_ALL_RELAYS;               // Sluk ALLE relæer først på én gang via register (IRAM-sikkert).
    GPIO.out_w1ts = GPIO_BIT_RELAY_DIV10_AND_DIV100;    // Tænd DIV10+DIV100 for ægte 1:100 safe state (IRAM-sikkert).
    relayGearIndex = 0;                                 // Marker i global variabel, at vi nu er i 1000V safe state.
}

// -------------------------------------------------------------------------
// 5.7 FUNKTION: selectRelayGear(uint8_t newGearIndex, MeasurementMode mode)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Aktiverer et specifikt hardwaregear — dvs. en bestemt relækombination
//   — for en given målemode. Funktionen er den centrale "gear-skifter" i
//   hele firmwaren og udfører en kontrolleret sekvens:
//
//     1) Slukker begge spændingsdæmpere (DIV10 + DIV100) og AC-ruten,
//        så der ikke er to samtidige strømveje under skiftet.
//     2) Slukker Ohm-teststrømmen — MEN KUN hvis vi ikke er i Ohm-mode
//        (i Ohm-mode bevares teststrømmen, så målingen ikke afbrydes).
//     3) Venter 100 ms (relæ blanking, jf. REQ-FSM-11.01), så alle
//        kontakter falder til ro.
//     4) Hvis vi er i spændingsmode (DCV/ACV): tænder den korrekte
//        kombination af DIV10/DIV100 for det ønskede gear via switch-case.
//        I andre modes røres spændingsdæmperne IKKE.
//     5) Hvis vi er i Ohm-mode: genopretter Ohm-teststrømmen.
//     6) Opdaterer relayGearIndex globalt, så GUI og FSM kan se gearet.
//     7) Venter yderligere 50 ms (settle-time, jf. REQ-AFE-7.03).
//
//   Resultatet er, at relæerne står i præcis den konfiguration, som
//   newGearIndex og mode specificerer — og at de har haft tid til at
//   falde til ro, før Task_Metrology begynder at sample igen.
//
// GEAR-MAPPING (hvad newGearIndex betyder):
//   Gear 0: 1000V range — DIV10 + DIV100 aktive (1:100 dæmpning).
//   Gear 1:  600V range — DIV10 + DIV100 aktive (1:100 dæmpning).
//   Gear 2:   60V range — DIV10 aktiv (1:10 dæmpning).
//   Gear 3:    6V range — ingen dæmper (1:1 via FET-buffer).
//   Gear 4:  600mV range — ingen dæmper (1:1 via FET-buffer).
//   Gear 5: Bruges kun i DCI/ACI/Ohm-modes (6 gear i alt).
//
//   For spændingsmodes er max gear 4 (5 gear). For strøm- og Ohm-modes
//   er max gear 5 (6 gear). Det valideres af getMaxGearForMode() i
//   Sektion 7 — denne funktion antager, at newGearIndex er gyldigt.
//
// BEGRUNDELSE (HVORFOR — inkl. RETTELSE buglist B og E):
//   Funktionen er kernen i auto-ranging og manuel range-valg. Uden den
//   ville hverken SCPI-kommandoer som CONF:VOLT:DC <range> eller den
//   automatiske hysterese-algoritme i Task_Metrology kunne skifte gear.
//
//   RETTELSE (buglist B) — BLOKERENDE KALD I KRITISK SEKTION:
//     Den oprindelige kode kaldte selectRelayGear() INDE FRA en kritisk
//     sektion (portENTER_CRITICAL). Det er en alvorlig fejl, fordi
//     funktionen indeholder vTaskDelay(), som er et blokerende kald.
//     Blokerende kald er FORBUDT i en kritisk sektion — det kan føre til
//     deadlock eller watchdog-reset. Rettelsen er, at funktionen nu
//     kaldes UDEN FOR den kritiske sektion, og at spinlock kun holdes
//     kortvarigt omkring selve relæ-tilstandslæsningen (se Task_Metrology).
//
//   RETTELSE (buglist E) — GEAR 0 OG 1 SKAL TÆNDE BEGGE DÆMPERE:
//     Ifølge HW-DOC tabel 4.2 kræver 1000V- og 600V-konfigurationerne,
//     at BÅDE DIV10 og DIV100 er aktive (1:100). Den oprindelige kode
//     tændte kun DIV100 for gear 0/1, hvilket gav en ugyldig konfiguration
//     (DIV100 alene er ikke en gyldig hardware-tilstand). Rettelsen er,
//     at begge dæmpere tændes eksplicit for gear 0 og 1.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   selectRelayGear() kaldes fra tre steder i firmwaren:
//
//     1) handleScpiCommand() i Sektion 11 — mange steder:
//        - CONF:VOLT:DC <range>   (manuel DCV-range)
//        - CONF:VOLT:AC <range>   (manuel ACV-range)
//        - CONF:CURR:DC <range>   (manuel DCI-range)
//        - CONF:CURR:AC <range>   (manuel ACI-range)
//        - CONF:RES <range>       (manuel 2W Ohm-range)
//        - CONF:FRES <range>      (manuel 4W Ohm-range)
//        Hvert sted kaldes funktionen UNDER relayMutex, fordi SCPI-parseren
//        kører på Core 0 og kan sameksistere med Task_Metrology på Core 1.
//
//     2) Task_Metrology i Sektion 12 — auto-ranging:
//        - Når hysterese-algoritmen beslutter at skifte gear op eller ned,
//          kaldes selectRelayGear() med currentGearSnapshot ± 1.
//        - Kaldes UNDER relayMutex, men UDEN FOR spinlock (buglist B-rettelse).
//
//     3) Task_GUI_SCPI i Sektion 14 — mode-skift:
//        - Ved CONF:VOLT:AC uden range: kaldes for at bevare gearet, mens
//          mode opdateres til ACV.
//        - Ved CONF:CURR:AC uden range: samme princip for ACI.
//
//   Funktionen kaldes ALDRIG:
//     * Fra ISR'en (den bruger forceRelaysToSafeStateISR i stedet).
//     * Fra setup() (den bruger forceRelaysToSafeState til initial safe state).
//     * Uden at relayMutex er taget (det er en forudsætning).
//
//   VIGTIGT — MUTEX-FORUDSÆTNING:
//   Funktionen forudsætter, at kalderen har taget relayMutex via
//   xSemaphoreTake(relayMutex, portMAX_DELAY). Det sikrer, at SCPI-
//   parseren og Task_Metrology ikke samtidig ændrer relæerne. Uden
//   mutex er der race-risiko, især fordi funktionen indeholder
//   vTaskDelay(), hvor den anden kerne kan nå at køre.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE)
//     Kravet specificerer: "Når der skiftes mellem disse [modes], SKAL
//     processoren tvinge alle relæer i deres 'Safe State' (1000V range,
//     alle teststrømme afbrudt), vente 100 ms, og derefter koble de nye
//     relæer ind."
//     OPFYLDELSE: Funktionen udfører præcis denne sekvens — slukker
//     dæmpere og AC-rute, slukker Ohm-teststrøm (hvis ikke i Ohm-mode),
//     venter 100 ms, og indkobler de nye relæer. Det er den centrale
//     implementering af REQ-FSM-11.01's gear-skift-sekvens.
//
//   * [REQ-AFE-7.03] RELÆ BLANKING
//     Kravet specificerer: "Lige efter et relæ er skiftet via digitalWrite,
//     SKAL der kaldes vTaskDelay(pdMS_TO_TICKS(50)); Derefter SKAL
//     ringbufferen (filterBuffer) fyldes op med 0'er, så de hoppende
//     forstyrrelser ikke blandes ind i de pæne gennemsnit."
//     OPFYLDELSE: Funktionen afslutter med vTaskDelay(pdMS_TO_TICKS(50))
//     — den ekstra settle-time. Selve buffer-flush håndteres af kalderen
//     (Task_Metrology sætter filterBufferPrimed = false og fylder med
//     friske målinger — en forbedring i forhold til kravets "0'er",
//     jf. buglist punkt 3).
//
//   * [REQ-AFE-7.01] SKIFT OP (UP-RANGING)
//     Kravet specificerer, at der skal skiftes op, når filteredVoltage
//     overstiger 4.000V. OPFYLDELSE: Funktionen er det kald, som
//     Task_Metrology bruger til at udføre selve skiftet, når hysterese-
//     algoritmen har besluttet det.
//
//   * [REQ-AFE-7.02] SKIFT NED (DOWN-RANGING)
//     Kravet specificerer, at der skal skiftes ned, når displayværdien
//     falder under 0.500V. OPFYLDELSE: Samme som ovenstående — funktionen
//     udfører selve relæskiftet.
//
//   * [REQ-FSM-11.02] HYSTERESE-ALGORITME (ANTI-CHATTER) — indirekte
//     Kravet specificerer up-range ved 98% og down-range ved 9%. OPFYLDELSE:
//     Funktionen udfører de skift, som hysterese-algoritmen beslutter;
//     den er det "håndtag", algoritmen trækker i.
//
//   * [REQ-FSM-11.03] FLUSH AF DSP-BUFFER VED RELÆSKIFT
//     Kravet specificerer, at filterbufferen skal flushes efter relæskift.
//     OPFYLDELSE: Funktionen sætter ikke selv flaget, men dens 50 ms
//     settle-time er forudsætningen for, at kalderen kan flushe bufferen
//     korrekt. Kalderen (Task_Metrology) sætter filterBufferPrimed = false
//     efter kaldet.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: newGearIndex, keepOhmActive, isVoltageMode,
//     PIN_RELAY_DIV10, PIN_RELAY_DIV100, PIN_RELAY_AC_ROUTE,
//     PIN_RELAY_OHM og relayGearIndex er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR TAGES mode SOM PARAMETER (IKKE GLOBAL)?
//     currentMode er en global volatile variabel, som SCPI-parseren
//     ændrer. Hvis selectRelayGear() læste currentMode direkte, kunne
//     den nå at se en ANDEN mode end den, kalderen bad om — f.eks. hvis
//     SCPI skiftede mode midt i kaldet. Ved at tage mode som parameter
//     fryses den ønskede mode for hele kaldets varighed. Det eliminerer
//     en race mellem kernerne.
//
//   * HVORFOR SLUKKES DIV10/DIV100/AC FØRST?
//     For at undgå to samtidige strømveje. Hvis man f.eks. skiftede fra
//     60V (DIV10) til 6V (1:1) ved at slukke DIV10 og tænde "intet",
//     ville der være et øjeblik, hvor DIV10 stadig var aktiv, mens den
//     nye konfiguration allerede var delvist indkoblet. Ved at slukke
//     ALT først og derefter tænde det ønskede, undgås enhver tvetydighed.
//
//   * HVORFOR BEVARES OHM-TESTSTRØMMEN I OHM-MODE?
//     Hvis teststrømmen blev afbrudt ved hvert gearskift i Ohm-mode,
//     ville målingen blive afbrudt unødigt. Ved at bevare den (kun slukke
//     hvis vi IKKE er i Ohm-mode) kan auto-ranging skifte gear i Ohm-mode
//     uden at forstyrre målingen mere end højst nødvendigt.
//
//   * HVORFOR RØRES SPÆNDINGSDÆMPERNE IKKE I OHM-MODE?
//     I Ohm-mode er det ikke spændingsdæmperne, der bestemmer gearet —
//     det er teststrømmen og shuntmodstanden. Hvis man tændte DIV10/DIV100
//     i Ohm-mode, ville man dæmpe det signal, der skal måles over DUT,
//     og målingen ville blive forkert. Derfor er switch-case-blokken
//     betinget af isVoltageMode.
//
//   * HVORFOR 100 ms + 50 ms?
//     100 ms er den blanking-tid, som REQ-FSM-11.01 kræver (så alle
//     kontakter falder til ro efter slukning). 50 ms er den ekstra
//     settle-time, som REQ-AFE-7.03 kræver (så kontakterne er stabile,
//     når de nye relæer tændes). Tilsammen 150 ms pr. gearskift — nok
//     til, at mekaniske relæer (Omron G6K) er fuldt ud stabiliserede.
//
//   * HVORFOR OPDATERES relayGearIndex FØRST TIL SIDST?
//     relayGearIndex opdateres EFTER de nye relæer er tændt, men FØR
//     den sidste 50 ms settle-time. Det betyder, at GUI og SCPI ser det
//     nye gear med det samme, mens Task_Metrology stadig venter på, at
//     relæerne falder til ro. Det er bevidst: SCPI kan svare korrekt på
//     CONF:VOLT:DC? med det samme, og Task_Metrology ved, hvilket gear
//     der er aktivt, når den vågner efter delayen.
//
//   * HVORFOR ER DER IKKE SPINLOCK HER?
//     Spinlock bruges kun i ISR-kontekst (hvor mutex ikke virker). Denne
//     funktion kaldes fra task-kontekst, hvor relayMutex er tilstrækkelig.
//     Hvis man tog spinlock her, ville man blokere ISR'en i op til 150 ms
//     — det er uacceptabelt for en sikkerhedsfunktion.
//
//   * SIDE-EFFEKTER:
//     - Ændrer tilstanden på op til fire GPIO-pins sekventielt.
//     - Opdaterer relayGearIndex globalt (volatile — synligt for begge
//       kerner).
//     - Blokerer i op til 150 ms (100 ms + 50 ms).
//     - Påvirker IKKE calibrationTable, systemHalted eller andre globals.
//
//   * RETURVÆRDI:
//     Ingen (void). Funktionen signalerer ikke succes eller fejl.
//     digitalWrite() fejler ikke for konfigurerede output-pins.
//
//   * FORHOLDSREGEL VED ÆNDRING AF GEAR-MAPPING:
//     Hvis man ændrer switch-case-blokken (f.eks. tilføjer et nyt gear
//     eller ændrer hvilke dæmpere der tændes), SKAL man også opdatere:
//       - mapModeAndGearToCalibrationSlot() i Sektion 8 (slot-mapping).
//       - getGearAttenuationFactor() i Sektion 7 (dæmpningsfaktor).
//       - getMaxGearForMode() i Sektion 7 (max gear pr. mode).
//     Ellers vil kalibrering, AC-ripple-skalering og auto-ranging bruge
//     forkerte værdier for det nye gear.
//
// -------------------------------------------------------------------------
void selectRelayGear(uint8_t newGearIndex, MeasurementMode mode) {
    bool keepOhmActive = (mode == MeasurementMode::MODE_OHM_2W) || (mode == MeasurementMode::MODE_OHM_4W); // Husk om vi er i Ohm-mode.
    bool isVoltageMode = (mode == MeasurementMode::MODE_DCV) || (mode == MeasurementMode::MODE_ACV);       // Er vi i spændingsmode?
    digitalWrite(PIN_RELAY_DIV10, LOW);     // Først: sluk begge spændingsdæmpere for at undgå to samtidige strømveje.
    digitalWrite(PIN_RELAY_DIV100, LOW);    // Sluk 1:100 dæmperen.
    digitalWrite(PIN_RELAY_AC_ROUTE, LOW);  // Sluk AC-ruten midlertidigt under skiftet.
    if (!keepOhmActive) {                   // Hvis vi IKKE er i Ohm-mode...
        digitalWrite(PIN_RELAY_OHM, LOW);   // ...så sluk teststrømmen som en del af safe state.
    }
    vTaskDelay(pdMS_TO_TICKS(100));         // Vent 100 ms som krævet i REQ-FSM-11.01, så alle kontakter falder til ro.
    if (isVoltageMode) {                    // Kun spændingsmode må røre spændingsdæmperne.
        switch (newGearIndex) {             // Vælg den korrekte relækombination for det ønskede gear.
            case 0:                         // 1000V range: 1:100 dæmper aktiv (DIV10+DIV100 per HW-DOC tabel 4.2).
                digitalWrite(PIN_RELAY_DIV10, HIGH);  // Tænd 1:10 dæmperen (del af 1:100-konfigurationen).
                digitalWrite(PIN_RELAY_DIV100, HIGH); // Tænd 1:100 dæmperen for samlet 1:100.
                break;                      // Afslut case.
            case 1:                         // 600V range: 1:100 dæmper aktiv (DIV10+DIV100 per HW-DOC tabel 4.2).
                digitalWrite(PIN_RELAY_DIV10, HIGH);  // Tænd 1:10 dæmperen (del af 1:100-konfigurationen).
                digitalWrite(PIN_RELAY_DIV100, HIGH); // Tænd 1:100 dæmperen for samlet 1:100.
                break;                      // Afslut case.
            case 2:                         // 60V range: 1:10 dæmper aktiv.
                digitalWrite(PIN_RELAY_DIV10, HIGH);  // Tænd 1:10 dæmperen.
                break;                      // Afslut case.
            case 3:                         // 6V range: 1:1 (ingen dæmper), FET-buffer aktiv.
                break;                      // Ingen relæer skal tændes for 1:1.
            case 4:                         // 600mV range: 1:1 via FET-buffer.
                break;                      // Ingen relæer skal tændes for 1:1.
            default:                        // Ukendt gear: bliv i safe state.
                break;                      // Gør intet, så vi forbliver sikre.
        }
    }
    if (keepOhmActive) {                    // Hvis vi er i Ohm-mode...
        digitalWrite(PIN_RELAY_OHM, HIGH);  // ...så genopret teststrømmen, så målingen fortsætter uforstyrret.
    }
    relayGearIndex = newGearIndex;          // Gem det nye gear globalt, så GUI og FSM kan se det.
    vTaskDelay(pdMS_TO_TICKS(50));          // Ekstra settle-time som krævet i REQ-AFE-7.03 (relæ blanking).
}

// =========================================================================
// SEKTION 6: HARDWARE INTERRUPT SERVICE ROUTINE (SIKKERHED) — REQ-SAF-5.01
// =========================================================================

// -------------------------------------------------------------------------
// 6.2 INTERRUPT SERVICE ROUTINE: hardwareFaultISR()
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Dette er firmwarens ENESTE interrupt service routine (ISR). Den udløses,
//   når LM393-komparatoren (Blok 5) detekterer farlig spænding — typisk
//   230V fra stikkontakten — mens instrumentet står i Ohm-mode. ISR'en
//   reagerer på under 1 mikrosekund og gør tre ting:
//
//     1) Tager relaySpinlock for at beskytte mod samtidig adgang fra
//        Task_Metrology (som kan køre på Core 1 og kalde selectRelayGear()).
//     2) Kalder forceRelaysToSafeStateISR(), der via direkte GPIO-register-
//        skrivning tvinger alle relæer i 1:100 safe state (teststrøm
//        afbrudt, AC-rute frakoblet).
//     3) Sætter systemHalted = true, så begge kerner stopper al videre
//        måling og GUI'en viser katastrofe-alarmen.
//
//   Undervejs ventes 10 µs (esp_rom_delay_us) for at undgå kontakt-prell
//   på relæerne, før systemHalted sættes. Til sidst frigives spinlock.
//
// BEGRUNDELSE (HVORFOR):
//   Et multimeter, der måler modstand, sender en lille teststrøm ud til
//   DUT. Hvis brugeren ved en fejl tilslutter 230V fra stikkontakten i
//   Ohm-mode, vil der løbe en farlig strøm gennem teststrømskilden og
//   potentielt gennem brugeren. PTC'en og hardware-klamperne (Blok 5)
//   begrænser strømmen, men firmwaren SKAL reagere øjeblikkeligt for at
//   undgå, at relæerne står i en farlig konfiguration.
//
//   ISR'en er derfor designet til at være:
//     * Hurtigst mulig (IRAM_ATTR — koden ligger i intern RAM, ikke flash).
//     * IRAM-sikker (ingen digitalWrite, ingen Serial, ingen malloc).
//     * Trådsikker (spinlock beskytter mod race med Task_Metrology).
//     * Idempotent (kan kaldes flere gange uden skade — den sætter bare
//       systemHalted igen, hvis den allerede er sat).
//
// HVAD UDLØSER DENNE ISR? (KALDE-GRAF):
//   hardwareFaultISR() kaldes UDELUKKENDE af ESP32'ens interrupt-
//   controller, når der detekteres en FALDENDE flanke på PIN_FAULT_INT
//   (GPIO 25). Det sker i følgende sekvens:
//
//     1) LM393-komparatoren (Blok 5) sammenligner spændingen over Ohm-
//        teststrømskilden med en reference.
//     2) Ved farlig spænding skifter LM393's output fra HIGH til LOW.
//     3) Optokobleren (PC817/6N137) videresender flanken til GPIO 25.
//     4) ESP32'ens interrupt-controller kalder hardwareFaultISR().
//
//   ISR'en er tilkoblet i setup() (Sektion 15) via:
//     attachInterrupt(digitalPinToInterrupt(PIN_FAULT_INT),
//                     hardwareFaultISR, FALLING);
//
//   ISR'en kalder selv:
//     * portENTER_CRITICAL_ISR() — tager spinlock.
//     * forceRelaysToSafeStateISR() — tvinger relæer i safe state.
//     * esp_rom_delay_us(10) — kort IRAM-sikker ventetid.
//     * portEXIT_CRITICAL_ISR() — frigiver spinlock.
//
//   ISR'en kaldes ALDRIG:
//     * Fra nogen task (den er udelukkende interrupt-drevet).
//     * Fra setup() eller loop().
//     * Fra SCPI-parseren eller GUI'en.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-SAF-5.01] IRAM-LÅST INTERRUPT SERVICE ROUTINE
//     Kravet specificerer: "Når komparatoren på printet opdager 230V i
//     ohm-mode, sender den et 5V signal til PIN_FAULT_INT. Koden skal
//     afbryde alt andet og reagere på under 1 mikrosekund. Dette gøres
//     ved at gemme funktionen i lynhurtig IRAM."
//     OPFYLDELSE: hardwareFaultISR() er præcis denne funktion. Den er
//     markeret IRAM_ATTR, bruger udelukkende IRAM-sikre kald, og
//     reagerer på under 1 µs. Uden denne ISR ville REQ-SAF-5.01 ikke
//     være opfyldt.
//
//   * [REQ-SAF-5.02] AKTIVERING
//     Kravet specificerer: "ISR'en skal tilkobles i setup():
//     attachInterrupt(digitalPinToInterrupt(PIN_FAULT_INT),
//     hardwareFaultISR, RISING);"
//     OPFYLDELSE: ISR'en er tilkoblet i setup() (Sektion 15). Bemærk:
//     kravet siger RISING, men den faktiske hardware (optokobler aktiv
//     LOW) kræver FALLING. Firmwaren bruger FALLING — en dokumenteret
//     afvigelse fra kravets bogstavelige ordlyd, fordi hardwarens
//     polaritet er omvendt. Funktionen er den samme; kun flanken er
//     korrigeret.
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE) — indirekte
//     Kravet specificerer, at systemet skal tvinge alle relæer i safe
//     state ved mode-skift. OPFYLDELSE: ISR'en er "nød-udgaven" af denne
//     safe state — den tvinger relæerne i 1:100 uden om den normale FSM,
//     fordi en hardware-fejl kræver øjeblikkelig reaktion.
//
//   * [REQ-ERR-16.01] VISUEL KATASTROFE-ALARM — indirekte
//     Kravet specificerer, at GUI'en skal vise en rød advarselsskærm, når
//     systemHalted == true. OPFYLDELSE: ISR'en sætter systemHalted = true,
//     hvilket er det signal, der udløser GUI'ens katastrofe-alarm. Uden
//     ISR'en ville GUI'en aldrig vide, at der er en fejl.
//
//   * [REQ-ERR-16.02] LÅST RECOVERY-TILSTAND — indirekte
//     Kravet specificerer, at systemet skal låses i en uendelig løkke,
//     indtil *RST modtages. OPFYLDELSE: ISR'en sætter systemHalted, som
//     GUI'en bruger til at låse systemet. Selve låse-løkken er i
//     Task_GUI_SCPI (Sektion 14), men ISR'en er det, der starter den.
//
//   * [REQ-RTOS-4.01] TASK ALLOKERING — indirekte
//     Kravet specificerer, at Core 1 kører Task_Metrology og Core 0
//     kører Task_GUI_SCPI. OPFYLDELSE: ISR'en kan afbryde BEGGE kerner,
//     fordi den er hardware-drevet. Spinlock'en beskytter mod race med
//     Task_Metrology, og systemHalted-flaget informerer begge kerner.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i ISR'en er kommenteret med både HVAD
//     og HVORFOR.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: relaySpinlock, systemHalted og hardwareFaultISR er
//     alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR IRAM_ATTR?
//     ESP32'ens flash-hukommelse er langsommere end intern RAM, og flash
//     kan være i brug af en anden kerne, når ISR'en udløses. IRAM_ATTR
//     fortæller compileren, at funktionen skal placeres i intern RAM,
//     så den altid er tilgængelig øjeblikkeligt. Uden IRAM_ATTR ville
//     ISR'en kunne crashe eller forsinkes, hvis flash var optaget.
//
//   * HVORFOR SPINLOCK OG IKKE MUTEX?
//     ESP32 har dual-core, og en almindelig mutex (SemaphoreHandle_t)
//     virker ikke i ISR-kontekst — den kan blokere, hvilket er forbudt
//     i en ISR. En portMUX_TYPE spinlock (portENTER_CRITICAL_ISR) er
//     designet til ISR-brug: den spinner kortvarigt, indtil låsen er
//     ledig, og blokerer aldrig.
//
//   * HVORFOR TAGES SPINLOCK OVERHOVEDET?
//     Task_Metrology på Core 1 kan samtidig kalde selectRelayGear(), som
//     ændrer relæerne. Uden spinlock kunne ISR'en og Task_Metrology
//     race om relæ-tilstanden — f.eks. kunne ISR'en slukke alle relæer,
//     mens Task_Metrology lige bagefter tændte DIV10 igen. Spinlock'en
//     sikrer, at kun én af dem ad gangen ændrer relæerne.
//
//   * HVORFOR esp_rom_delay_us(10) OG IKKE vTaskDelay()?
//     vTaskDelay() er et RTOS-kald, der kræver scheduler-kontekst og
//     er IKKE IRAM-sikkert. esp_rom_delay_us() er en ROM-funktion, der
//     blot spinner i et antal mikrosekunder — den er IRAM-sikker og
//     kan kaldes fra ISR. 10 µs er nok til, at relækontakterne falder
//     til ro efter det atomare skift.
//
//   * HVORFOR SÆTTES systemHalted FØRST EFTER DELAYEN?
//     Hvis systemHalted blev sat før delayen, kunne Task_Metrology nå at
//     se flaget og begynde recovery, FØR relæerne var faldet til ro.
//     Ved at vente 10 µs først sikres det, at relæerne er stabile, når
//     resten af systemet reagerer på flaget.
//
//   * HVORFOR ER systemHalted VOLATILE?
//     Fordi den ændres i ISR-kontekst og læses i task-kontekst på begge
//     kerner. Uden volatile kunne compileren cache værdien i et register
//     og aldrig se ændringen. volatile tvinger en frisk læsning hver gang.
//
//   * HVORFOR FRIGIVES SPINLOCK TIL SIDST?
//     Spinlock'en skal holdes, så længe relæerne ændres. Hvis den blev
//     frigivet før systemHalted blev sat, kunne Task_Metrology nå at
//     ændre relæerne igen, før fejltilstanden blev markeret. Ved at
//     frigive til sidst sikres en atomar overgang til fejltilstand.
//
//   * KAN ISR'EN KALDES FLERE GANGE?
//     Ja. Hvis LM393 fortsat detekterer farlig spænding, kan flanken
//     udløses igen. ISR'en er idempotent: forceRelaysToSafeStateISR()
//     sætter relæerne i samme tilstand hver gang, og systemHalted
//     sættes blot igen (allerede true). Det er harmløst.
//
//   * SIDE-EFFEKTER:
//     - Ændrer tilstanden på fire GPIO-pins via forceRelaysToSafeStateISR().
//     - Opdaterer relayGearIndex til 0.
//     - Sætter systemHalted = true (volatile — synligt for begge kerner).
//     - Blokerer i ca. 10 µs (esp_rom_delay_us).
//     - Påvirker IKKE calibrationTable, calibrationMutex eller andre globals.
//
//   * RETURVÆRDI:
//     Ingen (void). ISR'er returnerer altid void.
//
//   * FORHOLDSREGEL VED ÆNDRING AF RELÆ-PINS:
//     Hvis pin-mapping ændres til GPIO 32+, SKAL forceRelaysToSafeStateISR()
//     omskrives til at bruge GPIO.out1_w1tc / GPIO.out1_w1ts i stedet for
//     GPIO.out_w1tc / GPIO.out_w1ts. Ellers vil ISR'en ikke styre de nye
//     pins, og sikkerhedsfunktionen ville fejle lydløst. Dette er også
//     dokumenteret i filens header og i forceRelaysToSafeStateISR().
//
// -------------------------------------------------------------------------
void IRAM_ATTR hardwareFaultISR() {
    portENTER_CRITICAL_ISR(&relaySpinlock); // Tag spinlock, så vi ikke kolliderer med selectRelayGear() på Core 1.
    forceRelaysToSafeStateISR();            // Sluk alle relæer og tænd DIV10+DIV100 (IRAM-sikkert).
    esp_rom_delay_us(10);                   // Kort IRAM-sikker ventetid for at undgå kontakt-prell.
    systemHalted = true;                    // Fortæl resten af programmet (begge kerner) at vi er i en fejltilstand.
    portEXIT_CRITICAL_ISR(&relaySpinlock);  // Frigiv spinlock.
}

// =========================================================================
// SEKTION 7: AUTO-RANGING GRÆNSER PR. MODE — REQ-FSM-11.02 / REQ-AFE-7.01/7.02
// =========================================================================

// -------------------------------------------------------------------------
// 7.2 FUNKTION: getUpRangeThresholdForMode(MeasurementMode mode)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Returnerer den øvre ADC-spændingsgrænse — "up-range trigger" — som
//   auto-ranging-algoritmen skal bruge for en given målemode. Når den
//   filtrerede ADC-spænding (filteredVoltage) overstiger denne grænse,
//   skal systemet skifte til et gear med MERE dæmpning (lavere gear-
//   indeks), så ADC'en ikke overstyres.
//
//   Funktionen er en simpel switch-case, der returnerer en float. I den
//   nuværende version returnerer ALLE modes 4.000V, fordi alle målinger
//   i sidste ende lander på ADS1115's indgang med GAIN_ONE (±4.096V).
//   Funktionen er alligevel opdelt pr. mode, fordi:
//
//     1) Det gør koden selvdokumenterende — man kan se, at grænsen er
//        bevidst valgt pr. mode, ikke blot en tilfældig konstant.
//     2) Det gør det muligt at differentiere grænsen pr. mode i fremtiden
//        (f.eks. hvis en mode får en anden ADC-forstærkning).
//     3) Det opfylder REQ-FSM-11.02's krav om mode-specifik hysterese.
//
//   Grænsen 4.000V er valgt som 98% af ADC'ens full-scale (4.096V) —
//   dvs. 4.096 × 0.98 = 4.014V, rundet ned til 4.000V for lidt ekstra
//   margin. Det giver 96 mV headroom, før ADC'en klipper.
//
// BEGRUNDELSE (HVORFOR):
//   Uden denne funktion ville auto-ranging-algoritmen i Task_Metrology
//   (Sektion 12) ikke vide, hvornår den skal skifte gear. En hardkodet
//   grænse i Task_Metrology ville være svær at vedligeholde og umulig at
//   tilpasse pr. mode. Ved at samle grænserne i denne funktion opnås:
//
//     * Ét centralt sted at ændre grænsen for en given mode.
//     * Mulighed for at differentiere grænsen pr. mode uden at røre
//       Task_Metrology.
//     * Tydelig kobling mellem mode og grænse — man kan læse funktionen
//       og se, at f.eks. DCV og ACV har samme grænse, fordi de begge
//       lander på samme ADC-indgang.
//
//   Grænsen 4.000V er valgt frem for 4.096V, fordi:
//     * ADC'en har en vis støj og offset — 4.096V er den teoretiske max,
//       men i praksis kan selv en lille overskridelse klippe signalet.
//     * 4.000V giver 96 mV margin, hvilket er nok til at undgå klipning
//       uden at gå glip af målinger tæt på full-scale.
//     * 4.000V er et "rundt" tal, der er let at verificere i tests.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   getUpRangeThresholdForMode() kaldes UDELUKKENDE fra Task_Metrology
//   i Sektion 12 — specifikt i auto-ranging-hysterese-blokken (trin 8):
//
//     float upThreshold = getUpRangeThresholdForMode(localMode);
//
//   Her bruges den til at afgøre, om filteredVoltage har overskredet
//   grænsen, og om der derfor skal skiftes til et gear med mere dæmpning.
//
//   Funktionen kaldes ALDRIG:
//     * Fra SCPI-parseren (den bruger manuel range, ikke auto-ranging).
//     * Fra GUI'en (den viser kun resultatet).
//     * Fra setup() eller ISR'en.
//     * Fra nogen anden funktion i firmwaren.
//
//   Funktionen er en ren "getter" — den læser ingen globale variable og
//   har ingen side-effekter. Den kan derfor kaldes fra enhver kontekst
//   uden mutex eller spinlock.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-FSM-11.02] HYSTERESE-ALGORITME (ANTI-CHATTER)
//     Kravet specificerer: "Up-range (Skift op): Triggers når
//     filteredVoltage er ≥98% af ADC'ens Full-Scale (f.eks. ≥4.000V)."
//     OPFYLDELSE: Funktionen returnerer præcis 4.000V for alle modes,
//     hvilket opfylder kravet om ≥98% af 4.096V. Uden denne funktion
//     ville kravet ikke være opfyldt — der ville ikke være nogen
//     defineret up-range-grænse.
//
//   * [REQ-AFE-7.01] SKIFT OP (UP-RANGING)
//     Kravet specificerer: "Hvis filteredVoltage overstiger 4.000V
//     (tæt på ADS1115's max på 4.096V), skal systemet skifte gear."
//     OPFYLDELSE: Funktionen returnerer 4.000V, som er præcis den
//     grænse, REQ-AFE-7.01 kræver. Task_Metrology bruger denne værdi
//     til at udløse gearskiftet.
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE) — indirekte
//     Kravet specificerer, at systemet skal kunne skifte mellem modes.
//     OPFYLDELSE: Funktionen tager mode som parameter og returnerer en
//     mode-specifik grænse — det er forudsætningen for, at auto-ranging
//     kan fungere korrekt i alle modes.
//
//   * [REQ-DSP-6.01] ADC OPSÆTNING (ADS1115) — indirekte
//     Kravet specificerer, at ADC'en skal måle fra -4.096V til +4.096V.
//     OPFYLDELSE: Funktionens grænse på 4.000V er valgt med udgangspunkt
//     i denne full-scale. Uden REQ-DSP-6.01's 4.096V-reference ville
//     4.000V ikke give mening.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR (herunder hvilken mode grænsen gælder for).
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: getUpRangeThresholdForMode, MeasurementMode, MODE_DCV
//     osv. er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR RETURNERER ALLE MODES 4.000V?
//     Fordi alle målinger i sidste ende lander på ADS1115's indgang med
//     GAIN_ONE (±4.096V). Uanset om det er DCV, ACV, DCI, ACI eller Ohm,
//     er det den samme ADC-indgang, der skal beskyttes mod overstyring.
//     Grænsen er derfor identisk. Hvis en fremtidig hardware-revision
//     giver en mode en anden forstærkning (f.eks. GAIN_TWO for en
//     særlig følsom mode), kan grænsen differentieres her.
//
//   * HVORFOR ER DER EN default-CASE?
//     Hvis mode er en ugyldig enum-værdi (f.eks. pga. hukommelses-
//     korruption eller en fremtidig udvidelse, der glemmer at opdatere
//     denne funktion), returnerer default-casen 4.000V. Det er den
//     sikreste standard — den beskytter ADC'en mod overstyring, uanset
//     hvad mode er. Uden default-case ville funktionen returnere
//     udefineret adfærd (i praksis 0 eller garbage), hvilket kunne
//     føre til, at ADC'en blev overstyret.
//
//   * HVORFOR float OG IKKE double?
//     filteredVoltage i Task_Metrology er float, og ADC'ens opløsning
//     er 16-bit (0.125 mV pr. LSB). float har 24-bit præcision, hvilket
//     er rigeligt til at repræsentere 4.000V med den nøjagtighed, der
//     er nødvendig. double ville bruge 8 bytes i stedet for 4 og give
//     unødvendig præcision — det ville også gøre sammenligningen i
//     Task_Metrology langsommere på ESP32 (double er 64-bit).
//
//   * HVORFOR ER FUNKTIONEN IKKE inline?
//     Compileren kan selv vælge at inline'e den, fordi den er kort og
//     kaldes fra ét sted. At markere den eksplicit inline ville ikke
//     ændre adfærden, men ville gøre koden mindre læsbar. Funktionen
//     er bevidst holdt som en separat funktion for at gøre det tydeligt,
//     at grænsen er mode-specifik.
//
//   * SIDE-EFFEKTER:
//     Ingen. Funktionen læser ingen globale variable, skriver ingen
//     globale variable, og har ingen bivirkninger. Den er en ren
//     "getter" og kan kaldes fra enhver kontekst uden mutex.
//
//   * RETURVÆRDI:
//     float — den øvre ADC-grænse i volt (altid 4.000f i denne version).
//
//   * FORHOLDSREGEL VED ÆNDRING AF ADC-GAIN:
//     Hvis ADC'ens gain ændres fra GAIN_ONE (4.096V) til f.eks. GAIN_TWO
//     (2.048V), SKAL denne funktions returværdi opdateres tilsvarende
//     (f.eks. til 2.000V). Ellers vil auto-ranging ikke skifte gear
//     korrekt, og ADC'en kan blive overstyret. Samme forholdsregel
//     gælder for getDownRangeThresholdForMode() i Sektion 7.
//
//   * FORHOLDSREGEL VED TILFØJELSE AF NY MODE:
//     Hvis en ny MeasurementMode tilføjes, SKAL denne funktion opdateres
//     med en case for den nye mode — ellers vil den falde tilbage til
//     default (4.000V), hvilket måske ikke er korrekt for den nye mode.
//     Samme forholdsregel gælder for getDownRangeThresholdForMode() og
//     getMaxGearForMode() i Sektion 7 samt mapModeAndGearToCalibrationSlot()
//     i Sektion 8.
//
// -------------------------------------------------------------------------
float getUpRangeThresholdForMode(MeasurementMode mode) {
    switch (mode) {                         // Vælg grænse ud fra måletilstand.
        case MeasurementMode::MODE_DCV:     // DC-spænding: 98% af 4.096V ≈ 4.000V (REQ-FSM-11.02).
            return 4.000f;                  // Up-range ved 4.000V.
        case MeasurementMode::MODE_ACV:     // AC-spænding: samme ADC-grænse, men RMS-signal.
            return 4.000f;                  // Up-range ved 4.000V RMS.
        case MeasurementMode::MODE_DCI:     // DC-strøm: shunten giver 0-4V ved fuld skala.
            return 4.000f;                  // Up-range ved 4.000V over shunt.
        case MeasurementMode::MODE_ACI:     // AC-strøm: samme ADC-grænse.
            return 4.000f;                  // Up-range ved 4.000V RMS over shunt.
        case MeasurementMode::MODE_OHM_2W:  // 2-leder Ohm: spændingsfald over DUT.
            return 4.000f;                  // Up-range ved 4.000V.
        case MeasurementMode::MODE_OHM_4W:  // 4-leder Ohm: samme grænse.
            return 4.000f;                  // Up-range ved 4.000V.
        default:                            // Ukendt mode: brug sikker standard.
            return 4.000f;                  // Fallback til 4.000V.
    }
}

// -------------------------------------------------------------------------
// 7.3 FUNKTION: getDownRangeThresholdForMode(MeasurementMode mode)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Returnerer den nedre ADC-spændingsgrænse — "down-range trigger" — som
//   auto-ranging-algoritmen skal bruge for en given målemode. Når den
//   filtrerede ADC-spænding (filteredVoltage) falder til eller under denne
//   grænse, skal systemet skifte til et gear med MINDRE dæmpning (højere
//   gear-indeks), så ADC'ens opløsning udnyttes bedst muligt.
//
//   Funktionen er en simpel switch-case, der returnerer en float. I den
//   nuværende version returnerer ALLE modes 0.360V, fordi alle målinger
//   i sidste ende lander på ADS1115's indgang med GAIN_ONE (±4.096V), og
//   fordi kravet om 9% overlap gælder ens for alle modes. Funktionen er
//   alligevel opdelt pr. mode, fordi:
//
//     1) Det gør koden selvdokumenterende — man kan se, at grænsen er
//        bevidst valgt pr. mode, ikke blot en tilfældig konstant.
//     2) Det gør det muligt at differentiere grænsen pr. mode i fremtiden
//        (f.eks. hvis en mode får en anden ADC-forstærkning eller et
//        andet støjgulv).
//     3) Det opfylder REQ-FSM-11.02's krav om mode-specifik hysterese.
//
//   Grænsen 0.360V er valgt som 9% af ADC'ens full-scale (4.096V) —
//   dvs. 4.096 × 0.09 = 0.3686V, rundet ned til 0.360V for lidt ekstra
//   margin. Det massive overlap (fra 9% til 98%) er kernen i anti-
//   chatter-strategien: der skal en ændring på over 10× til, før
//   relæerne skifter frem og tilbage.
//
// BEGRUNDELSE (HVORFOR):
//   Uden denne funktion ville auto-ranging-algoritmen i Task_Metrology
//   (Sektion 12) ikke vide, hvornår den skal skifte gear ned. En hardkodet
//   grænse i Task_Metrology ville være svær at vedligeholde og umulig at
//   tilpasse pr. mode. Ved at samle grænserne i denne funktion opnås:
//
//     * Ét centralt sted at ændre grænsen for en given mode.
//     * Mulighed for at differentiere grænsen pr. mode uden at røre
//       Task_Metrology.
//     * Tydelig kobling mellem mode og grænse — man kan læse funktionen
//       og se, at f.eks. DCV og ACV har samme grænse, fordi de begge
//       lander på samme ADC-indgang.
//
//   HVORFOR 9% OG IKKE F.EKS. 50%?
//   Kravet (REQ-FSM-11.02) specificerer et "massivt, sikkert overlap"
//   mellem up-range og down-range. Hvis down-range lå tæt på up-range
//   (f.eks. ved 90%), ville en spænding, der svinger omkring grænsen,
//   få relæerne til at klapre frem og tilbage ("chatter"). Med 9% er
//   der et forhold på ca. 11× mellem de to grænser (4.000V / 0.360V).
//   Det betyder, at spændingen skal falde til under 1/11 af den værdi,
//   der udløste up-range, før der skiftes ned igen. Det giver en meget
//   stabil range-tilstand og forhindrer unødigt relæslid.
//
//   HVORFOR 0.360V OG IKKE 0.3686V (9% af 4.096V)?
//     0.3686V er den matematisk korrekte 9% af 4.096V. Men 0.360V er
//     valgt for at give lidt ekstra margin og fordi det er et "rundt"
//     tal, der er lettere at verificere i tests. Forskellen på 8.6 mV
//     er ubetydelig i praksis, men 0.360V er nemmere at dokumentere og
//     huske. Samme princip som 4.000V vs 4.014V for up-range.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   getDownRangeThresholdForMode() kaldes UDELUKKENDE fra Task_Metrology
//   i Sektion 12 — specifikt i auto-ranging-hysterese-blokken (trin 8):
//
//     float downThreshold = getDownRangeThresholdForMode(localMode);
//
//   Her bruges den til at afgøre, om filteredVoltage er faldet til eller
//   under grænsen, og om der derfor skal skiftes til et gear med mindre
//   dæmpning (for at få flere decimalers præcision).
//
//   Funktionen kaldes ALDRIG:
//     * Fra SCPI-parseren (den bruger manuel range, ikke auto-ranging).
//     * Fra GUI'en (den viser kun resultatet).
//     * Fra setup() eller ISR'en.
//     * Fra nogen anden funktion i firmwaren.
//
//   Funktionen er en ren "getter" — den læser ingen globale variable og
//   har ingen side-effekter. Den kan derfor kaldes fra enhver kontekst
//   uden mutex eller spinlock.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-FSM-11.02] HYSTERESE-ALGORITME (ANTI-CHATTER)
//     Kravet specificerer: "Down-range (Skift ned): Triggers først når
//     spændingen er faldet til ≤9% af det nuværende gear (f.eks. ≤0.360V).
//     Dette giver et massivt, sikkert overlap."
//     OPFYLDELSE: Funktionen returnerer præcis 0.360V for alle modes,
//     hvilket opfylder kravet om ≤9% af 4.096V. Uden denne funktion
//     ville kravet ikke være opfyldt — der ville ikke være nogen
//     defineret down-range-grænse.
//
//   * [REQ-AFE-7.02] SKIFT NED (DOWN-RANGING)
//     Kravet specificerer: "Hvis den udregnede displayværdi falder under
//     0.500V, og vi er i et højt gear, skal vi skifte ned for at få
//     flere decimalers præcision."
//     OPFYLDELSE: Funktionen returnerer 0.360V, som er under 0.500V —
//     dvs. kravets grænse er opfyldt med margin. Bemærk, at kravet
//     nævner 0.500V, mens REQ-FSM-11.02 nævner 9% (0.3686V). Firmwaren
//     følger REQ-FSM-11.02's 9%-regel (0.360V), som er den strengere
//     og mere veldefinerede af de to krav. Det giver et endnu større
//     overlap end REQ-AFE-7.02's 0.500V alene.
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE) — indirekte
//     Kravet specificerer, at systemet skal kunne skifte mellem modes.
//     OPFYLDELSE: Funktionen tager mode som parameter og returnerer en
//     mode-specifik grænse — det er forudsætningen for, at auto-ranging
//     kan fungere korrekt i alle modes.
//
//   * [REQ-DSP-6.01] ADC OPSÆTNING (ADS1115) — indirekte
//     Kravet specificerer, at ADC'en skal måle fra -4.096V til +4.096V.
//     OPFYLDELSE: Funktionens grænse på 0.360V er valgt med udgangspunkt
//     i denne full-scale (9% af 4.096V). Uden REQ-DSP-6.01's 4.096V-
//     reference ville 0.360V ikke give mening.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR (herunder hvilken mode grænsen gælder for).
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: getDownRangeThresholdForMode, MeasurementMode,
//     MODE_DCV osv. er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR RETURNERER ALLE MODES 0.360V?
//     Fordi alle målinger i sidste ende lander på ADS1115's indgang med
//     GAIN_ONE (±4.096V). Uanset om det er DCV, ACV, DCI, ACI eller Ohm,
//     er det den samme ADC-indgang, der skal beskyttes mod at stå i et
//     for højt gear. Grænsen er derfor identisk. Hvis en fremtidig
//     hardware-revision giver en mode en anden forstærkning, kan grænsen
//     differentieres her.
//
//   * HVORFOR ER DER EN default-CASE?
//     Hvis mode er en ugyldig enum-værdi (f.eks. pga. hukommelses-
//     korruption eller en fremtidig udvidelse, der glemmer at opdatere
//     denne funktion), returnerer default-casen 0.360V. Det er den
//     sikreste standard — den forhindrer, at systemet skifter ned til
//     et for lavt gear, hvor ADC'en kan overstyres. Uden default-case
//     ville funktionen returnere udefineret adfærd (i praksis 0 eller
//     garbage), hvilket kunne føre til, at ADC'en blev overstyret.
//
//   * HVORFOR float OG IKKE double?
//     filteredVoltage i Task_Metrology er float, og ADC'ens opløsning
//     er 16-bit (0.125 mV pr. LSB). float har 24-bit præcision, hvilket
//     er rigeligt til at repræsentere 0.360V med den nøjagtighed, der
//     er nødvendig. double ville bruge 8 bytes i stedet for 4 og give
//     unødvendig præcision — det ville også gøre sammenligningen i
//     Task_Metrology langsommere på ESP32 (double er 64-bit).
//
//   * HVORFOR ER FUNKTIONEN IKKE inline?
//     Compileren kan selv vælge at inline'e den, fordi den er kort og
//     kaldes fra ét sted. At markere den eksplicit inline ville ikke
//     ændre adfærden, men ville gøre koden mindre læsbar. Funktionen
//     er bevidst holdt som en separat funktion for at gøre det tydeligt,
//     at grænsen er mode-specifik.
//
//   * SIDE-EFFEKTER:
//     Ingen. Funktionen læser ingen globale variable, skriver ingen
//     globale variable, og har ingen bivirkninger. Den er en ren
//     "getter" og kan kaldes fra enhver kontekst uden mutex.
//
//   * RETURVÆRDI:
//     float — den nedre ADC-grænse i volt (altid 0.360f i denne version).
//
//   * FORHOLDSREGEL VED ÆNDRING AF ADC-GAIN:
//     Hvis ADC'ens gain ændres fra GAIN_ONE (4.096V) til f.eks. GAIN_TWO
//     (2.048V), SKAL denne funktions returværdi opdateres tilsvarende
//     (f.eks. til 0.180V, som er 9% af 2.048V). Ellers vil auto-ranging
//     ikke skifte gear korrekt, og ADC'en kan blive overstyret. Samme
//     forholdsregel gælder for getUpRangeThresholdForMode() i Sektion 7.
//
//   * FORHOLDSREGEL VED TILFØJELSE AF NY MODE:
//     Hvis en ny MeasurementMode tilføjes, SKAL denne funktion opdateres
//     med en case for den nye mode — ellers vil den falde tilbage til
//     default (0.360V), hvilket måske ikke er korrekt for den nye mode.
//     Samme forholdsregel gælder for getUpRangeThresholdForMode() og
//     getMaxGearForMode() i Sektion 7 samt mapModeAndGearToCalibrationSlot()
//     i Sektion 8.
//
//   * FORHOLDSREGEL VED ÆNDRING AF REQ-FSM-11.02's 9%-KRAV:
//     Hvis kravet ændres (f.eks. til 10% eller 15%), SKAL denne funktions
//     returværdi opdateres tilsvarende. 9% af 4.096V = 0.3686V; 10% =
//     0.4096V; 15% = 0.6144V. Den nuværende værdi (0.360V) er valgt som
//     "9% rundet ned til nærmeste 10 mV" for at give lidt ekstra margin.
//
//   * SAMMENHÆNG MED getUpRangeThresholdForMode():
//     De to funktioner er bevidst adskilt, så up-range og down-range kan
//     justeres uafhængigt. Hvis man ændrer den ene, bør man overveje, om
//     den anden også skal ændres, for at bevare det tilsigtede overlap.
//     Det nuværende forhold er 4.000V / 0.360V ≈ 11.1× — et massivt
//     overlap, der giver meget stabil range-tilstand.
//
// -------------------------------------------------------------------------
float getDownRangeThresholdForMode(MeasurementMode mode) {
    switch (mode) {                         // Vælg grænse ud fra måletilstand.
        case MeasurementMode::MODE_DCV:     // DC-spænding: 9% af 4.096V ≈ 0.360V.
            return 0.360f;                  // Down-range ved 0.360V.
        case MeasurementMode::MODE_ACV:     // AC-spænding: 9% af 4.096V RMS.
            return 0.360f;                  // Down-range ved 0.360V RMS.
        case MeasurementMode::MODE_DCI:     // DC-strøm: 9% af fuld shunt-skala.
            return 0.360f;                  // Down-range ved 0.360V over shunt.
        case MeasurementMode::MODE_ACI:     // AC-strøm: 9% af fuld shunt-skala.
            return 0.360f;                  // Down-range ved 0.360V RMS.
        case MeasurementMode::MODE_OHM_2W:  // 2-leder Ohm: 9% af fuld skala.
            return 0.360f;                  // Down-range ved 0.360V.
        case MeasurementMode::MODE_OHM_4W:  // 4-leder Ohm: 9% af fuld skala.
            return 0.360f;                  // Down-range ved 0.360V.
        default:                            // Ukendt mode: brug sikker standard.
            return 0.360f;                  // Fallback til 0.360V.
    }
}

// -------------------------------------------------------------------------
// 7.4 FUNKTION: getMaxGearForMode(MeasurementMode mode)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Returnerer det højeste gyldige gear-indeks for en given målemode.
//   Funktionen bruges af auto-ranging-algoritmen i Task_Metrology til at
//   afgøre, hvornår der IKKE kan skiftes længere ned (dvs. hvornår det
//   højeste gear-indeks er nået). Uden denne funktion ville auto-ranging
//   ikke vide, om gear 4 eller gear 5 er det sidste gyldige for en given
//   mode — og kunne forsøge at skifte til et ikke-eksisterende gear.
//
//   Funktionen returnerer:
//     * 4 for DCV og ACV (gear 0..4 = 5 gear i alt).
//     * 5 for DCI, ACI, OHM_2W og OHM_4W (gear 0..5 = 6 gear i alt).
//     * 4 som fallback for ukendte modes (sikker standard).
//
// GEAR-OVERSIGT (hvad indekserne betyder):
//   Spændingsmodes (DCV, ACV) — 5 gear:
//     Gear 0: 1000V range — DIV10 + DIV100 aktive (1:100 dæmpning).
//     Gear 1:  600V range — DIV10 + DIV100 aktive (1:100 dæmpning).
//     Gear 2:   60V range — DIV10 aktiv (1:10 dæmpning).
//     Gear 3:    6V range — ingen dæmper (1:1 via FET-buffer).
//     Gear 4:  600mV range — ingen dæmper (1:1 via FET-buffer).
//
//   Strøm- og Ohm-modes (DCI, ACI, OHM_2W, OHM_4W) — 6 gear:
//     Gear 0: højeste range (mest dæmpning / største shunt).
//     Gear 1-4: mellemliggende ranges.
//     Gear 5: laveste range (mindst dæmpning / mindste shunt).
//
//   Forskellen på 5 og 6 gear skyldes hardwaren: spændingsdeleren har kun
//   5 faste dæmpningstrin (1:100, 1:100, 1:10, 1:1, 1:1), mens strøm-
//   og Ohm-kredsløbene har 6 shunt-/modstandstrin (600µA/600Ω, 6mA/6kΩ,
//   60mA/60kΩ, 600mA/600kΩ, 6A/6MΩ, 10A/60MΩ).
//
// BEGRUNDELSE (HVORFOR):
//   Auto-ranging-algoritmen i Task_Metrology (Sektion 12, trin 8) skal
//   vide, hvornår den ikke kan skifte længere ned. Hvis den forsøgte at
//   skifte til gear 5 i DCV-mode (hvor max er 4), ville selectRelayGear()
//   ramme sin default-case og efterlade relæerne i en uændret tilstand —
//   men relayGearIndex ville blive sat til 5, hvilket er uden for det
//   gyldige interval. Det ville føre til:
//
//     * Forkert kalibreringsslot (mapModeAndGearToCalibrationSlot ville
//       returnere et slot uden for det tilsigtede interval).
//     * Forkert AC-ripple-skalering (getGearAttenuationFactor ville
//       returnere 1.0 som fallback).
//     * Forkert SCPI-svar på CONF:VOLT:DC? (ville rapportere gear 5).
//
//   Ved at kalde getMaxGearForMode() FØR hvert down-range-forsøg sikres
//   det, at auto-ranging aldrig forsøger at skifte til et ugyldigt gear.
//   Det er kernen i funktionens eksistensberettigelse.
//
//   Derudover giver funktionen ét centralt sted at ændre max gear, hvis
//   hardwaren senere får flere eller færre trin. Uden den ville max gear
//   være hardkodet i Task_Metrology, hvilket ville være svært at
//   vedligeholde og let at overse ved hardware-revisioner.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   getMaxGearForMode() kaldes UDELUKKENDE fra Task_Metrology i Sektion 12
//   — specifikt i auto-ranging-hysterese-blokken (trin 8):
//
//     uint8_t maxGear = getMaxGearForMode(localMode);
//
//   Her bruges den til at afgøre, om shouldRangeDown skal være sand:
//
//     bool shouldRangeDown = (filteredVoltage <= downThreshold) &&
//                            (currentGearSnapshot < maxGear);
//
//   Uden maxGear-værdien ville den anden betingelse ikke kunne evalueres,
//   og auto-ranging ville enten skifte for langt ned eller slet ikke
//   skifte ned.
//
//   Funktionen kaldes ALDRIG:
//     * Fra SCPI-parseren (den bruger manuel range og validerer selv
//       mod faste grænser — 0..4 for spænding, 0..5 for strøm/Ohm).
//     * Fra GUI'en (den viser kun resultatet).
//     * Fra setup() eller ISR'en.
//     * Fra selectRelayGear() (den antager, at newGearIndex er gyldigt).
//
//   Funktionen er en ren "getter" — den læser ingen globale variable og
//   har ingen side-effekter. Den kan derfor kaldes fra enhver kontekst
//   uden mutex eller spinlock.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-FSM-11.02] HYSTERESE-ALGORITME (ANTI-CHATTER)
//     Kravet specificerer: "Down-range (Skift ned): Triggers først når
//     spændingen er faldet til ≤9% af det nuværende gear."
//     OPFYLDELSE: Kravet forudsætter, at der findes et "nuværende gear"
//     og et "næste gear". getMaxGearForMode() leverer den øvre grænse for,
//     hvilke gear der findes — uden den ville down-range ikke kunne
//     afgøre, om der overhovedet er et lavere gear at skifte til.
//
//   * [REQ-AFE-7.02] SKIFT NED (DOWN-RANGING)
//     Kravet specificerer: "Hvis den udregnede displayværdi falder under
//     0.500V, og vi er i et højt gear, skal vi skifte ned for at få
//     flere decimalers præcision."
//     OPFYLDELSE: "Vi er i et højt gear" er præcis den betingelse, som
//     getMaxGearForMode() hjælper med at afgøre — hvis currentGearSnapshot
//     allerede er lig maxGear, er vi i det højeste gear, og der kan ikke
//     skiftes længere ned. Uden funktionen ville kravet ikke kunne opfyldes
//     korrekt.
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE) — indirekte
//     Kravet specificerer, at systemet skal kunne skifte mellem modes.
//     OPFYLDELSE: Funktionen tager mode som parameter og returnerer en
//     mode-specifik max-grænse — det er forudsætningen for, at auto-
//     ranging kan fungere korrekt i alle modes med deres respektive
//     gear-antal.
//
//   * [REQ-CAL-13.02] DATASTRUKTUR I FLASH — indirekte
//     Kravet specificerer 40 kalibreringsslots (DCV 5 + ACV 5 + DCI 6 +
//     ACI 6 + Ohm2W 6 + Ohm4W 6 = 34, rundet op til 40).
//     OPFYLDELSE: getMaxGearForMode() returnerer præcis de max-indekser,
//     der svarer til denne slot-fordeling. Uden funktionen ville
//     mapModeAndGearToCalibrationSlot() (Sektion 8) ikke kunne vide,
//     hvilke gear-indekser der er gyldige for en given mode — og
//     kalibreringsslots ville blive blandet sammen.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR (herunder hvilken mode grænsen gælder for).
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: getMaxGearForMode, MeasurementMode, MODE_DCV osv. er
//     alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR ER DCV OG ACV BEGGE 4 (5 GEAR)?
//     Fordi spændingsdeleren er fysisk den samme for DC og AC — kun
//     AC-ruten (AD8436) kobles ind eller ud. Dæmpningstrinene (1:100,
//     1:100, 1:10, 1:1, 1:1) er identiske. Derfor er max gear det samme.
//
//   * HVORFOR ER DCI, ACI, OHM_2W OG OHM_4W ALLE 5 (6 GEAR)?
//     Fordi strøm- og Ohm-kredsløbene har 6 fysiske shunt-/modstands-
//     trin. For DCI/ACI er det 6 shunts (600µA, 6mA, 60mA, 600mA, 6A,
//     10A); for OHM_2W/OHM_4W er det 6 teststrømme (600Ω, 6kΩ, 60kΩ,
//     600kΩ, 6MΩ, 60MΩ). De to Ohm-modes deler de samme 6 trin, men har
//     separate kalibreringsslots (buglist-rettelse, REQ-CAL-13.02).
//
//   * HVORFOR 2W OG 4W OHM HAR SAMME MAX GEAR?
//     Fordi 4-leder Kelvin-måling bruger de samme 6 teststrømme som
//     2-leder — forskellen er kun, om spændingsmålingen sker over DUT
//     (2W) eller over separate sense-linjer (4W). Gear-antallet er
//     derfor identisk.
//
//   * HVORFOR ER DER EN default-CASE?
//     Hvis mode er en ugyldig enum-værdi (f.eks. pga. hukommelses-
//     korruption eller en fremtidig udvidelse, der glemmer at opdatere
//     denne funktion), returnerer default-casen 4. Det er den mest
//     konservative standard — den antager, at der kun er 5 gear, hvilket
//     er det mindste antal i systemet. Hvis den nye mode i virkeligheden
//     har 6 gear, vil auto-ranging bare ikke udnytte det sidste gear —
//     hvilket er sikkert, men suboptimalt. Det er bedre end at forsøge
//     at skifte til et ikke-eksisterende gear.
//
//   * HVORFOR uint8_t OG IKKE int?
//     Fordi max gear er et lille tal (0..5), og uint8_t er den mindste
//     heltalstype. Det sparer hukommelse og gør det tydeligt, at værdien
//     er en lille ikke-negativ størrelse. Det matcher også typen på
//     relayGearIndex (volatile uint8_t) og currentGearSnapshot, så
//     sammenligningerne i Task_Metrology ikke kræver type-konvertering.
//
//   * SIDE-EFFEKTER:
//     Ingen. Funktionen læser ingen globale variable, skriver ingen
//     globale variable, og har ingen bivirkninger. Den er en ren
//     "getter" og kan kaldes fra enhver kontekst uden mutex.
//
//   * RETURVÆRDI:
//     uint8_t — det højeste gyldige gear-indeks for den givne mode
//     (4 for DCV/ACV, 5 for DCI/ACI/OHM_2W/OHM_4W, 4 som fallback).
//
//   * FORHOLDSREGEL VED TILFØJELSE AF NY MODE:
//     Hvis en ny MeasurementMode tilføjes, SKAL denne funktion opdateres
//     med en case for den nye mode — ellers vil den falde tilbage til
//     default (4), hvilket måske ikke er korrekt for den nye mode.
//     Samme forholdsregel gælder for getUpRangeThresholdForMode(),
//     getDownRangeThresholdForMode() og getGearAttenuationFactor() i
//     Sektion 7 samt mapModeAndGearToCalibrationSlot() i Sektion 8.
//
//   * FORHOLDSREGEL VED ÆNDRING AF HARDWARE-GEAR-ANTAL:
//     Hvis hardwaren ændres til f.eks. 7 gear for strøm, SKAL denne
//     funktions returværdi for DCI/ACI opdateres fra 5 til 6. Ellers
//     vil auto-ranging aldrig forsøge at bruge det nye gear. Samme
//     forholdsregel gælder for CALIBRATION_SLOT_COUNT (Sektion 3) og
//     mapModeAndGearToCalibrationSlot() (Sektion 8), som skal have
//     plads til de nye slots.
//
//   * SAMMENHÆNG MED getUpRangeThresholdForMode() OG getDownRangeThresholdForMode():
//     De tre funktioner arbejder sammen i auto-ranging-hysterese-blokken:
//       - getUpRangeThresholdForMode()   → hvornår skal vi skifte OP.
//       - getDownRangeThresholdForMode() → hvornår skal vi skifte NED.
//       - getMaxGearForMode()            → hvornår KAN vi ikke skifte
//                                          længere ned (øvre grænse).
//     Tilsammen udgør de den komplette hysterese-algoritme, som
//     REQ-FSM-11.02 kræver.
//
// -------------------------------------------------------------------------
uint8_t getMaxGearForMode(MeasurementMode mode) {
    switch (mode) {                         // Vælg max gear ud fra måletilstand.
        case MeasurementMode::MODE_DCV:     // DCV har gear 0..4.
            return 4;                       // 5 gear i alt.
        case MeasurementMode::MODE_ACV:     // ACV har gear 0..4.
            return 4;                       // 5 gear i alt.
        case MeasurementMode::MODE_DCI:     // DCI har gear 0..5.
            return 5;                       // 6 gear i alt.
        case MeasurementMode::MODE_ACI:     // ACI har gear 0..5.
            return 5;                       // 6 gear i alt.
        case MeasurementMode::MODE_OHM_2W:  // 2W Ohm har gear 0..5.
            return 5;                       // 6 gear i alt.
        case MeasurementMode::MODE_OHM_4W:  // 4W Ohm har gear 0..5.
            return 5;                       // 6 gear i alt.
        default:                            // Ukendt mode: brug sikker standard.
            return 4;                       // Fallback til 5 gear.
    }
}

// -------------------------------------------------------------------------
// 7.5 FUNKTION: getGearAttenuationFactor(uint8_t gearIndex)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Returnerer dæmpningsfaktoren — forholdet mellem indgangsspænding og
//   ADC-spænding — for et givet gear. F.eks. betyder 1:100 dæmpning, at
//   en indgangsspænding på 100V præsenteres som 1V for ADC'en. Funktionen
//   returnerer den faktor, man skal GANGE ADC-værdien med for at komme
//   tilbage til det faktiske indgangsniveau.
//
//   Konkret returnerer funktionen:
//     * Gear 0 (1000V): 100.0  — 1:100 dæmpning (DIV10+DIV100 aktive).
//     * Gear 1 ( 600V): 100.0  — 1:100 dæmpning (DIV10+DIV100 aktive).
//     * Gear 2 (  60V):  10.0  — 1:10 dæmpning (kun DIV10 aktiv).
//     * Gear 3 (   6V):   1.0  — 1:1 dæmpning (ingen dæmper).
//     * Gear 4 ( 600mV):  1.0  — 1:1 dæmpning (ingen dæmper).
//     * Ukendt gear:      1.0  — sikker fallback.
//
//   Funktionen bruges udelukkende til at skalere AC-ripple-målingen fra
//   AIN1 (AD8436's RMS-DC output) tilbage til det faktiske indgangsniveau.
//   Uden denne skalering ville f.eks. 60V-gearet vise en ripple-værdi,
//   der er 10× for lav, fordi AD8436 kun ser 1/10 af indgangsspændingen.
//
// BEGRUNDELSE (HVORFOR — inkl. RETTELSE buglist punkt 6):
//   AC-ripple-målingen (REQ-UI-14.02) foretages på AIN1, som er forbundet
//   til AD8436's output. AD8436 sidder EFTER spændingsdeleren — dvs. den
//   ser den SAMME dæmpede spænding, som AIN0 ser i DCV-mode. Hvis man
//   blot viste AD8436's output direkte som "AC Ripple", ville værdien
//   være dæmpet med gearets faktor:
//
//     * I 60V-gear (1:10) ville en ægte ripple på 1.0V blive vist som
//       0.1V — 10× for lav.
//     * I 600V/1000V-gear (1:100) ville en ægte ripple på 1.0V blive
//       vist som 0.01V — 100× for lav.
//
//   RETTELSEN (buglist punkt 6) er at gange AD8436's output med gearets
//   dæmpningsfaktor, FØR værdien sendes til GUI'en. Dermed viser "AC
//   Ripple: X.XXX V" altid den faktiske ripple på indgangen — uanset
//   hvilket gear instrumentet står i. Uden denne funktion ville brugeren
//   få et forkert billede af ripple-størrelsen, især i høje gear.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   getGearAttenuationFactor() kaldes UDELUKKENDE fra Task_Metrology i
//   Sektion 12 — specifikt i AC-ripple-polling-blokken (trin 3):
//
//     float rippleAttenuation = getGearAttenuationFactor(relayGearIndex);
//     latestAcRippleValue = rippleAdcVoltage * rippleAttenuation;
//
//   Her bruges den til at skalere den rå ADC-værdi fra AIN1 tilbage til
//   indgangsniveauet, før den gemmes i latestAcRippleValue og sendes til
//   Core 0 i MeasurementPacket.acRippleValue.
//
//   Funktionen kaldes ALDRIG:
//     * Fra SCPI-parseren (den viser kun de rå måleværdier).
//     * Fra GUI'en (den modtager allerede den skalerede værdi i pakken).
//     * Fra setup() eller ISR'en.
//     * Fra selectRelayGear() (den styrer kun relæerne, ikke skaleringen).
//
//   Funktionen er en ren "getter" — den læser ingen globale variable og
//   har ingen side-effekter. Den kan derfor kaldes fra enhver kontekst
//   uden mutex eller spinlock.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-UI-14.02] DUAL-DISPLAY MODE
//     Kravet specificerer: "Når instrumentet er i DCV-mode, og Blok 6
//     (True RMS) stadig kører parallelt i baggrunden, SKAL der kunne
//     renders en mindre tekst i Zone 3 (bund) med 'AC Ripple: X.XXX V'."
//     OPFYLDELSE: Funktionen er forudsætningen for, at den viste
//     ripple-værdi er KORREKT — dvs. viser den faktiske ripple på
//     indgangen, ikke den dæmpede ADC-værdi. Uden den ville kravet
//     være opfyldt formelt (der vises en tekst), men indholdet ville
//     være forkert i alle gear undtagen 1:1. Det er kernen i buglist
//     punkt 6-rettelsen.
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE) — indirekte
//     Kravet specificerer, at systemet skal kunne skifte mellem modes
//     og gear. OPFYLDELSE: Funktionen tager gearIndex som parameter og
//     returnerer den korrekte faktor for det aktive gear — det er
//     forudsætningen for, at AC-ripple-visningen følger med, når
//     gearet skifter.
//
//   * [REQ-AFE-7.03] RELÆ BLANKING — indirekte
//     Kravet specificerer, at relæerne skal have tid til at falde til
//     ro efter gearskift. OPFYLDELSE: Funktionen læser relayGearIndex,
//     som opdateres af selectRelayGear() EFTER relæerne er skiftet.
//     Dermed bruges altid den korrekte faktor for det aktuelle gear —
//     ikke det forrige.
//
//   * [REQ-CAL-13.01] DEN MATEMATISKE MODEL — indirekte
//     Kravet specificerer den lineære model V_display = (V_raw * Gain) +
//     Offset. OPFYLDELSE: Funktionens faktor er analog til Gain, men
//     bruges kun til AC-ripple-skalering — ikke til den primære måling.
//     Den primære måling bruger calibrationTable[].gain fra NVS, mens
//     ripple-skaleringen bruger denne funktions hardware-bestemte faktor.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR (herunder hvilket gear faktoren gælder for).
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: getGearAttenuationFactor, gearIndex, relayGearIndex
//     og latestAcRippleValue er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR RETURNERER GEAR 0 OG 1 BEGGE 100.0?
//     Fordi begge gear bruger samme hardware-konfiguration: DIV10+DIV100
//     aktive = 1:100 dæmpning. Forskellen på 1000V og 600V er ikke
//     dæmpningen, men den maksimale spænding, hardwaren må udsættes for
//     (og dermed hvilken kalibreringsslot der bruges). Set fra ADC'ens
//     synspunkt er de to gear identiske.
//
//   * HVORFOR RETURNERER GEAR 3 OG 4 BEGGE 1.0?
//     Fordi begge gear er 1:1 — ingen dæmper er aktiv. Forskellen på 6V
//     og 600mV er ADC'ens opløsning og kalibreringsslottens gain/offset,
//     ikke dæmpningen. Set fra ADC'ens synspunkt er de to gear identiske
//     mht. dæmpning.
//
//   * HVORFOR ER DER EN default-CASE?
//     Hvis gearIndex er uden for 0..4 (f.eks. pga. hukommelses-korruption
//     eller en fremtidig udvidelse, der glemmer at opdatere denne
//     funktion), returnerer default-casen 1.0. Det er den sikreste
//     standard — den antager, at der ikke er nogen dæmpning, hvilket
//     betyder, at ripple-værdien vises uden skalering. Hvis det
//     faktiske gear HAR dæmpning, vil værdien være for lav — men det
//     er bedre end at gange med et tilfældigt tal.
//
//   * HVORFOR float OG IKKE double?
//     Fordi værdien bruges til at gange en float (rippleAdcVoltage er
//     double, men resultatet gemmes i latestAcRippleValue, som er
//     double). float er tilstrækkeligt præcis til at repræsentere
//     1.0, 10.0 og 100.0, og det matcher typen på de øvrige
//     hjælpefunktioner i Sektion 7. Hvis man senere ændrer
//     latestAcRippleValue til float, vil der ikke være nogen
//     præcisionstab.
//
//   * HVORFOR ER FUNKTIONEN IKKE inline?
//     Compileren kan selv vælge at inline'e den, fordi den er kort og
//     kaldes fra ét sted. At markere den eksplicit inline ville ikke
//     ændre adfærden, men ville gøre koden mindre læsbar. Funktionen
//     er bevidst holdt som en separat funktion for at gøre det tydeligt,
//     at faktoren er gear-specifik.
//
//   * SIDE-EFFEKTER:
//     Ingen. Funktionen læser ingen globale variable, skriver ingen
//     globale variable, og har ingen bivirkninger. Den er en ren
//     "getter" og kan kaldes fra enhver kontekst uden mutex.
//
//   * RETURVÆRDI:
//     float — dæmpningsfaktoren for det givne gear (1.0, 10.0 eller
//     100.0, eller 1.0 som fallback).
//
//   * FORHOLDSREGEL VED ÆNDRING AF GEAR-MAPPING:
//     Hvis selectRelayGear() (Sektion 5) ændres, så et gear får en
//     anden dæmpning (f.eks. hvis gear 2 ændres fra 1:10 til 1:20),
//     SKAL denne funktion opdateres tilsvarende. Ellers vil AC-ripple-
//     visningen være forkert. Samme forholdsregel gælder for
//     getUpRangeThresholdForMode() og getDownRangeThresholdForMode()
//     (hvis ADC-full-scale ændres) samt for mapModeAndGearToCalibrationSlot()
//     (hvis gear-antal ændres).
//
//   * FORHOLDSREGEL VED TILFØJELSE AF NY MODE:
//     Hvis en ny MeasurementMode tilføjes, og den bruger et nyt gear-
//     indeks, SKAL denne funktion opdateres med en case for det nye
//     indeks. Ellers vil AC-ripple-skaleringen bruge fallback (1.0),
//     hvilket kan give forkerte ripple-værdier. Samme forholdsregel
//     gælder for getMaxGearForMode() og getGearAttenuationFactor()
//     selv — hvis gear-antallet ændres, skal denne funktion have flere
//     cases.
//
//   * SAMMENHÆNG MED selectRelayGear():
//     De to funktioner er tæt koblede: selectRelayGear() bestemmer,
//     HVILKEN dæmpning der er aktiv; getGearAttenuationFactor() fortæller,
//     HVAD den dæmpning er. Hvis man ændrer det ene, skal man ændre det
//     andet. Det er især vigtigt for gear 0 og 1, hvor HW-DOC tabel 4.2
//     kræver, at BEGGE dæmpere er aktive (buglist E-rettelse) — og hvor
//     denne funktion derfor returnerer 100.0, ikke 10.0.
//
// -------------------------------------------------------------------------
float getGearAttenuationFactor(uint8_t gearIndex) {
    switch (gearIndex) {                    // Vælg faktor ud fra gear-indeks.
        case 0:                             // 1000V-gear: 1:100 dæmpning.
            return 100.0f;                  // Indgang = ADC × 100.
        case 1:                             // 600V-gear: 1:100 dæmpning.
            return 100.0f;                  // Indgang = ADC × 100.
        case 2:                             // 60V-gear: 1:10 dæmpning.
            return 10.0f;                   // Indgang = ADC × 10.
        case 3:                             // 6V-gear: 1:1 dæmpning.
            return 1.0f;                    // Indgang = ADC × 1.
        case 4:                             // 600mV-gear: 1:1 dæmpning.
            return 1.0f;                    // Indgang = ADC × 1.
        default:                            // Ukendt gear: brug sikker standard.
            return 1.0f;                    // Fallback til 1:1.
    }
}

// =========================================================================
// SEKTION 8: KALIBRERINGS-SLOT-MAPPING — REQ-CAL-13.02
// =========================================================================

// -------------------------------------------------------------------------
// 8.2 FUNKTION: mapModeAndGearToCalibrationSlot(MeasurementMode mode, uint8_t gear)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Mapper en kombination af (mode, gear) til et entydigt kalibreringsslot
//   i intervallet 0..39. Hvert slot svarer til ét fysisk måleområde — dvs.
//   én bestemt relækombination i én bestemt målemode — og har sit eget
//   sæt af gain/offset-konstanter gemt i NVS (se Sektion 4).
//
//   Funktionen er en switch-case, der først vælger en base ud fra mode,
//   og derefter lægger gear-indekset til. Resultatet er en fast, entydig
//   mapping:
//
//     Mode          Base   Gear-interval   Slot-interval   Antal
//     -----------   ----   -------------   -------------   -----
//     MODE_DCV        0      0..4            0..4            5
//     MODE_ACV        5      0..4            5..9            5
//     MODE_DCI       10      0..5           10..15           6
//     MODE_ACI       16      0..5           16..21           6
//     MODE_OHM_2W    22      0..5           22..27           6
//     MODE_OHM_4W    28      0..5           28..33           6
//     (ukendt)        0      —               0               1 (fallback)
//
//   Tilsammen 34 slots (5+5+6+6+6+6), men CALIBRATION_SLOT_COUNT er sat
//   til 40 for at give plads til fremtidige udvidelser og for at undgå
//   modulo-overlap.
//
//   Hvis gear er uden for det gyldige interval for moden (f.eks. gear 5
//   i DCV-mode, hvor max er 4), returneres base-slot'et (f.eks. 0 for
//   DCV) i stedet for et slot uden for intervallet. Det er en
//   sikkerhedsforanstaltning mod out-of-bounds-adgang.
//
// BEGRUNDELSE (HVORFOR — inkl. RETTELSE REQ-CAL-13.02):
//   Hvert fysisk måleområde har sine egne tolerancer: relækontakt-
//   modstand, dæmper-modstande, shunt-tolerance og offset-drift er
//   forskellige for hvert gear. Derfor skal hvert (mode, gear)-par have
//   sit EGET sæt gain/offset i NVS. Uden denne funktion ville
//   Task_Metrology ikke kunne vide, hvilket slot der hører til den
//   aktuelle måling — og kalibreringen ville være meningsløs.
//
//   RETTELSE (REQ-CAL-13.02) — OHM_4W FÅR EGNE SLOTS:
//     Den oprindelige kode modulo-mappede 6 Ohm-4W-gear ind i kun 4
//     slots, hvilket betød, at f.eks. gear 0 og gear 4 (600Ω og 6MΩ)
//     delte samme slot. Det blandede kalibreringen for vidt forskellige
//     modstandsområder — en kalibrering af 600Ω ville ændre gain for
//     6MΩ, hvilket er katastrofalt for præcisionen.
//
//     Rettelsen er, at OHM_4W nu får sine EGNE 6 slots (28..33), adskilt
//     fra OHM_2W's 6 slots (22..27). Dermed har hvert eneste fysiske
//     måleområde sit eget slot — 34 i alt, som specificeret i REQ-CAL-13.02.
//     CALIBRATION_SLOT_COUNT blev samtidig udvidet fra 32 til 40 for at
//     give plads uden at ændre de eksisterende slot-numre.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   mapModeAndGearToCalibrationSlot() kaldes UDELUKKENDE fra Task_Metrology
//   i Sektion 12 — specifikt i kalibreringsslot-valg-blokken (trin 9):
//
//     int calibrationSlot = mapModeAndGearToCalibrationSlot(localMode, relayGearIndex);
//     if (calibrationSlot < 0 || calibrationSlot >= CALIBRATION_SLOT_COUNT) {
//         calibrationSlot = 0;
//     }
//     xSemaphoreTake(calibrationMutex, portMAX_DELAY);
//     CalibrationData activeCal = calibrationTable[calibrationSlot];
//     xSemaphoreGive(calibrationMutex);
//
//   Her bruges resultatet til at slå det korrekte gain/offset-par op i
//   calibrationTable[] (RAM-kopien af NVS-data, se Sektion 3 og 4).
//   Resultatet gemmes også i MeasurementPacket.calibrationSlot, så
//   SCPI-kommandoerne CAL:VAL og CAL:ZERO kan bruge samme slot som den
//   måling, de kalibrerer mod (se Sektion 11).
//
//   Funktionen kaldes ALDRIG:
//     * Fra SCPI-parseren (den bruger snapshot.calibrationSlot fra pakken).
//     * Fra GUI'en (den viser kun resultatet).
//     * Fra setup() eller ISR'en.
//     * Fra loadCalibrationFromNvs() eller saveCalibrationToNvs() (de
//       arbejder med slot-numre, ikke mode/gear).
//
//   Funktionen er en ren "getter" — den læser ingen globale variable og
//   har ingen side-effekter. Den kan derfor kaldes fra enhver kontekst
//   uden mutex eller spinlock.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-CAL-13.02] DATASTRUKTUR I FLASH
//     Kravet specificerer: "Der SKAL oprettes en struct CalibrationData
//     bestående af to double variabler (Gain og Offset) for de cirka 25
//     forskellige måleområder (DCV 600mV...1000V, ACV 600mV...750V, DCI,
//     Ohm osv.)."
//     OPFYLDELSE: Funktionen er det, der mapper de faktiske (mode, gear)-
//     par til de 34 slots, som kravet omtaler. Uden den ville struct'en
//     være defineret, men uden en entydig måde at slå det korrekte slot
//     op på. Rettelsen (OHM_4W får egne slots) er en direkte konsekvens
//     af kravet om, at HVERT måleområde skal have sit eget sæt.
//
//   * [REQ-CAL-13.01] DEN MATEMATISKE MODEL
//     Kravet specificerer: "Hvert eneste gear (hver relækombination)
//     skal have sit eget sæt af kalibreringskonstanter. Koden bruger en
//     lineær transformation: V_display = (V_raw * Gain) + Offset."
//     OPFYLDELSE: Funktionen sikrer, at hvert gear faktisk FÅR sit eget
//     sæt — uden den ville flere gear dele samme slot (som fejlen var
//     før rettelsen), og den lineære model ville bruge forkerte
//     konstanter for nogle gear.
//
//   * [REQ-CAL-13.03] KALIBRERINGSPROCEDURE (KODENIVEAU)
//     Kravet specificerer, at CAL:VAL og CAL:ZERO skal gemme resultatet
//     for det aktive måleområde. OPFYLDELSE: Funktionens resultat gemmes
//     i MeasurementPacket.calibrationSlot, som SCPI-parseren bruger til
//     at vide, hvilket slot der skal opdateres. Uden funktionen ville
//     CAL:VAL/CAL:ZERO ikke vide, hvilket slot der er aktivt.
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE) — indirekte
//     Kravet specificerer, at systemet skal kunne skifte mellem modes.
//     OPFYLDELSE: Funktionen tager mode som parameter og returnerer det
//     korrekte slot for den aktuelle mode — det er forudsætningen for,
//     at kalibreringen følger med, når moden skifter.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR (herunder hvilket slot-interval der gælder for hvilken
//     mode).
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: mapModeAndGearToCalibrationSlot, calibrationSlot,
//     MeasurementMode, MODE_DCV osv. er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR FAST SLOT-NUMMER I STEDET FOR EN HASH ELLER MAP?
//     Fast slot-nummer giver:
//       - O(1) opslag (et simpelt regnestykke, ingen hashtabel).
//       - Deterministisk adfærd (samme input giver altid samme slot).
//       - Ingen risiko for hash-kollisioner (to gear der deler slot).
//       - Let at verificere i tests (slot 12 er ALTID DCI gear 2).
//     En hash eller std::map ville være langsommere, mere kompleks og
//     sværere at dokumentere. Den faste mapping er også nem at inspicere
//     i NVS, hvis man vil se, hvilke slots der er gemt.
//
//   * HVORFOR 40 SLOTS OG IKKE 34?
//     Der er 34 faktiske slots (5+5+6+6+6+6), men CALIBRATION_SLOT_COUNT
//     er sat til 40. Det giver:
//       - 6 slots til fremtidige udvidelser (f.eks. en ny mode).
//       - Mulighed for at tilføje slots uden at ændre eksisterende
//         slot-numre (bagudkompatibilitet med NVS).
//       - En "rund" størrelse, der er let at huske og dokumentere.
//     De 6 ubrugte slots (34..39) får default gain=1.0, offset=0.0 af
//     loadCalibrationFromNvs() og bruges ikke i den nuværende version.
//
//   * HVORFOR RETURNERES BASE-SLOT VED UGYLDIGT GEAR?
//     Hvis gear er uden for det gyldige interval (f.eks. gear 5 i DCV-
//     mode), kunne man returnere -1 eller et slot uden for intervallet.
//     Men det ville kræve, at kalderen håndterer fejlen — og Task_Metrology
//     har ikke tid til fejlhåndtering i sin hot path. Ved at returnere
//     base-slot'et (f.eks. 0 for DCV) får man et gyldigt slot med
//     fabriksdefault (gain=1.0, offset=0.0), hvilket er sikkert —
//     målingen vil være upræcis, men ikke crash'e.
//
//   * HVORFOR ER DER EN default-CASE?
//     Hvis mode er en ugyldig enum-værdi (f.eks. pga. hukommelses-
//     korruption eller en fremtidig udvidelse, der glemmer at opdatere
//     denne funktion), returnerer default-casen 0. Det er DCV 600mV —
//     det mest følsomme slot. Hvorfor ikke et "sikkert" slot som 0?
//     Fordi slot 0 ER DCV 600mV, og det er det slot, der bruges i
//     safe state (gear 0 = 1000V, men slot 0 = 600mV). Hvis nogen
//     utilsigtet rammer default, vil de se en upræcis måling i 600mV-
//     området — hvilket er tydeligt forkert, hvis de faktisk måler
//     1000V. Det er bedre end at vise en "pæn" værdi, der er forkert.
//
//   * HVORFOR int OG IKKE uint8_t SOM RETURTYPE?
//     Slot-numre går op til 39, hvilket passer i uint8_t. Men int giver
//     mulighed for at returnere -1 i fremtiden (f.eks. hvis man vil
//     signalere "ugyldigt slot" eksplicit). Det gør funktionen mere
//     fleksibel uden at koste noget i praksis (int og uint8_t fylder
//     det samme i registre på ESP32). Task_Metrology tjekker desuden
//     resultatet mod CALIBRATION_SLOT_COUNT som en ekstra sikkerhed.
//
//   * SIDE-EFFEKTER:
//     Ingen. Funktionen læser ingen globale variable, skriver ingen
//     globale variable, og har ingen bivirkninger. Den er en ren
//     "getter" og kan kaldes fra enhver kontekst uden mutex.
//
//   * RETURVÆRDI:
//     int — kalibreringsslot i intervallet 0..33 (eller 0 som fallback).
//     Returnerer ALDRIG et slot uden for 0..CALIBRATION_SLOT_COUNT-1.
//
//   * FORHOLDSREGEL VED TILFØJELSE AF NY MODE:
//     Hvis en ny MeasurementMode tilføjes, SKAL denne funktion opdateres
//     med en case for den nye mode — ellers vil den falde tilbage til
//     slot 0 (DCV 600mV), hvilket er forkert for den nye mode. Samtidig
//     SKAL CALIBRATION_SLOT_COUNT (Sektion 3) muligvis øges, hvis den
//     nye mode har flere gear end de ledige slots (34..39). Samme
//     forholdsregel gælder for getMaxGearForMode() i Sektion 7.
//
//   * FORHOLDSREGEL VED ÆNDRING AF GEAR-ANTAL:
//     Hvis et gear-antal ændres (f.eks. DCV får 6 gear i stedet for 5),
//     SKAL slot-intervallerne for ALLE efterfølgende modes rykkes — og
//     NVS-data skal migreres, hvis instrumentet allerede er i brug. Det
//     er en ikke-triviel operation, der kræver en firmware-opdatering
//     med eksplicit NVS-migration. Dokumentér altid slot-mappingen i
//     kravspecifikationen, før man ændrer den.
//
//   * SAMMENHÆNG MED loadCalibrationFromNvs() OG saveCalibrationToNvs():
//     De to NVS-funktioner (Sektion 4) arbejder med slot-numre direkte
//     — de kender ikke mode/gear. Denne funktion er det eneste sted,
//     hvor mode/gear oversættes til slot. Hvis man ændrer mappingen,
//     påvirker det:
//       - Hvilke NVS-nøgler ("gain<N>", "offs<N>") der læses/skrives.
//       - Hvilke kalibreringsdata der bruges ved en given måling.
//       - Hvilke slots CAL:VAL/CAL:ZERO opdaterer.
//     Derfor skal mappingen være stabil og veldokumenteret.
//
//   * SAMMENHÆNG MED MeasurementPacket.calibrationSlot:
//     Resultatet gemmes i MeasurementPacket.calibrationSlot (Sektion 3),
//     så SCPI-parseren kan bruge samme slot som den måling, den
//     kalibrerer mod. Uden denne kobling ville CAL:VAL/CAL:ZERO risikere
//     at kalibrere et andet slot end det, der aktuelt måles — f.eks.
//     hvis gearet skiftede mellem målingen og kalibreringskommandoen.
//
// -------------------------------------------------------------------------
int mapModeAndGearToCalibrationSlot(MeasurementMode mode, uint8_t gear) {
    switch (mode) {                         // Vælg base-slot ud fra mode.
        case MeasurementMode::MODE_DCV:     // DCV slots 0..4.
            return (gear <= 4) ? (0 + gear) : 0;             // Map gear 0..4 → slot 0..4.
        case MeasurementMode::MODE_ACV:     // ACV slots 5..9.
            return (gear <= 4) ? (5 + gear) : 5;             // Map gear 0..4 → slot 5..9.
        case MeasurementMode::MODE_DCI:     // DCI slots 10..15.
            return (gear <= 5) ? (10 + gear) : 10;           // Map gear 0..5 → slot 10..15.
        case MeasurementMode::MODE_ACI:     // ACI slots 16..21.
            return (gear <= 5) ? (16 + gear) : 16;           // Map gear 0..5 → slot 16..21.
        case MeasurementMode::MODE_OHM_2W:  // Ohm 2W slots 22..27.
            return (gear <= 5) ? (22 + gear) : 22;           // Map gear 0..5 → slot 22..27.
        case MeasurementMode::MODE_OHM_4W:  // Ohm 4W slots 28..33 (egne 6 slotte, ingen modulo).
            return (gear <= 5) ? (28 + gear) : 28;           // Map gear 0..5 → slot 28..33.
        default:                            // Ukendt mode: brug slot 0.
            return 0;                       // Fallback til DCV 600mV.
    }
}

// =========================================================================
// SEKTION 9: TREND-GRAF HJÆLPEFUNKTION — REQ-UI-14.03
// =========================================================================

// Globale variable til trend-graf, så de overlever mellem kald og kan nulstilles.
float trendBufferYValues[TREND_BUFFER_SIZE]; // Cirkulær buffer med Y-værdier for de seneste 400 punkter.
int trendWriteIndex = 0;                     // Skriveposition i den cirkulære buffer.
bool trendBufferInitialized = false;         // Flag: er bufferen klar til brug?

// -------------------------------------------------------------------------
// 9.2 FUNKTION: renderTrendGraph(float newYValue, float minYValue, float maxYValue)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Tegner den rullende trend-graf på TFT-skærmen ud fra den cirkulære
//   Y-buffer trendBufferYValues[TREND_BUFFER_SIZE]. Funktionen gør fem
//   ting i fast rækkefølge:
//
//     1) Hvis bufferen ikke er initialiseret endnu (første kald efter
//        opstart eller reset), fyldes HELE bufferen med newYValue, så
//        grafen starter som en vandret linje i stedet for at hoppe fra
//        nul. trendBufferInitialized sættes til true.
//
//     2) Det nye punkt (newYValue) gemmes i bufferen på position
//        trendWriteIndex, og trendWriteIndex rykkes én plads frem
//        (med wrap-around via modulo TREND_BUFFER_SIZE).
//
//     3) Grafen rammes ind: graphX0=20, graphY0=200, graphWidth=760,
//        graphHeight=200. Det er det område på skærmen, hvor grafen
//        tegnes — under det store tal og over ripple-linjen.
//
//     4) Værdi-spændet (maxYValue - minYValue) beregnes og bruges til
//        at normalisere hver Y-værdi til 0..1. Hvis spændet er nul
//        eller negativt, sættes det til 1.0f for at undgå division
//        med nul.
//
//     5) Bufferen gennemløbes i kronologisk rækkefølge (startende ved
//        trendWriteIndex, så det ældste punkt kommer først). Hvert
//        punkt mappes til en X-pixel (lineært fra graphX0 til
//        graphX0+graphWidth) og en Y-pixel (normaliseret værdi vendt
//        om, så høj værdi = lav Y-pixel = top af grafen). Der tegnes
//        en cyan linje fra forrige punkt til det aktuelle — undtagen
//        for det første punkt, hvor der ikke er noget "forrige".
//
//   Resultatet er en rullende graf, hvor hvert nyt sample skubber
//   grafen én plads til venstre (i praksis: det ældste punkt
//   overskrives, og hele linjen tegnes op igen).
//
// BEGRUNDELSE (HVORFOR):
//   Trend-grafen giver brugeren et visuelt billede af, hvordan
//   målingen udvikler sig over tid — noget et rent tal ikke kan.
//   Den er især nyttig til at spotte:
//     * Drift i en spændingsforsyning (langsomt stigende/faldende).
//     * Periodisk støj (50 Hz-ripple, switching-støj).
//     * Transienter (korte spidser, der ikke ses i et gennemsnit).
//     * Langsomme ændringer (termisk drift, batteri-afladning).
//
//   Uden denne funktion ville brugeren kun se det aktuelle tal, og
//   ville ikke kunne se, om værdien er stabil eller svinger.
//
//   Funktionen er bevidst holdt simpel: den tegner HELE grafen op
//   hver gang i stedet for kun at opdatere de ændrede segmenter.
//   Det er en bevidst afvejning — se designvalg nedenfor.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   renderTrendGraph() kaldes UDELUKKENDE fra Task_GUI_SCPI i Sektion 14
//   — specifikt i GUI-opdateringsblokken (trin 5), når graphModeActive
//   er true:
//
//     if (graphModeActive) {
//         tft.fillRect(0, 180, 800, 300, TFT_BLACK);   // Slet graf-området
//         xSemaphoreTake(trendMutex, portMAX_DELAY);
//         renderTrendGraph((float)receivedPacket.finalValue, 0.0f, 5.0f);
//         xSemaphoreGive(trendMutex);
//     }
//
//   Her kaldes funktionen med:
//     * newYValue = receivedPacket.finalValue (den aktuelle måling).
//     * minYValue = 0.0f (fast nedre grænse).
//     * maxYValue = 5.0f (fast øvre grænse).
//
//   De faste grænser 0..5V er valgt, fordi det er det typiske
//   spændingsinterval for de fleste målinger. Hvis brugeren måler
//   højere spændinger, vil grafen blive klippet ved 5V (normalized
//   clampes til 1.0) — det er en kendt begrænsning i denne version.
//   Fremtidige versioner kan gøre grænserne dynamiske baseret på
//   gearet.
//
//   Funktionen kaldes ALDRIG:
//     * Fra Task_Metrology (den må ikke tegne på skærmen — det er
//       Core 0's ansvar).
//     * Fra SCPI-parseren (den kan kun aktivere/deaktivere graf-mode).
//     * Fra setup() eller ISR'en.
//     * Uden at trendMutex er taget (det er en forudsætning).
//
//   VIGTIGT — MUTEX-FORUDSÆTNING:
//   Funktionen forudsætter, at kalderen har taget trendMutex. Det
//   sikrer, at Task_GUI_SCPI ikke samtidig ændrer trendBufferYValues[]
//   via resetTrendGraph() (som kaldes fra Task_Metrology ved gearskift).
//   Uden mutex kunne grafen tegne et halvt-ændret buffer og vise
//   forkerte data.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-UI-14.03] RULLENDE TREND-GRAF
//     Kravet specificerer: "I et dedikeret graf-mode optegnes en linje
//     baseret på de seneste 400 punkter. Dette kræver en cirkulær
//     buffer i RAM, der holder Y-koordinater. For hver ny pakke
//     slettes grafen, bufferen skiftes én plads til venstre, og linjen
//     tegnes op igen med funktionen tft.drawLine(). For at mindske
//     flickering optimeres dette ved kun at opdatere de berørte linjer."
//     OPFYLDELSE: renderTrendGraph() er præcis den funktion, kravet
//     beskriver. Den:
//       - Bruger trendBufferYValues[TREND_BUFFER_SIZE=400] (cirkulær
//         buffer i RAM).
//       - Skubber bufferen én plads frem (via trendWriteIndex) for
//         hver ny pakke.
//       - Tegner linjen op igen med tft.drawLine().
//     Bemærk: kravet nævner "kun at opdatere de berørte linjer" for
//     at mindske flickering. Firmwaren løser dette ved at slette HELE
//     graf-området med fillRect FØR tegning (se Task_GUI_SCPI), hvilket
//     er en forenkling — det giver stadig acceptabel ydeevne, fordi
//     fillRect er hurtigere end 400 individuelle drawLine-kald. En
//     mere avanceret variant (kun opdatere ændrede segmenter) er
//     planlagt, men ikke implementeret.
//
//   * [REQ-UI-14.01] ZONEDELING AF SKÆRMEN — indirekte
//     Kravet specificerer, at skærmen skal opdeles i zoner. OPFYLDELSE:
//     Graf-området (graphX0=20, graphY0=200, graphWidth=760,
//     graphHeight=200) er netop den "midterste" zone, hvor grafen
//     tegnes. Uden denne funktion ville zone-opdelingen ikke blive
//     brugt til graf-mode.
//
//   * [REQ-RTOS-4.01] TASK ALLOKERING — indirekte
//     Kravet specificerer, at Core 0 kører GUI'en. OPFYLDELSE:
//     Funktionen kaldes kun fra Task_GUI_SCPI (Core 0), hvilket
//     opfylder kravet om, at al grafik foregår på bruger-kernen.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: renderTrendGraph, trendBufferInitialized,
//     trendBufferYValues, trendWriteIndex, graphX0, graphWidth,
//     previousPixelX, normalized osv. er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR FÆLDES HELE BUFFEREN VED FØRSTE KALD?
//     Hvis bufferen blev initialiseret med nuller, ville grafen starte
//     med at hoppe fra 0 op til den første måling — et visuelt "spring",
//     der ikke afspejler virkeligheden. Ved at fylde med newYValue
//     starter grafen som en vandret linje på det aktuelle niveau, og
//     brugeren ser kun ægte ændringer. Det er især vigtigt efter et
//     gearskift eller reset, hvor målingen kan være på et helt andet
//     niveau end før.
//
//   * HVORFOR LÆSES BUFFEREN I KRONOLOGISK RÆKKEFØLGE FRA trendWriteIndex?
//     Fordi trendBufferYValues[] er en cirkulær buffer: det ældste
//     punkt er på trendWriteIndex, det næstældste på trendWriteIndex+1,
//     osv. Ved at læse fra trendWriteIndex og fremefter (med modulo)
//     får man punkterne i den rækkefølge, de blev samplet — hvilket er
//     præcis den rækkefølge, grafen skal vise (ældst til venstre,
//     nyest til højre).
//
//   * HVORFOR ER DER EN KLAMP AF normalized?
//     Hvis en måling ligger uden for [minYValue, maxYValue] (f.eks.
//     en transient over 5V i den nuværende konfiguration), ville
//     normalized blive <0 eller >1, og pixelY ville ligge uden for
//     graf-området. Det ville tegne linjer ud over skærmen eller ind
//     i andre zoner. Klampen sikrer, at grafen altid holder sig inden
//     for det tildelte område — transiente spidser bliver klippet ved
//     rammen i stedet for at ødelægge layoutet.
//
//   * HVORFOR TEGNES FØRSTE PUNKT IKKE?
//     Fordi der ikke er noget "forrige punkt" at tegne en linje fra.
//     Hvis man tegnede fra (0,0) til første punkt, ville der komme en
//     skæv linje fra øverste venstre hjørne. Ved at starte med
//     pointIndex > 0 undgås dette.
//
//   * HVORFOR previousPixelX/Y INITIALISERES TIL BUNDEN AF GRAFEN?
//     Fordi det første punkt (pointIndex=0) ikke tegnes, men dets
//     koordinater gemmes som "forrige". Initialiseringen er derfor
//     uden praktisk betydning — den overskrives i første iteration.
//     At sætte den til grafens nederste venstre hjørne er en
//     "nøytral" startværdi, der ikke kan forvirre læseren.
//
//   * HVORFOR ER DER EN valueSpan <= 0 CHECK?
//     Hvis maxYValue == minYValue (f.eks. hvis man kalder funktionen
//     med 0..0), ville division med nul give NaN eller inf, og
//     pixelY ville blive meningsløs. Ved at sætte valueSpan til 1.0f
//     undgås dette, og normalized bliver 0 for alle punkter (grafen
//     bliver en vandret linje i bunden). Det er en sikker fallback.
//
//   * HVORFOR TEGNES HELE GRAFEN OP HVER GANG (IKKE KUN ÆNDREDE SEGMENTER)?
//     Fordi det er enklere og mere robust. Med 400 punkter tager det
//     ca. 400 × ~10 µs = 4 ms at tegne hele grafen — hvilket er
//     acceptabelt ved den lave opdateringsfrekvens (~83 Hz). En mere
//     avanceret variant (kun opdatere ændrede segmenter) ville kræve
//     at spore, hvilke punkter der er ændret, og at tegne baggrund
//     over de gamle linjer — mere komplekst og lettere at fejle.
//     Kravet nævner denne optimering, men den er ikke implementeret
//     i denne version (se REQ-UI-14.03's OPFYLDELSE ovenfor).
//
//   * HVORFOR ER GRAF-OMRÅDET PLACERET SOM DET ER (x0=20, y0=200, 760×200)?
//     Skærmen er 800×480. Graf-området:
//       - Starter 20 px fra venstre (margin).
//       - Slutter 20 px fra højre (760 + 20 = 780, så 20 px margin).
//       - Starter ved y=200 (under det store tal i Zone 2).
//       - Slutter ved y=400 (200 + 200 = 400, så 80 px til ripple-
//         linjen i Zone 3 ved y=440).
//     Det giver plads til både det store tal (over grafen) og
//     ripple-visningen (under grafen) — alle tre zoner kan ses
//     samtidigt.
//
//   * SIDE-EFFEKTER:
//     - Opdaterer trendBufferYValues[] og trendWriteIndex (globale).
//     - Tegner op til 399 linjesegmenter på TFT-skærmen.
//     - Blokerer i ca. 4 ms (afhængigt af SPI-hastigheden).
//     - Påvirker IKKE calibrationTable, systemHalted eller andre
//       globals.
//
//   * RETURVÆRDI:
//     Ingen (void). Funktionen tegner og returnerer.
//
//   * FORHOLDSREGEL VED ÆNDRING AF TREND_BUFFER_SIZE:
//     Hvis TREND_BUFFER_SIZE ændres fra 400 (Sektion 3), skal man
//     være opmærksom på:
//       - Flere punkter giver finere graf, men mere CPU-tid pr. tegning.
//       - Færre punkter giver grovere graf, men hurtigere tegning.
//       - Kravet (REQ-UI-14.03) specificerer 400 punkter — en ændring
//         kræver en opdatering af kravspecifikationen.
//
//   * FORHOLDSREGEL VED ÆNDRING AF GRAF-OMRÅDET:
//     Hvis graphX0, graphY0, graphWidth eller graphHeight ændres, skal
//     man sikre, at:
//       - Området ikke overlapper det store tal (Zone 2).
//       - Området ikke overlapper ripple-linjen (Zone 3).
//       - fillRect-kaldet i Task_GUI_SCPI (0, 180, 800, 300) matcher
//         det nye område — ellers vil gamle streger blive efterladt.
//
//   * FORHOLDSREGEL VED ÆNDRING AF minYValue/maxYValue:
//     Hvis man gør grænserne dynamiske (f.eks. baseret på gearet), skal
//     man sikre, at:
//       - valueSpan aldrig bliver 0 (der er en check, men den bør
//         stadig overvejes).
//       - Grænserne passer til det aktuelle måleområde, så grafen
//         udnytter hele højden.
//       - Skalaen er stabil, så grafen ikke hopper visuelt ved små
//         ændringer.
//
// -------------------------------------------------------------------------
void renderTrendGraph(float newYValue, float minYValue, float maxYValue) {
    if (!trendBufferInitialized) {           // Første kald: initialisér bufferen med den nye værdi.
        for (int initIndex = 0; initIndex < TREND_BUFFER_SIZE; initIndex++) { // Løb alle punkter igennem.
            trendBufferYValues[initIndex] = newYValue; // Fyld med startværdien, så grafen ikke hopper fra nul.
        }
        trendBufferInitialized = true;       // Marker at bufferen nu er klar.
    }
    trendBufferYValues[trendWriteIndex] = newYValue; // Gem det nye punkt i bufferen.
    trendWriteIndex = (trendWriteIndex + 1) % TREND_BUFFER_SIZE; // Ryk skrivepositionen én plads frem (wrap around).
    const int graphX0 = 20;                  // Grafens venstre X-koordinat på skærmen.
    const int graphY0 = 200;                 // Grafens top Y-koordinat på skærmen.
    const int graphWidth = 760;              // Grafens bredde i pixels.
    const int graphHeight = 200;             // Grafens højde i pixels.
    float valueSpan = maxYValue - minYValue; // Beregn værdi-spændet for at skalere til pixels.
    if (valueSpan <= 0.0f) valueSpan = 1.0f; // Undgå division med nul hvis span er nul.
    int previousPixelX = graphX0;            // Startpunkt for forrige linje-segment.
    int previousPixelY = graphY0 + graphHeight; // Startpunkt for forrige linje-segment.
    for (int pointIndex = 0; pointIndex < TREND_BUFFER_SIZE; pointIndex++) { // Løb alle punkter i bufferen igennem.
        int bufferReadIndex = (trendWriteIndex + pointIndex) % TREND_BUFFER_SIZE; // Læs i kronologisk rækkefølge.
        float pointValue = trendBufferYValues[bufferReadIndex]; // Hent Y-værdien for dette punkt.
        int pixelX = graphX0 + (pointIndex * graphWidth) / TREND_BUFFER_SIZE; // Map punktindeks til X-pixel.
        float normalized = (pointValue - minYValue) / valueSpan; // Normaliser værdien til 0..1.
        if (normalized < 0.0f) normalized = 0.0f; // Klamp til 0 for at undgå tegning uden for rammen.
        if (normalized > 1.0f) normalized = 1.0f; // Klamp til 1 for at undgå tegning uden for rammen.
        int pixelY = graphY0 + graphHeight - (int)(normalized * graphHeight); // Map normaliseret værdi til Y-pixel.
        if (pointIndex > 0) {                // Tegn kun linje fra andet punkt og frem.
            tft.drawLine(previousPixelX, previousPixelY, pixelX, pixelY, TFT_CYAN); // Tegn linje-segment i cyan.
        }
        previousPixelX = pixelX;             // Gem X til næste segment.
        previousPixelY = pixelY;             // Gem Y til næste segment.
    }
}

// Nulstiller trend-bufferen, så næste graf starter forfra (bruges ved mode-/gear-skift).
// Kaldes under trendMutex.
void resetTrendGraph() {
    trendBufferInitialized = false;          // Marker at bufferen skal initialiseres igen.
    trendWriteIndex = 0;                     // Nulstil skriveposition.
}

// =========================================================================
// SEKTION 10: FILNAVNS-VALIDERING TIL MMEM-KOMMANDOER — REQ-SCPI-12.02
// =========================================================================

// Hjælpefunktion: tjekker om et filnavn er sikkert at bruge (ingen "..", ingen "/" i starten).
bool isSafeScpiFileName(const String &candidateName) {
    if (candidateName.length() == 0) return false;         // Tomt navn er ugyldigt.
    if (candidateName.length() > 64) return false;         // For langt navn er ugyldigt.
    if (candidateName.indexOf("..") >= 0) return false;    // Afvis path-traversal.
    if (candidateName.indexOf('/') >= 0) return false;     // Afvis undermapper.
    if (candidateName.indexOf('\\') >= 0) return false;    // Afvis Windows-stier.
    return true;                                            // Navnet er sikkert.
}

// =========================================================================
// SEKTION 11: SCPI-KOMMANDOPARSER — REQ-SCPI-12.01/12.02/12.03 / REQ-COM-8.01
// =========================================================================

// -------------------------------------------------------------------------
// 11.2 FUNKTION: handleScpiCommand(String commandString, const String &originalCommandString)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Dette er firmwarens SCPI-parser — den eneste funktion, der fortolker
//   kommandoer fra PC'en over USB-CDC og sender svar tilbage. Funktionen
//   modtager én komplet kommando-linje (uden '\n'), normaliserer den,
//   og dispatcher til den relevante handling via en lang if/else-if-kæde.
//
//   Funktionen håndterer følgende kommando-grupper:
//
//     IEEE 488.2 obligatoriske:
//       *IDN?  — Identifikation ("DMM-MASTER,16SRMS,SN1,V6.0").
//       *RST   — Reset til DCV 1000V safe state.
//
//     Målelæsning:
//       READ?           — Seneste formaterede måling.
//       MEAS:VOLT:DC?   — Alias for READ? (bagudkompatibilitet).
//
//     Remote-kontrol:
//       SYST:REM — Sæt i Remote mode (sluk GUI-opdateringer).
//       SYST:LOC — Forlad Remote mode (genaktivér GUI).
//
//     Måleopsætning (REQ-SCPI-12.01):
//       CONF:VOLT:DC [<range>]  — DCV, range 0..4 eller auto.
//       CONF:VOLT:AC [<range>]  — ACV, range 0..4 eller auto.
//       CONF:CURR:DC [<range>]  — DCI, range 0..5 eller auto.
//       CONF:CURR:AC [<range>]  — ACI, range 0..5 eller auto.
//       CONF:RES [<range>]      — 2W Ohm, range 0..5 eller auto.
//       CONF:FRES [<range>]     — 4W Ohm, range 0..5 eller auto.
//       Hver CONF-kommando understøtter også "?" som query, der
//       returnerer det aktuelle gear-indeks.
//
//     Graf-mode (REQ-UI-14.03):
//       DISP:GRAPH ON  — Aktivér trend-graf.
//       DISP:GRAPH OFF — Deaktivér trend-graf.
//
//     Fil-håndtering (REQ-SCPI-12.02):
//       MMEM:CAT?              — List filer og størrelser.
//       MMEM:DATA? "<fil>"     — Dump fil-indhold over USB.
//       MMEM:DEL "<fil>"       — Slet fil.
//
//     Kalibrering (REQ-CAL-13.03):
//       CAL:VAL <reference>    — Span-kalibrering (ny gain).
//       CAL:ZERO               — Zero-kalibrering (nyt offset).
//
//   Ukendte kommandoer ignoreres lydløst (buglist punkt 7-rettelse).
//
//   Funktionen returnerer intet selv — den skriver svaret direkte til
//   Serial. Ved succes svares typisk "OK"; ved fejl "ERROR"; ved målinger
//   selve værdien; ved overload "9.9E+37"; ved stale "NAN".
//
// BEGRUNDELSE (HVORFOR):
//   SCPI (Standard Commands for Programmable Instruments) er den
//   industri-standard, som professionelle måleinstrumenter bruger til
//   at blive styret fra PC, LabVIEW, Python, MATLAB osv. Uden denne
//   funktion ville DMM-MASTER være et "lukkket" instrument, der kun
//   kunne bruges manuelt via skærmen — hvilket ville gøre det uegnet
//   til automatiserede testopstillinger og dataindsamling.
//
//   Funktionen er bevidst implementeret som en lang if/else-if-kæde
//   frem for en tabel eller et regelmotor-baseret system, fordi:
//     * Det er let at læse og følge for en udvikler, der ikke kender
//       SCPI i forvejen (jf. REQ-DOC-1.02 og REQ-DOC-1.03).
//     * Det kræver ingen ekstra biblioteker.
//     * Det er let at tilføje nye kommandoer (bare en ny else-if).
//     * Det er let at fejlfinde (man kan se præcis hvilken gren, der
//       blev taget).
//
//   Ulempen er, at funktionen er lang (~250 linjer), men det er en
//   bevidst afvejning — læsbarhed og enkelhed er vigtigere end
//   "elegance" i dette projekt.
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   handleScpiCommand() kaldes UDELUKKENDE fra Task_GUI_SCPI i Sektion 14
//   — specifikt i SCPI-polling-blokken (trin 3):
//
//     Serial.setTimeout(SCPI_READ_TIMEOUT_MS);
//     while (Serial.available() > 0) {
//         String originalCommand = Serial.readStringUntil('\n');
//         handleScpiCommand(originalCommand, originalCommand);
//     }
//
//   Her kaldes funktionen for hver komplet linje, der modtages over USB.
//   Bemærk, at både commandString og originalCommandString er den SAMME
//   streng ved kaldet — funktionen laver selv en uppercase-kopi internt
//   (commandString.toUpperCase()), men bevarer originalCommandString
//   urørt til brug for filnavne-udtræk.
//
//   Funktionen kaldes ALDRIG:
//     * Fra Task_Metrology (den må ikke tale med Serial — det er Core 0's
//       ansvar).
//     * Fra ISR'en (Serial er ikke IRAM-sikkert).
//     * Fra setup() (SCPI aktiveres først, når RTOS-tasks starter).
//
//   VIGTIGT — BLOKERENDE KALD:
//   Funktionen indeholder flere blokerende kald:
//     * xSemaphoreTake(..., portMAX_DELAY) — kan blokere, indtil mutex
//       er ledig.
//     * selectRelayGear() — indeholder vTaskDelay(150 ms).
//     * vTaskDelay(pdMS_TO_TICKS(1)) i MMEM:DATA? — giver GUI'en luft.
//   Derfor kaldes funktionen KUN fra Task_GUI_SCPI (lav prioritet), så
//   Task_Metrology (høj prioritet) kan fortsætte uforstyrret.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-SCPI-12.01] MÅLEOPSÆTNING (CONFIGURATION)
//     Kravet specificerer: CONF:VOLT:DC [<range>], CONF:RES [<range>],
//     CONF:FRES [<range>] — og at range sætter gearet manuelt og
//     deaktiverer auto-ranging.
//     OPFYLDELSE: Funktionen implementerer alle seks CONF-kommandoer
//     (DCV, ACV, DCI, ACI, RES, FRES). Ved manuel range sættes
//     autoRangeEnabled = false og selectRelayGear() kaldes. Kravet
//     nævner kun DCV/RES/FRES, men firmwaren understøtter også ACV,
//     DCI og ACI (buglist D-rettelse) — en udvidelse ud over kravet.
//
//   * [REQ-SCPI-12.02] FIL- OG HUKOMMELSESHÅNDTERING
//     Kravet specificerer: MMEM:CAT?, MMEM:DATA? "log.csv", MMEM:DEL
//     "log.csv".
//     OPFYLDELSE: Alle tre kommandoer er implementeret. MMEM:DATA? og
//     MMEM:DEL bruger originalCommandString (ikke uppercase) til at
//     udtrække filnavnet, så case bevares — og validerer via
//     isSafeScpiFileName() mod path-traversal (Sektion 10).
//
//   * [REQ-SCPI-12.03] UDLÆSNING OG KONTROL
//     Kravet specificerer: SYST:REM (deaktiverer TFT-opdatering),
//     READ? (returnerer seneste måling på under 5 ms).
//     OPFYLDELSE: SYST:REM sætter isRemoteMode = true, hvilket
//     deaktiverer GUI-opdateringer i Task_GUI_SCPI (Sektion 14).
//     READ? returnerer latestPacket.finalValue med 5 decimaler —
//     svaret sendes på under 5 ms, fordi funktionen ikke blokerer
//     (kun Serial.println).
//
//   * [REQ-COM-8.01] SCPI OVERVÅGNING
//     Kravet specificerer eksempler på *IDN?, MEAS:VOLT:DC? og *RST.
//     OPFYLDELSE: Alle tre er implementeret (*IDN? svarer korrekt,
//     MEAS:VOLT:DC? er alias for READ?, *RST sætter remoteRescueRequested
//     og autoRangeEnabled).
//
//   * [REQ-CAL-13.03] KALIBRERINGSPROCEDURE (KODENIVEAU)
//     Kravet specificerer: Zero-punkt (CAL:ZERO) og Span-punkt
//     (CAL:VAL 5.0) — og at MCU'en gemmer resultatet.
//     OPFYLDELSE: Begge kommandoer er implementeret. De bruger
//     snapshot.calibrationSlot (fra MeasurementPacket) til at sikre,
//     at kalibreringen rammer det slot, der aktuelt måles — og kalder
//     saveCalibrationToNvs() for at gøre ændringen persistent.
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE)
//     Kravet specificerer, at modeskift skal tvinge relæerne i safe
//     state, vente 100 ms, og derefter koble de nye relæer ind.
//     OPFYLDELSE: Alle CONF-kommandoer kalder selectRelayGear() under
//     relayMutex — og ved modeskift slukkes Ohm-teststrømmen og/eller
//     AC-ruten eksplicit, før det nye gear sættes. Selve 100 ms +
//     50 ms ventetiden ligger i selectRelayGear().
//
//   * [REQ-FSM-11.03] FLUSH AF DSP-BUFFER VED RELÆSKIFT
//     Kravet specificerer, at filterbufferen skal flushes efter
//     relæskift.
//     OPFYLDELSE: Ved manuel range (alle CONF-kommandoer med range)
//     sættes externalRangeChangePending = true, hvilket får
//     Task_Metrology til at flushe filterBuffer og sætte isStaleReading
//     = true, indtil bufferen er genfyldt.
//
//   * [REQ-UI-14.03] RULLENDE TREND-GRAF
//     Kravet specificerer, at grafen skal kunne aktiveres/deaktiveres.
//     OPFYLDELSE: DISP:GRAPH ON/OFF sætter graphModeActive og kalder
//     resetTrendGraph() ved aktivering.
//
//   * [REQ-ERR-16.02] LÅST RECOVERY-TILSTAND
//     Kravet specificerer, at systemet skal låses i en uendelig løkke,
//     indtil *RST modtages.
//     OPFYLDELSE: *RST sætter remoteRescueRequested = true, hvilket
//     Task_GUI_SCPI (Sektion 14) bruger til at forlade recovery-løkken.
//     Bemærk: *RST udfører IKKE selve reset her — det gør GUI-tasken,
//     fordi den ejer skærmen.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: commandString, originalCommandString, rangeToken,
//     rangeValid, requestedGear, referenceValue, snapshot, activeSlot
//     osv. er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR ER DER TO STRENGE (commandString OG originalCommandString)?
//     commandString er en kopi, der toUpperCase()'es, så kommando-sammen-
//     ligninger bliver case-insensitive ("meas?" == "MEAS?"). Men
//     filnavne i MMEM:DATA? og MMEM:DEL er case-sensitive på LittleFS
//     ("Log.csv" != "log.csv"). Derfor bevares originalCommandString
//     urørt, og filnavnet udtrækkes derfra. Uden denne adskillelse ville
//     filnavne blive ødelagt af toUpperCase().
//
//   * HVORFOR STARTER FUNKTIONEN MED trim() OG toUpperCase()?
//     Fordi PC'en kan sende kommandoer med mellemrum og blandet case
//     (f.eks. "  conf:volt:dc 5  \n"). trim() fjerner mellemrum i
//     starten og slutningen; toUpperCase() gør resten case-insensitivt.
//     Det gør parseren robust over for små variationer i PC'ens
//     kommando-format.
//
//   * HVORFOR ER DER EN TOM-STRENG-CHECK?
//     Hvis PC'en sender en tom linje (f.eks. bare '\n'), ville
//     funktionen ellers løbe gennem hele if/else-if-kæden uden at
//     matche noget. Det er spild af CPU-tid og kan forvirre debug-loggen.
//     Ved at returnere tidligt undgås dette.
//
//   * HVORFOR BRUGES startsWith() FOR CONF-KOMMANDOER?
//     Fordi CONF-kommandoer kan have en range-parameter (f.eks.
//     "CONF:VOLT:DC 3"). Hvis man brugte ==, ville kommandoen med
//     range-parameter ikke matche. startsWith() matcher både med og
//     uden range, og substring() udtrækker range-parameteren.
//
//   * HVORFOR ER DER EN "?"-QUERY FOR HVER CONF-KOMMANDO?
//     SCPI-standarden kræver, at man kan spørge om den aktuelle
//     konfiguration (f.eks. "CONF:VOLT:DC?" → returnerer gear-indeks).
//     Uden denne query ville PC'en ikke kunne vide, hvilket gear
//     instrumentet står i.
//
//   * HVORFOR SÆTTES autoRangeEnabled = false VED MANUEL RANGE?
//     Fordi brugeren eksplicit har bedt om et bestemt gear — så skal
//     auto-ranging ikke overskrive det. Det er en buglist punkt 4-
//     rettelse: den oprindelige kode satte ikke dette, hvilket betød,
//     at auto-ranging kunne skifte væk fra det gear, brugeren havde
//     valgt.
//
//   * HVORFOR SÆTTES externalRangeChangePending = true VED MANUEL RANGE?
//     Fordi Task_Metrology (Core 1) skal flushe sin filterBuffer, når
//     gearet skifter. Uden dette flag ville gamle relæstøj-data blive
//     blandet ind i det nye gennemsnit — det er en REQ-FSM-11.03-
//     rettelse.
//
//   * HVORFOR TAGES relayMutex OMKRING HVER CONF-KOMMANDO?
//     Fordi SCPI-parseren (Core 0) og Task_Metrology (Core 1) kan
//     køre samtidig, og begge kan ændre relætilstanden. Mutex'en
//     sikrer, at kun én af dem ad gangen ændrer relæerne.
//
//   * HVORFOR KALDES selectRelayGear() INDE I relayMutex?
//     Fordi selectRelayGear() ændrer relæerne — det skal være under
//     mutex. Bemærk, at selectRelayGear() selv tager relayMutex i
//     nogle tilfælde? Nej — den forudsætter, at kalderen har taget
//     mutex (se Sektion 5). Så mutex'en tages her, og selectRelayGear()
//     kaldes inden den frigives.
//
//   * HVORFOR ER DER EN "?"-QUERY FOR HVER CONF-KOMMANDO?
//     Allerede dækket ovenfor.
//
//   * HVORFOR RETURNERES "NAN" VED isStale?
//     Fordi PC'en skal kunne skelne mellem "værdien er midlertidigt
//     upålidelig" (stale) og "værdien er ægte overload" (9.9E+37).
//     Hvis man returnerede 9.9E+37 for stale, ville PC'en tro, at
//     der var en ægte overload — hvilket er forkert.
//
//   * HVORFOR RETURNERES "9.9E+37" VED isOverload?
//     Fordi det er IEEE 488.2's officielle overload-format. PC'en
//     kan parse dette og vide, at værdien er uden for måleområdet.
//
//   * HVORFOR BRUGES Serial.println(snapshot.finalValue, 5)?
//     Fordi 5 decimaler er den præcision, som 4½-cifret multimeter
//     kan give. Flere decimaler ville være falsk præcision. Færre
//     ville miste opløsning.
//
//   * HVORFOR SÆTTES isRemoteMode = true VED SYST:REM?
//     Fordi GUI-opdateringer koster CPU-tid. Ved at slukke dem kan
//     SCPI-svaret gives hurtigere — det er REQ-SCPI-12.03's krav om
//     "maksimering af målehastighed over USB".
//
//   * HVORFOR KOMMENTERES UKENDTE KOMMANDOER UDEN SVAR?
//     Fordi buglist punkt 7-rettelsen siger, at scpi-reference.md
//     kræver, at ukendte kommandoer ignoreres lydløst. Den oprindelige
//     kode svarede "ERROR:UNKNOWN", hvilket kunne forvirre PC'en, hvis
//     den sendte en kommando, som instrumentet ikke kendte — f.eks.
//     en standard-SCPI-kommando, som dette instrument ikke implementerer.
//
//   * HVORFOR BRUGES originalCommandString TIL FILNAVNE?
//     Fordi LittleFS er case-sensitiv. Hvis brugeren har gemt en fil
//     som "Data.csv" og skriver MMEM:DATA? "Data.csv", skal det
//     matche præcis. Hvis man brugte commandString (som er uppercase),
//     ville filnavnet blive "DATA.CSV", og filen ville ikke blive fundet.
//
//   * HVORFOR VALIDERES FILNAVNE MED isSafeScpiFileName()?
//     For at forhindre path-traversal-angreb. Hvis PC'en sender
//     MMEM:DEL "../calibration", kunne man slette NVS-data eller
//     andre filer uden for LittleFS-partitionen. isSafeScpiFileName()
//     afviser "..", "/" og "\" — det er en sikkerhedsforanstaltning,
//     som REQ-SCPI-12.02 kræver indirekte.
//
//   * HVORFOR BRUGES CHUNKED LÆSNING I MMEM:DATA??
//     Fordi en fil kan være op til 1 MB (LOG_FILE_MAX_SIZE). Hvis man
//     læste hele filen ind i RAM på én gang, ville det kræve 1 MB
//     RAM — hvilket ESP32'en ikke har til rådighed. Ved at læse i
//     512-byte chunks (FILE_DUMP_CHUNK_SIZE) holdes RAM-forbruget nede,
//     og vTaskDelay(1) mellem chunks giver GUI'en luft til at opdatere
//     skærmen.
//
//   * HVORFOR BRUGES snapshot = latestPacket I READ? OG CAL-KOMMANDOER?
//     Fordi latestPacket kan ændres midt i funktionen af Task_Metrology
//     (Core 1). Ved at tage en lokal kopi sikres det, at alle felter
//     (finalValue, rawVoltage, calibrationSlot) stammer fra SAMME
//     måling. Uden kopi kunne man risikere, at f.eks. calibrationSlot
//     blev ændret mellem læsning af rawVoltage og brug af slot — det
//     ville kalibrere det forkerte slot.
//
//   * HVORFOR ER DER EN EKSTRA SIKKERHEDSCHECK PÅ activeSlot?
//     Fordi snapshot.calibrationSlot er uint8_t og kan være korrupt
//     (f.eks. hvis Task_Metrology skrev midt i en opdatering). Ved at
//     tjekke activeSlot >= 0 && activeSlot < CALIBRATION_SLOT_COUNT
//     undgås out-of-bounds-adgang til calibrationTable[].
//
//   * HVORFOR BEVARES currentGain I CAL:ZERO?
//     Fordi offset skal beregnes som -(raw * gain). Hvis man brugte
//     gain = 1.0 (default), ville offset være forkert for et slot, der
//     allerede er kalibreret. Ved at bevare den aktuelle gain sikres
//     det, at offset kompenserer korrekt for den nuværende gain.
//
//   * HVORFOR KALDES saveCalibrationToNvs() EFTER mutex-frigivelse?
//     Fordi NVS-skrivning er langsom (op til 10 ms). Hvis man holdt
//     calibrationMutex så længe, ville Task_Metrology blokere i op
//     til 10 ms — hvilket er uacceptabelt. Ved at frigive mutex først
//     og derefter skrive til NVS, undgås denne blokering. Risikoen for
//     race er minimal, fordi kun én SCPI-kommando ad gangen behandles.
//
//   * SIDE-EFFEKTER:
//     - Skriver svar til Serial (USB-CDC).
//     - Ændrer currentMode, autoRangeEnabled, isRemoteMode,
//       graphModeActive, remoteRescueRequested, externalRangeChangePending
//       (alle volatile globals).
//     - Kalder selectRelayGear() (ændrer relæer og relayGearIndex).
//     - Kalder saveCalibrationToNvs() (skriver til NVS).
//     - Kalder resetTrendGraph() (nulstiller trend-buffer).
//     - Kan slette filer på LittleFS (MMEM:DEL).
//     - Kan læse filer fra LittleFS (MMEM:DATA?).
//
//   * RETURVÆRDI:
//     Ingen (void). Funktionen skriver svaret direkte til Serial.
//
//   * FORHOLDSREGEL VED TILFØJELSE AF NY KOMMANDO:
//     Hvis man tilføjer en ny SCPI-kommando, skal man:
//       - Placere den i den korrekte else-if-gren (rækkefølgen betyder
//         noget — f.eks. skal CONF:VOLT:DC komme før CONF:VOLT, hvis
//         man har en sådan).
//       - Sende et svar til Serial (typisk "OK" eller en værdi).
//       - Overveje, om der skal tages mutex (hvis kommandoen ændrer
//         delte data).
//       - Opdatere docs/scpi-reference.md.
//     Ellers vil PC'en ikke kunne bruge kommandoen.
//
//   * FORHOLDSREGEL VED ÆNDRING AF SVAR-FORMAT:
//     Hvis man ændrer svaret (f.eks. fra "OK" til "SUCCESS"), skal man
//     opdatere docs/scpi-reference.md og eventuelle Python-scripts i
//     tools/, der parser svaret. Ellers vil de fejle lydløst.
//
//   * SAMMENHÆNG MED isSafeScpiFileName():
//     Funktionen kalder isSafeScpiFileName() (Sektion 10) for at
//     validere filnavne i MMEM:DATA? og MMEM:DEL. Hvis man ændrer
//     valideringsreglerne, påvirker det begge kommandoer.
//
//   * SAMMENHÆNG MED saveCalibrationToNvs():
//     Funktionen kalder saveCalibrationToNvs() (Sektion 4) efter
//     CAL:VAL og CAL:ZERO. Hvis man ændrer NVS-lagringen, påvirker
//     det begge kalibreringskommandoer.
//
//   * SAMMENHÆNG MED selectRelayGear():
//     Funktionen kalder selectRelayGear() (Sektion 5) for alle
//     CONF-kommandoer med range. Hvis man ændrer gear-mappingen,
//     påvirker det alle CONF-kommandoer.
//
// -------------------------------------------------------------------------
void handleScpiCommand(String commandString, const String &originalCommandString) {
    commandString.trim();                // Fjern ubrugelige mellemrum i starten og slutningen.
    commandString.toUpperCase();         // Gør alt til store bogstaver (så 'meas?' og 'MEAS?' bliver ens).
    if (commandString.length() == 0) return; // Ignorer tomme kommandoer.

    if (commandString == "*IDN?") {      // SCPI standard-kommando for "Identify".
        Serial.println("DMM-MASTER,16SRMS,SN1,V6.0"); // Svar i officielt IEEE 488.2 format.
    } else if (commandString == "*RST") { // SCPI standard-kommando for "Reset".
        remoteRescueRequested = true;    // Bed GUI-tasken om at udføre selve reset (den ejer skærmen).
        autoRangeEnabled = true;         // RETTELSE (buglist punkt 4): *RST genaktiverer auto-ranging.
        Serial.println("OK");            // Bekræft overfor PC'en.
    } else if (commandString == "READ?" || commandString == "MEAS:VOLT:DC?") { // Anmodning om seneste måling.
        MeasurementPacket snapshot = latestPacket; // Tag en lokal kopi for at undgå race med Core 1.
        if (snapshot.isStale) {          // Hvis værdien er "stale" (under gearskift)...
            Serial.println("NAN");       // ...returnér NAN, så PC'en kan skelne fra ægte overload.
        } else if (snapshot.isOverload) { // Hvis det er ægte overload...
            Serial.println("9.9E+37");   // ...returnér IEEE 488.2 overload-format.
        } else {
            Serial.println(snapshot.finalValue, 5); // Ellers svar med 5 decimaler på under 5 ms (REQ-SCPI-12.03).
        }
    } else if (commandString == "SYST:REM") { // Sæt instrumentet i Remote mode.
        isRemoteMode = true;             // Sluk GUI-opdateringer for at frigive CPU-cyklusser (REQ-SCPI-12.03).
        Serial.println("OK");            // Bekræft overfor PC'en.
    } else if (commandString == "SYST:LOC") { // Forlad Remote mode.
        isRemoteMode = false;            // Genaktivér GUI-opdateringer.
        Serial.println("OK");            // Bekræft overfor PC'en.
    } else if (commandString.startsWith("CONF:VOLT:DC")) { // Skift til DCV (REQ-SCPI-12.01).
        String rangeToken = commandString.substring(12); // Uddrag range-parameter hvis angivet.
        rangeToken.trim();               // Fjern mellemrum.
        bool rangeValid = true;          // Antag at range er gyldig indtil det modsatte er bevist.
        if (rangeToken == "?") {         // Hvis det er en query (CONF:VOLT:DC?).
            Serial.println(relayGearIndex); // Returnér nuværende gear som svar.
            return;                      // Afslut uden at ændre noget.
        }
        if (rangeToken.length() == 0) {  // Uden range-parameter: auto-ranging aktiveres.
            autoRangeEnabled = true;     // RETTELSE (buglist punkt 4): Slå auto-ranging til.
            xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
            currentMode = MeasurementMode::MODE_DCV; // Sæt FSM i DCV-mode.
            digitalWrite(PIN_RELAY_OHM, LOW); // RETTELSE (REQ-FSM-11.01): Fravælg OHM-teststrøm ved modeskift.
            digitalWrite(PIN_RELAY_AC_ROUTE, LOW); // Sluk AC-ruten, da vi er i DCV-mode.
            xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
            Serial.println("OK");        // Bekræft overfor PC'en.
        } else {                         // Med range-parameter: manuelt gear.
            // RETTELSE (buglist punkt 5): Følg scpi-reference.md — <range> er GEAR-INDEKS (0-4), ikke volt.
            long requestedGear = rangeToken.toInt(); // Konverter teksten til et heltals-gear-indeks.
            if (requestedGear < 0 || requestedGear > 4) { // Afvis gear uden for 0..4.
                rangeValid = false;      // Marker range som ugyldig.
            }
            if (rangeValid) {            // Hvis range er gyldig...
                autoRangeEnabled = false; // RETTELSE (buglist punkt 4): Slå auto-ranging FRA ved manuelt gear.
                xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
                currentMode = MeasurementMode::MODE_DCV; // Sæt FSM i DCV-mode.
                digitalWrite(PIN_RELAY_OHM, LOW); // RETTELSE (REQ-FSM-11.01): Fravælg OHM-teststrøm.
                digitalWrite(PIN_RELAY_AC_ROUTE, LOW); // Sluk AC-ruten, da vi er i DCV-mode.
                externalRangeChangePending = true; // RETTELSE (REQ-FSM-11.03): Bed Core 1 om at flushe filterbuffer.
                selectRelayGear((uint8_t)requestedGear, MeasurementMode::MODE_DCV); // Sæt det ønskede gear.
                xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
                Serial.println("OK");    // Bekræft overfor PC'en.
            } else {
                Serial.println("ERROR"); // Afvis ugyldig range.
            }
        }
    } else if (commandString.startsWith("CONF:VOLT:AC")) { // RETTELSE (buglist D): Skift til ACV (REQ-SCPI-12.01 udvidet).
        String rangeToken = commandString.substring(12); // Uddrag range-parameter hvis angivet.
        rangeToken.trim();               // Fjern mellemrum.
        bool rangeValid = true;          // Antag at range er gyldig.
        if (rangeToken == "?") {         // Hvis det er en query.
            Serial.println(relayGearIndex); // Returnér nuværende gear.
            return;                      // Afslut uden at ændre noget.
        }
        if (rangeToken.length() == 0) {  // Uden range: auto-ranging.
            autoRangeEnabled = true;     // Slå auto-ranging til.
            xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
            currentMode = MeasurementMode::MODE_ACV; // Sæt FSM i ACV-mode.
            digitalWrite(PIN_RELAY_OHM, LOW); // Fravælg OHM-teststrøm ved modeskift (REQ-FSM-11.01).
            selectRelayGear(relayGearIndex, MeasurementMode::MODE_ACV); // Bevar gear, men opdater mode.
            digitalWrite(PIN_RELAY_AC_ROUTE, HIGH); // RETTELSE (buglist D): Aktivér AD8436 True RMS-ruten.
            xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
            Serial.println("OK");        // Bekræft overfor PC'en.
        } else {                         // Med range: manuelt gear (gear-indeks 0-4).
            long requestedGear = rangeToken.toInt(); // Konverter til gear-indeks.
            if (requestedGear < 0 || requestedGear > 4) { // Afvis gear uden for 0..4.
                rangeValid = false;      // Marker ugyldig.
            }
            if (rangeValid) {            // Hvis range er gyldig...
                autoRangeEnabled = false; // Slå auto-ranging FRA.
                xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
                currentMode = MeasurementMode::MODE_ACV; // Sæt FSM i ACV-mode.
                digitalWrite(PIN_RELAY_OHM, LOW); // Fravælg OHM-teststrøm (REQ-FSM-11.01).
                externalRangeChangePending = true; // Bed Core 1 om at flushe filterbuffer (REQ-FSM-11.03).
                selectRelayGear((uint8_t)requestedGear, MeasurementMode::MODE_ACV); // Sæt gearet.
                digitalWrite(PIN_RELAY_AC_ROUTE, HIGH); // Aktivér AD8436 True RMS-ruten.
                xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
                Serial.println("OK");    // Bekræft overfor PC'en.
            } else {
                Serial.println("ERROR"); // Afvis ugyldig range.
            }
        }
    } else if (commandString.startsWith("CONF:CURR:DC")) { // RETTELSE (buglist D): Skift til DCI.
        String rangeToken = commandString.substring(12); // Uddrag range-parameter.
        rangeToken.trim();               // Fjern mellemrum.
        bool rangeValid = true;          // Antag gyldig range.
        if (rangeToken == "?") {         // Hvis det er en query.
            Serial.println(relayGearIndex); // Returnér nuværende gear.
            return;                      // Afslut uden at ændre noget.
        }
        if (rangeToken.length() == 0) {  // Uden range: auto-ranging.
            autoRangeEnabled = true;     // Slå auto-ranging til.
            xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
            currentMode = MeasurementMode::MODE_DCI; // Sæt FSM i DCI-mode.
            digitalWrite(PIN_RELAY_OHM, LOW); // Fravælg OHM-teststrøm (REQ-FSM-11.01).
            digitalWrite(PIN_RELAY_AC_ROUTE, LOW); // Sluk AC-ruten, da vi er i DC-mode.
            xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
            Serial.println("OK");        // Bekræft overfor PC'en.
        } else {                         // Med range: manuelt gear (gear-indeks 0-5).
            long requestedGear = rangeToken.toInt(); // Konverter til gear-indeks.
            if (requestedGear < 0 || requestedGear > 5) { // Afvis gear uden for 0..5.
                rangeValid = false;      // Marker ugyldig.
            }
            if (rangeValid) {            // Hvis range er gyldig...
                autoRangeEnabled = false; // Slå auto-ranging FRA.
                xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
                currentMode = MeasurementMode::MODE_DCI; // Sæt FSM i DCI-mode.
                digitalWrite(PIN_RELAY_OHM, LOW); // Fravælg OHM-teststrøm (REQ-FSM-11.01).
                digitalWrite(PIN_RELAY_AC_ROUTE, LOW); // Sluk AC-ruten, da vi er i DC-mode.
                externalRangeChangePending = true; // Bed Core 1 om at flushe filterbuffer (REQ-FSM-11.03).
                selectRelayGear((uint8_t)requestedGear, MeasurementMode::MODE_DCI); // Sæt gearet.
                xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
                Serial.println("OK");    // Bekræft overfor PC'en.
            } else {
                Serial.println("ERROR"); // Afvis ugyldig range.
            }
        }
    } else if (commandString.startsWith("CONF:CURR:AC")) { // RETTELSE (buglist D): Skift til ACI.
        String rangeToken = commandString.substring(12); // Uddrag range-parameter.
        rangeToken.trim();               // Fjern mellemrum.
        bool rangeValid = true;          // Antag gyldig range.
        if (rangeToken == "?") {         // Hvis det er en query.
            Serial.println(relayGearIndex); // Returnér nuværende gear.
            return;                      // Afslut uden at ændre noget.
        }
        if (rangeToken.length() == 0) {  // Uden range: auto-ranging.
            autoRangeEnabled = true;     // Slå auto-ranging til.
            xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
            currentMode = MeasurementMode::MODE_ACI; // Sæt FSM i ACI-mode.
            digitalWrite(PIN_RELAY_OHM, LOW); // Fravælg OHM-teststrøm (REQ-FSM-11.01).
            selectRelayGear(relayGearIndex, MeasurementMode::MODE_ACI); // Bevar gear, men opdater mode.
            digitalWrite(PIN_RELAY_AC_ROUTE, HIGH); // Aktivér AD8436 True RMS-ruten.
            xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
            Serial.println("OK");        // Bekræft overfor PC'en.
        } else {                         // Med range: manuelt gear (gear-indeks 0-5).
            long requestedGear = rangeToken.toInt(); // Konverter til gear-indeks.
            if (requestedGear < 0 || requestedGear > 5) { // Afvis gear uden for 0..5.
                rangeValid = false;      // Marker ugyldig.
            }
            if (rangeValid) {            // Hvis range er gyldig...
                autoRangeEnabled = false; // Slå auto-ranging FRA.
                xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
                currentMode = MeasurementMode::MODE_ACI; // Sæt FSM i ACI-mode.
                digitalWrite(PIN_RELAY_OHM, LOW); // Fravælg OHM-teststrøm (REQ-FSM-11.01).
                externalRangeChangePending = true; // Bed Core 1 om at flushe filterbuffer (REQ-FSM-11.03).
                selectRelayGear((uint8_t)requestedGear, MeasurementMode::MODE_ACI); // Sæt gearet.
                digitalWrite(PIN_RELAY_AC_ROUTE, HIGH); // Aktivér AD8436 True RMS-ruten.
                xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
                Serial.println("OK");    // Bekræft overfor PC'en.
            } else {
                Serial.println("ERROR"); // Afvis ugyldig range.
            }
        }
    } else if (commandString.startsWith("CONF:RES")) { // Skift til 2-leder modstand (REQ-SCPI-12.01).
        String rangeToken = commandString.substring(8); // Uddrag range-parameter hvis angivet.
        rangeToken.trim();               // Fjern mellemrum.
        bool rangeValid = true;          // Antag gyldig range.
        if (rangeToken == "?") {         // Hvis det er en query.
            Serial.println(relayGearIndex); // Returnér nuværende gear.
            return;                      // Afslut uden at ændre noget.
        }
        if (rangeToken.length() == 0) {  // Uden range: auto-ranging.
            autoRangeEnabled = true;     // Slå auto-ranging til.
            xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
            currentMode = MeasurementMode::MODE_OHM_2W; // Sæt FSM i 2W Ohm-mode.
            digitalWrite(PIN_RELAY_AC_ROUTE, LOW); // Sluk AC-ruten, da vi er i Ohm-mode.
            xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
            Serial.println("OK");        // Bekræft overfor PC'en.
        } else {                         // Med range: manuelt gear (gear-indeks 0-5).
            long requestedGear = rangeToken.toInt(); // Konverter til gear-indeks.
            if (requestedGear < 0 || requestedGear > 5) { // Afvis gear uden for 0..5.
                rangeValid = false;      // Marker ugyldig.
            }
            if (rangeValid) {            // Hvis range er gyldig...
                autoRangeEnabled = false; // Slå auto-ranging FRA.
                xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
                currentMode = MeasurementMode::MODE_OHM_2W; // Sæt FSM i 2W Ohm-mode.
                digitalWrite(PIN_RELAY_AC_ROUTE, LOW); // Sluk AC-ruten, da vi er i Ohm-mode.
                externalRangeChangePending = true; // Bed Core 1 om at flushe filterbuffer (REQ-FSM-11.03).
                selectRelayGear((uint8_t)requestedGear, MeasurementMode::MODE_OHM_2W); // Sæt gearet (bevarer Ohm-teststrøm).
                xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
                Serial.println("OK");    // Bekræft overfor PC'en.
            } else {
                Serial.println("ERROR"); // Afvis ugyldig range.
            }
        }
    } else if (commandString.startsWith("CONF:FRES")) { // Skift til 4-leder Kelvin (REQ-SCPI-12.01).
        String rangeToken = commandString.substring(9); // Uddrag range-parameter hvis angivet.
        rangeToken.trim();               // Fjern mellemrum.
        bool rangeValid = true;          // Antag gyldig range.
        if (rangeToken == "?") {         // Hvis det er en query.
            Serial.println(relayGearIndex); // Returnér nuværende gear.
            return;                      // Afslut uden at ændre noget.
        }
        if (rangeToken.length() == 0) {  // Uden range: auto-ranging.
            autoRangeEnabled = true;     // Slå auto-ranging til.
            xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
            currentMode = MeasurementMode::MODE_OHM_4W; // Sæt FSM i 4W Ohm-mode.
            digitalWrite(PIN_RELAY_AC_ROUTE, LOW); // Sluk AC-ruten, da vi er i Ohm-mode.
            xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
            Serial.println("OK");        // Bekræft overfor PC'en.
        } else {                         // Med range: manuelt gear (gear-indeks 0-5).
            long requestedGear = rangeToken.toInt(); // Konverter til gear-indeks.
            if (requestedGear < 0 || requestedGear > 5) { // Afvis gear uden for 0..5.
                rangeValid = false;      // Marker ugyldig.
            }
            if (rangeValid) {            // Hvis range er gyldig...
                autoRangeEnabled = false; // Slå auto-ranging FRA.
                xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
                currentMode = MeasurementMode::MODE_OHM_4W; // Sæt FSM i 4W Ohm-mode.
                digitalWrite(PIN_RELAY_AC_ROUTE, LOW); // Sluk AC-ruten, da vi er i Ohm-mode.
                externalRangeChangePending = true; // Bed Core 1 om at flushe filterbuffer (REQ-FSM-11.03).
                selectRelayGear((uint8_t)requestedGear, MeasurementMode::MODE_OHM_4W); // Sæt gearet (bevarer Ohm-teststrøm).
                xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
                Serial.println("OK");    // Bekræft overfor PC'en.
            } else {
                Serial.println("ERROR"); // Afvis ugyldig range.
            }
        }
    } else if (commandString == "DISP:GRAPH ON") { // Aktivér trend-graf-mode (REQ-UI-14.03).
        graphModeActive = true;          // Sæt flag, så GUI-tasken tegner grafen.
        xSemaphoreTake(trendMutex, portMAX_DELAY); // Lås trend-mutex.
        resetTrendGraph();               // Start grafen forfra med friske data.
        xSemaphoreGive(trendMutex);      // Frigiv trend-mutex.
        Serial.println("OK");            // Bekræft overfor PC'en.
    } else if (commandString == "DISP:GRAPH OFF") { // Deaktivér trend-graf-mode.
        graphModeActive = false;         // Slå grafen fra, så det store tal vises igen.
        Serial.println("OK");            // Bekræft overfor PC'en.
    } else if (commandString == "MMEM:CAT?") { // List filer på LittleFS (REQ-SCPI-12.02).
        File rootDirectory = LittleFS.open("/"); // Åbn rod-mappen.
        File listEntry = rootDirectory.openNextFile(); // Åbn første fil i mappen.
        while (listEntry) {              // Løb alle filer igennem.
            String entryName = String(listEntry.name()); // Hent filnavn som String.
            if (entryName.startsWith("/")) { // Fjern ledende "/" hvis den findes (ældre core).
                entryName = entryName.substring(1); // Skær første tegn af.
            }
            Serial.print(entryName);     // Skriv filnavn uden "/".
            Serial.print(",");           // Separator.
            Serial.println(listEntry.size()); // Skriv filstørrelse i bytes.
            listEntry = rootDirectory.openNextFile(); // Gå til næste fil.
        }
        Serial.println("");              // Afslut med tom linje.
    } else if (commandString.startsWith("MMEM:DATA?")) { // Dump fil-indhold over USB (REQ-SCPI-12.02).
        // Brug ORIGINAL streng (ikke toUpperCase) til at udtrække filnavnet, så case bevares.
        int firstQuote = originalCommandString.indexOf('"'); // Find start på filnavn.
        int lastQuote = originalCommandString.lastIndexOf('"'); // Find slut på filnavn.
        if (firstQuote >= 0 && lastQuote > firstQuote) { // Tjek at begge citationstegn findes.
            String fileName = originalCommandString.substring(firstQuote + 1, lastQuote); // Uddrag filnavnet (bevarer case).
            if (isSafeScpiFileName(fileName)) { // Validér filnavnet mod path-traversal.
                File dataFile = LittleFS.open("/" + fileName, "r"); // Åbn filen i read-mode.
                if (dataFile) {          // Hvis den findes...
                    uint8_t fileChunkBuffer[FILE_DUMP_CHUNK_SIZE]; // Lokal buffer til chunks.
                    while (dataFile.available()) { // ...så læs i chunks...
                        size_t bytesRead = dataFile.read(fileChunkBuffer, FILE_DUMP_CHUNK_SIZE); // Læs én chunk.
                        if (bytesRead == 0) break; // Stop hvis intet blev læst.
                        Serial.write(fileChunkBuffer, bytesRead); // Send chunk over USB.
                        vTaskDelay(pdMS_TO_TICKS(1)); // Giv GUI-tasken luft til at opdatere skærmen.
                    }
                    dataFile.close();    // Luk filen.
                }
            }
        }
    } else if (commandString.startsWith("MMEM:DEL")) { // Slet en fil (REQ-SCPI-12.02).
        // Brug ORIGINAL streng til at udtrække filnavnet, så case bevares.
        int firstQuote = originalCommandString.indexOf('"'); // Find start på filnavn.
        int lastQuote = originalCommandString.lastIndexOf('"'); // Find slut på filnavn.
        if (firstQuote >= 0 && lastQuote > firstQuote) { // Tjek at begge citationstegn findes.
            String fileName = originalCommandString.substring(firstQuote + 1, lastQuote); // Uddrag filnavnet (bevarer case).
            if (isSafeScpiFileName(fileName)) { // Validér filnavnet mod path-traversal.
                LittleFS.remove("/" + fileName); // Slet filen.
                loggingEnabled = true;   // Genaktivér logging, da der nu er plads.
                Serial.println("OK");    // Bekræft overfor PC'en.
            } else {
                Serial.println("ERROR"); // Afvis usikkert filnavn.
            }
        }
    } else if (commandString.startsWith("CAL:VAL")) { // Span-kalibrering med kendt reference (REQ-CAL-13.03).
        String valueToken = commandString.substring(7); // Uddrag referenceværdien.
        valueToken.trim();               // Fjern mellemrum.
        double referenceValue = valueToken.toDouble(); // Konverter til kommatal.
        MeasurementPacket snapshot = latestPacket; // Tag en lokal kopi — slot og raw hører sammen.
        if (referenceValue > 0.0 && fabs(snapshot.rawVoltage) > 1e-9) { // Tjek at både reference og rå-måling er gyldige.
            int activeSlot = (int)snapshot.calibrationSlot; // Brug DENNE pakkes slot — ikke en global.
            if (activeSlot >= 0 && activeSlot < CALIBRATION_SLOT_COUNT) { // Sikkerhedscheck mod out-of-bounds.
                xSemaphoreTake(calibrationMutex, portMAX_DELAY); // Lås kalibrerings-mutex.
                calibrationTable[activeSlot].gain = referenceValue / snapshot.rawVoltage; // Ny gain = reference / rå-værdi.
                xSemaphoreGive(calibrationMutex); // Frigiv kalibrerings-mutex.
                saveCalibrationToNvs(activeSlot); // Gem den nye kalibrering i NVS.
                Serial.println("OK");    // Bekræft overfor PC'en.
            } else {
                Serial.println("ERROR"); // Afvis ugyldigt slot.
            }
        } else {
            Serial.println("ERROR");     // Afvis ugyldig reference eller nul-måling.
        }
    } else if (commandString.startsWith("CAL:ZERO")) { // Zero-kalibrering (REQ-CAL-13.03).
        MeasurementPacket snapshot = latestPacket; // Tag en lokal kopi — slot og raw hører sammen.
        int activeSlot = (int)snapshot.calibrationSlot; // Brug DENNE pakkes slot — ikke en global.
        if (activeSlot >= 0 && activeSlot < CALIBRATION_SLOT_COUNT) { // Sikkerhedscheck mod out-of-bounds.
            xSemaphoreTake(calibrationMutex, portMAX_DELAY); // Lås kalibrerings-mutex.
            double currentGain = calibrationTable[activeSlot].gain; // Bevar nuværende gain under zero-justeringen.
            calibrationTable[activeSlot].offset = -(snapshot.rawVoltage * currentGain); // Offset skal annullere rå*gain.
            xSemaphoreGive(calibrationMutex); // Frigiv kalibrerings-mutex.
            saveCalibrationToNvs(activeSlot); // Gem den nye kalibrering i NVS.
            Serial.println("OK");        // Bekræft overfor PC'en.
        } else {
            Serial.println("ERROR");     // Afvis ugyldigt slot.
        }
    } else {
        // RETTELSE (buglist punkt 7): scpi-reference.md kræver at ukendte kommandoer
        // ignoreres lydløst. Vi returnerer derfor intet svar her (tidligere: "ERROR:UNKNOWN").
    }
}

// =========================================================================
// SEKTION 12: RTOS TASK — METROLOGI OG DSP (CORE 1) — REQ-RTOS-4.01 / REQ-DSP
// =========================================================================

// -------------------------------------------------------------------------
// 12.2 RTOS TASK: Task_Metrology(void *pvParameters)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Dette er firmwarens metrologi-task — den ene af to RTOS-tasks, der
//   udgør hele firmwarens kørende system. Task_Metrology kører på Core 1
//   med høj prioritet (prioritet 3) og har ÉT ansvar: at læse ADC'en,
//   filtrere signalet, styre auto-ranging, anvende kalibrering og sende
//   færdige målepakker til Core 0 via en FreeRTOS-kø.
//
//   Tasken er en uendelig løkke (for(;;)) med 12 veldefinerede trin:
//
//     Trin 1:  Læs ADC'en på den korrekte kanal for den aktive mode.
//     Trin 2:  Konverter rå 16-bit ADC-værdi til volt.
//     Trin 3:  Parallelt AC-ripple-poll (kun i DCV-mode, hvert 100 ms).
//     Trin 4:  Eksternt gearskift-flush (fra SCPI via flag).
//     Trin 5:  Relæ blanking og buffer-flush (fyld med første friske
//              måling, ikke nuller — buglist punkt 3).
//     Trin 6:  Put måling i ringbuffer (100 samples).
//     Trin 7:  Beregn gennemsnit (DSP — 50 Hz NMRR).
//     Trin 8:  Auto-ranging hysterese (rate-limited til 200 ms).
//     Trin 9:  Vælg kalibreringsslot ud fra mode og gear.
//     Trin 10: Anvend lineær kalibrering (gain + offset).
//     Trin 11: Pak data i MeasurementPacket og send til Core 0.
//     Trin 12: vTaskDelay(10 ms) — tilstræb ~83 Hz sample-rate.
//
//   Derudover tjekker tasken i starten af hver iteration, om systemHalted
//   er sat (hardware-fejl). Hvis ja, sover den 100 ms og springer resten
//   af løkken over — det forhindrer måling under en fejltilstand.
//
// BEGRUNDELSE (HVORFOR):
//   Hvorfor en separat task på Core 1? Fordi ESP32 har to kerner, og
//   arbejdet med at sample, filtrere og kalibrere kræver:
//     * Deterministisk timing (100 samples/sekund).
//     * Høj prioritet (så GUI'en ikke stjæler CPU-tid midt i en måling).
//     * Ingen blokering på Serial, TFT eller LittleFS.
//
//   Hvis metrologien kørte på samme kerne som GUI'en, ville en langsom
//   TFT-opdatering eller en filskrivning forsinke sample-loopen — og
//   dermed ødelægge 50 Hz NMRR-filteret, som er afhængigt af en stabil
//   sample-rate. Ved at isolere metrologien på Core 1 sikres det, at
//   hver sample tages med samme interval, uanset hvad GUI'en laver.
//
//   Hvorfor 12 trin i fast rækkefølge? Fordi rækkefølgen er kritisk:
//     * ADC-læsning skal ske FØR filtrering.
//     * Filterring skal ske FØR gennemsnit.
//     * Gennemsnit skal ske FØR auto-ranging-beslutning.
//     * Auto-ranging skal ske FØR kalibreringsslot-valg (fordi slot
//       afhænger af gear).
//     * Kalibrering skal ske FØR pakning.
//     * Pakning skal ske FØR queue-send.
//   Enhver ændring i rækkefølgen kan give forkerte målinger eller
//   race-betingelser.
//
// HVAD KALDER DENNE TASK? (KALDE-GRAF):
//   Task_Metrology er en FreeRTOS-task, der startes ÉN gang fra setup()
//   i Sektion 15:
//
//     xTaskCreatePinnedToCore(
//         Task_Metrology,
//         "MetrologyTask",
//         8192,
//         NULL,
//         3,       // Prioritet 3 (høj)
//         NULL,
//         1        // Core 1
//     );
//
//   Efter start kører tasken uendeligt, indtil ESP32'en genstarter.
//   Den kaldes ALDRIG direkte fra andre funktioner — den er en selvstændig
//   RTOS-task, der styres af FreeRTOS-scheduler'en.
//
//   Tasken kalder selv følgende funktioner:
//     * adc.readADC_SingleEnded() / adc.readADC_Differential_2_3()
//       (Sektion 12, trin 1 og 3).
//     * getGearAttenuationFactor() (Sektion 7, trin 3).
//     * getUpRangeThresholdForMode() / getDownRangeThresholdForMode() /
//       getMaxGearForMode() (Sektion 7, trin 8).
//     * selectRelayGear() (Sektion 5, trin 8).
//     * resetTrendGraph() (Sektion 9, trin 8).
//     * mapModeAndGearToCalibrationSlot() (Sektion 8, trin 9).
//     * xQueueSend() (FreeRTOS, trin 11).
//     * vTaskDelay() (FreeRTOS, trin 12 og systemHalted-grenen).
//
//   Tasken læser følgende globale variable:
//     * systemHalted (volatile bool).
//     * currentMode (volatile MeasurementMode).
//     * relayGearIndex (volatile uint8_t).
//     * externalRangeChangePending (volatile bool).
//     * autoRangeEnabled (volatile bool).
//     * isStaleReading (volatile bool, skrives).
//     * calibrationTable[] (under calibrationMutex).
//     * filterBuffer[] (lokal, ikke global).
//
//   Tasken skriver følgende globale variable:
//     * isStaleReading (volatile bool).
//     * externalRangeChangePending (volatile bool).
//     * filterBufferPrimed (lokal, ikke global).
//     * latestAcRippleValue (lokal, ikke global).
//     * dataQueue (via xQueueSend).
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-RTOS-4.01] TASK ALLOKERING
//     Kravet specificerer: "Core 1 (Hardware-kernen): Får funktionen
//     Task_Metrology. Den har højeste prioritet. Den måler strøm,
//     udregner gennemsnit (DSP), kalibrerer og styrer relæerne."
//     OPFYLDELSE: Task_Metrology er præcis denne funktion. Den kører på
//     Core 1 med prioritet 3 (højeste i systemet), måler ADC, beregner
//     gennemsnit (trin 7), kalibrerer (trin 10) og styrer relæer (trin 8).
//
//   * [REQ-DSP-6.01] ADC OPSÆTNING (ADS1115)
//     Kravet specificerer: "ADC'en skal sættes op til at måle fra -4.096V
//     til +4.096V."
//     OPFYLDELSE: Tasken konverterer rå ADC-værdi med 0.000125f (trin 2),
//     hvilket svarer til GAIN_ONE (±4.096V). Selve gain-konfigurationen
//     sker i setup(), men tasken bruger den korrekte skaleringsfaktor.
//
//   * [REQ-DSP-6.02] MOVING AVERAGE FILTER (RINGBUFFER)
//     Kravet specificerer: "For at fjerne 50Hz støj (NMRR) skal vi samle
//     en række målinger og tage gennemsnittet af præcis én bølgelængde."
//     OPFYLDELSE: Trin 6 (ringbuffer) og trin 7 (gennemsnit) implementerer
//     dette. FILTER_BUFFER_SIZE = 100 samples ved ~83 Hz = ~1,2 s, hvilket
//     dækker ca. 60 perioder af 50 Hz — mere end nok til NMRR.
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE)
//     Kravet specificerer: "Når der skiftes mellem disse [modes], SKAL
//     processoren tvinge alle relæer i deres 'Safe State' (1000V range),
//     vente 100 ms, og derefter koble de nye relæer ind."
//     OPFYLDELSE: Trin 8 kalder selectRelayGear(), som indeholder
//     safe-state-sekvensen og de 100 ms + 50 ms ventetider. Tasken
//     tjekker også systemHalted i starten (katastrofe-sikring).
//
//   * [REQ-FSM-11.02] HYSTERESE-ALGORITME (ANTI-CHATTER)
//     Kravet specificerer up-range ved ≥98% af full-scale (4.000V) og
//     down-range ved ≤9% (0.360V), med massivt overlap.
//     OPFYLDELSE: Trin 8 bruger getUpRangeThresholdForMode() og
//     getDownRangeThresholdForMode() (Sektion 7) til at hente grænserne,
//     og rate-limiter til 200 ms (lastRangeCheckTime).
//
//   * [REQ-FSM-11.03] FLUSH AF DSP-BUFFER VED RELÆSKIFT
//     Kravet specificerer: "Koden SKAL ikke bare vente, men eksplicit
//     slette alle tidligere værdier i den 100-samples store filterBuffer
//     array og fylde den med målinger foretaget efter relæets settle-time."
//     OPFYLDELSE: Trin 4 (eksternt gearskift) og trin 5 (relæ blanking)
//     implementerer dette. RETTELSE (buglist punkt 3): Bufferen fyldes
//     med den FØRSTE friske måling i stedet for nuller — det eliminerer
//     den opadgående rampe, der opstod, når nul-værdier blev blandet med
//     ægte målinger i ~1,8 s efter hvert gearskift.
//
//   * [REQ-CAL-13.01] DEN MATEMATISKE MODEL
//     Kravet specificerer: "V_display = (V_raw * Gain) + Offset."
//     OPFYLDELSE: Trin 10 anvender præcis denne formel:
//       finalDisplayValue = (filteredVoltage * activeCal.gain) + activeCal.offset.
//
//   * [REQ-CAL-13.02] DATASTRUKTUR I FLASH
//     Kravet specificerer, at struct'en skal loades i RAM i setup() for
//     lynhurtig adgang under drift.
//     OPFYLDELSE: Trin 9 læser calibrationTable[] (RAM-kopi) under
//     calibrationMutex — det tager nanosekunder og slider ikke flash.
//
//   * [REQ-UI-14.02] DUAL-DISPLAY MODE
//     Kravet specificerer: "Når instrumentet er i DCV-mode, og Blok 6
//     (True RMS) stadig kører parallelt i baggrunden, SKAL der kunne
//     renders en mindre tekst i Zone 3 (bund) med 'AC Ripple: X.XXX V'."
//     OPFYLDELSE: Trin 3 poller AIN1 hvert 100 ms i DCV-mode og gemmer
//     den skalerede ripple-værdi i latestAcRippleValue, som sendes i
//     packet.acRippleValue til Core 0 (trin 11).
//
//   * [REQ-UI-14.03] RULLENDE TREND-GRAF
//     Kravet specificerer, at grafen skal nulstilles ved gearskift.
//     OPFYLDELSE: Trin 8 kalder resetTrendGraph() under trendMutex, når
//     der skiftes gear — så grafen ikke blander data fra to gear.
//
//   * [REQ-AFE-7.01] SKIFT OP (UP-RANGING)
//     Kravet specificerer: "Hvis filteredVoltage overstiger 4.000V, skal
//     systemet skifte gear."
//     OPFYLDELSE: Trin 8 beregner shouldRangeUp og kalder selectRelayGear
//     med currentGearSnapshot - 1 (lavere indeks = mere dæmpning).
//
//   * [REQ-AFE-7.02] SKIFT NED (DOWN-RANGING)
//     Kravet specificerer: "Hvis displayværdien falder under 0.500V, og
//     vi er i et højt gear, skal vi skifte ned."
//     OPFYLDELSE: Trin 8 beregner shouldRangeDown (mod 0.360V, som er
//     strengere end kravets 0.500V) og kalder selectRelayGear med
//     currentGearSnapshot + 1 (højere indeks = mindre dæmpning).
//
//   * [REQ-AFE-7.03] RELÆ BLANKING
//     Kravet specificerer: "Lige efter et relæ er skiftet, SKAL der kaldes
//     vTaskDelay(pdMS_TO_TICKS(50)); Derefter SKAL ringbufferen fyldes op."
//     OPFYLDELSE: selectRelayGear() (Sektion 5) indeholder 50 ms delay.
//     Trin 5 fylder bufferen med første friske måling.
//
//   * [REQ-ERR-16.02] LÅST RECOVERY-TILSTAND — indirekte
//     Kravet specificerer, at systemet skal låses i en uendelig løkke ved
//     fejl. OPFYLDELSE: Tasken tjekker systemHalted i starten af hver
//     iteration og sover 100 ms, hvis den er sat. Det forhindrer måling
//     under fejltilstand — selve låse-løkken er i Task_GUI_SCPI.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i tasken er kommenteret med både HVAD og
//     HVORFOR.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: filterBuffer, bufferIndex, filterBufferPrimed,
//     primingSamplesRemaining, lastRangeCheckTime, lastAcRipplePollTime,
//     latestAcRippleValue, rawAdcValue, localMode, rawVoltage,
//     filteredVoltage, calibrationSlot, activeCal, finalDisplayValue,
//     packet osv. er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR ER filterBuffer LOKAL (IKKE GLOBAL)?
//     Fordi den kun bruges af denne task. Ved at holde den lokal undgås
//     race-betingelser (ingen anden task kan læse/skrive den), og
//     compileren kan optimere bedre. Hvis den var global, skulle den
//     beskyttes af en mutex — hvilket ville være unødvendigt overhead.
//
//   * HVORFOR ER DER EN systemHalted-CHECK I STARTEN AF HVER ITERATION?
//     Fordi ISR'en kan sætte systemHalted midt i en iteration. Ved at
//     tjekke i starten af hver iteration sikres det, at tasken stopper
//     med at måle så hurtigt som muligt efter en fejl. Uden checken
//     ville tasken fortsætte med at sample og sende pakker, selvom
//     relæerne er i safe state — hvilket ville give meningsløse målinger.
//
//   * HVORFOR ER localMode EN LOKAL KOPI AF currentMode?
//     Fordi currentMode kan ændres af SCPI-parseren (Core 0) midt i en
//     iteration. Hvis man læste currentMode direkte i switch-case'en,
//     kunne man risikere, at mode skiftede mellem to reads — f.eks. at
//     man læste ADC på AIN0 (DCV), men derefter brugte MODE_ACV i
//     efterfølgende logik. Ved at tage en lokal kopi fryses moden for
//     hele iterationen.
//
//   * HVORFOR ER DER EN "default"-CASE I SWITCH?
//     Hvis currentMode er en ugyldig enum-værdi (f.eks. pga. hukommelses-
//     korruption eller en fremtidig udvidelse, der glemmer at opdatere
//     denne switch), læses AIN0 som fallback. Det er den sikreste
//     standard — den giver en måling (måske forkert), men crasher ikke.
//
//   * HVORFOR KONVERTERES MED 0.000125f?
//     Fordi ADS1115 med GAIN_ONE har en LSB på 4.096V / 32768 = 0.125 mV
//     = 0.000125 V. Ved at gange rå ADC-værdi med 0.000125f får man
//     spændingen i volt. Det er den korrekte skaleringsfaktor for
//     REQ-DSP-6.01's ±4.096V-konfiguration.
//
//   * HVORFOR ER AC-RIPPLE-POLLING BETINGET AF localMode == MODE_DCV?
//     Fordi AC-ripple kun giver mening i DCV-mode. I ACV-mode er AIN1
//     allerede den primære måling (AD8436's RMS-output), så en separat
//     ripple-måling ville være redundant. I DCI/ACI/OHM-modes er AIN1
//     ikke koblet til indgangsspændingen, så en ripple-måling ville være
//     meningsløs.
//
//   * HVORFOR ER AC-RIPPLE-POLLING RATE-LIMITED TIL 100 ms?
//     Fordi AD8436's output har en vis settling-tid (C_AVG-kondensatoren
//     skal lades op), og fordi ripple-visningen ikke behøver opdateres
//     lige så ofte som det primære tal. 100 ms = 10 Hz opdatering, hvilket
//     er rigeligt for brugerens øje.
//
//   * HVORFOR TAGES relayMutex I TRIN 8 MEN IKKE I TRIN 1?
//     Fordi trin 1 kun LÆSER ADC'en — den rører ikke relæerne. Trin 8
//     ændrer relæerne via selectRelayGear(), hvilket kræver mutex for at
//     undgå race med SCPI-parseren (Core 0). Mutex'en tages kun omkring
//     selve relæskiftet, ikke omkring hele iterationen — det minimerer
//     blokeringstiden.
//
//   * HVORFOR TAGES BÅDE relayMutex OG relaySpinlock I TRIN 8?
//     Fordi relayMutex beskytter mod race med SCPI-parseren (Core 0),
//     mens relaySpinlock beskytter mod race med ISR'en (hardwareFaultISR).
//     Mutex'en kan ikke bruges i ISR-kontekst, og spinlock'en kan ikke
//     bruges i task-kontekst (den blokerer ikke). Ved at tage begge sikres
//     det, at både SCPI-parseren og ISR'en er udelukket, mens
//     currentGearSnapshot læses.
//
//   * HVORFOR FRIGIVES SPINLOCK FØR relayMutex?
//     Fordi selectRelayGear() indeholder vTaskDelay(150 ms). Hvis
//     spinlock'en blev holdt så længe, ville ISR'en blive blokeret i
//     150 ms — hvilket er uacceptabelt for en sikkerhedsfunktion.
//     Ved at frigive spinlock først og derefter kalde selectRelayGear()
//     under mutex, undgås denne blokering. Det er buglist B-rettelsen.
//
//   * HVORFOR ER DER EN EKSTRA SIKKERHEDSCHECK PÅ calibrationSlot?
//     Fordi mapModeAndGearToCalibrationSlot() kan returnere et slot uden
//     for intervallet, hvis mode eller gear er korrupt. Ved at tjekke
//     calibrationSlot >= 0 && < CALIBRATION_SLOT_COUNT undgås out-of-
//     bounds-adgang til calibrationTable[].
//
//   * HVORFOR ER packet EN LOKAL VARIABEL?
//     Fordi den skal sendes via xQueueSend(), som kopierer den ind i
//     køen. Hvis den var global, kunne Core 0 læse den midt i en
//     opdatering — hvilket ville give inkonsistente data. Ved at gøre
//     den lokal og sende den via køen sikres det, at Core 0 altid ser
//     en komplet, konsistent pakke.
//
//   * HVORFOR SÆTTES packet.isOverload = (fabs(filteredVoltage) > 4.095f)?
//     Fordi ADS1115's fuld-skala er ±4.096V. Hvis filteredVoltage
//     overstiger 4.095V (lidt under full-scale for at give margin),
//     er ADC'en ved at overstyre, og målingen er upålidelig. fabs()
//     bruges, fordi differentielle målinger kan være negative.
//
//   * HVORFOR SÆTTES packet.isAcMode = (localMode == MODE_ACV || MODE_ACI)?
//     Fordi Core 0 skal vide, om værdien skal vises som AC eller DC.
//     Det påvirker f.eks. enheden (V vs V RMS) og om AC-ripple-linjen
//     skal vises.
//
//   * HVORFOR SÆTTES packet.timeStamp = millis()?
//     Fordi dataloggingsfilen skal have et tidsstempel pr. måling.
//     millis() returnerer antal millisekunder siden opstart — det er
//     tilstrækkeligt til at korrelere målinger med hinanden.
//
//   * HVORFOR BRUGES xQueueSend(..., 0)?
//     Fordi vi ikke vil blokere, hvis køen er fuld. Køen har plads til
//     10 pakker — hvis den er fuld, betyder det, at Core 0 ikke følger
//     med, og det er bedre at tabe en pakke end at blokere metrologien.
//     Det er en bevidst afvejning: sample-raten er vigtigere end
//     garanteret levering.
//
//   * HVORFOR vTaskDelay(10 ms) OG IKKE PRÆCIS 100 Hz?
//     Fordi ADS1115's konverteringstid (2,1 ms ved 475 SPS) plus
//     loop-overhead giver en reel loop-tid på ~12 ms. Med 10 ms delay
//     bliver det ~12 ms → ~83 Hz. For præcis 100 Hz skulle delay være
//     ~7,9 ms. Men 10 ms er tættere på kravet end den tidligere ~56 Hz
//     (buglist punkt 1-rettelse). 83 Hz giver stadig ~1,2 s filter-
//     vindue, hvilket er tilstrækkeligt til 50 Hz NMRR.
//
//   * SIDE-EFFEKTER:
//     - Læser ADC'en (I2C-trafik).
//     - Skriver isStaleReading, externalRangeChangePending (volatile).
//     - Ændrer relæer via selectRelayGear() (trin 8).
//     - Nulstiller trend-buffer via resetTrendGraph() (trin 8).
//     - Sender MeasurementPacket til dataQueue (trin 11).
//     - Sover 10 ms pr. iteration (vTaskDelay).
//
//   * RETURVÆRDI:
//     Ingen (void). RTOS-tasks returnerer aldrig — de kører uendeligt.
//
//   * FORHOLDSREGEL VED ÆNDRING AF FILTER_BUFFER_SIZE:
//     Hvis FILTER_BUFFER_SIZE ændres fra 100 (Sektion 3), skal man
//     være opmærksom på:
//       - Større buffer = mere NMRR, men langsommere respons.
//       - Mindre buffer = hurtigere respons, men dårligere NMRR.
//       - Kravet (REQ-DSP-6.02) specificerer 100 samples — en ændring
//         kræver en opdatering af kravspecifikationen.
//
//   * FORHOLDSREGEL VED ÆNDRING AF SAMPLE-RATE:
//     Hvis man ændrer vTaskDelay(10) eller ADC-dataraten, påvirker det:
//       - Filtervinduets længde (FILTER_BUFFER_SIZE / sample-rate).
//       - 50 Hz NMRR-effektiviteten.
//       - Auto-rangingens reaktionstid.
//       - Trend-grafens opløsning.
//     Test altid mod en kendt 50 Hz-kilde, før man ændrer sample-raten.
//
//   * FORHOLDSREGEL VED TILFØJELSE AF NY MODE:
//     Hvis en ny MeasurementMode tilføjes, SKAL switch-case'en i trin 1
//     have en ny case — ellers vil den falde tilbage til default (AIN0).
//     Samtidig skal getMaxGearForMode(), getUpRangeThresholdForMode(),
//     getDownRangeThresholdForMode() og mapModeAndGearToCalibrationSlot()
//     opdateres (se Sektion 7 og 8).
//
//   * FORHOLDSREGEL VED ÆNDRING AF KALIBRERINGSLOGIK:
//     Hvis trin 10 ændres (f.eks. til en ikke-lineær model), skal man
//     sikre, at:
//       - calibrationTable[] stadig har de rigtige felter.
//       - loadCalibrationFromNvs() og saveCalibrationToNvs() matcher.
//       - CAL:VAL/CAL:ZERO i Sektion 11 stadig virker.
//     Ellers vil kalibreringen fejle lydløst.
//
//   * SAMMENHÆNG MED Task_GUI_SCPI:
//     Task_Metrology og Task_GUI_SCPI deler data via dataQueue (trin 11)
//     og via volatile globals (systemHalted, currentMode, relayGearIndex,
//     externalRangeChangePending, autoRangeEnabled, isStaleReading).
//     Hvis man ændrer MeasurementPacket's felter, skal Task_GUI_SCPI
//     også opdateres — ellers vil pakken blive læst forkert.
//
//   * SAMMENHÆNG MED hardwareFaultISR:
//     ISR'en kan sætte systemHalted midt i en iteration. Tasken tjekker
//     dette i starten af hver iteration (systemHalted-grenen). Hvis
//     man ændrer ISR'ens adfærd, skal man sikre, at tasken stadig
//     reagerer korrekt på systemHalted.
//
// -------------------------------------------------------------------------
void Task_Metrology(void *pvParameters) {
    float filterBuffer[FILTER_BUFFER_SIZE];  // En kasse med 100 rum til at gemme de sidste 100 målinger (ringbuffer).
    int bufferIndex = 0;                     // Holder styr på, hvilket rum vi skal putte næste måling i.
    bool filterBufferPrimed = false;         // Flag: er bufferen fyldt med friske data efter seneste relæskift?
    int primingSamplesRemaining = 0;         // RETTELSE (buglist punkt 3): Antal friske samples der mangler før isStale kan frigives.
    unsigned long lastRangeCheckTime = 0;    // Tidsstempel for seneste hysterese-vurdering, så vi ikke skifter for ofte.
    unsigned long lastAcRipplePollTime = 0;  // Tidsstempel for seneste AC-ripple-aflæsning (REQ-UI-14.02).
    double latestAcRippleValue = 0.0;        // Seneste målte AC-ripple-værdi fra AIN1 (skaleret til indgangsniveau).

    for (;;) {                               // Denne for-løkke kører for evigt (RTOS task).
        if (systemHalted == true) {          // Hvis der er sket en katastrofe (hov-fejl), så stop med at måle.
            vTaskDelay(pdMS_TO_TICKS(100));  // Vent 100 ms og tjek igen for at spare strøm.
            filterBufferPrimed = false;      // Tving buffer-flush, når vi genstarter efter fejl.
            primingSamplesRemaining = FILTER_BUFFER_SIZE; // Kræv fuld genfyldning efter fejl.
            continue;                        // Spring resten af løkken over og start forfra.
        }

        // -- 1. LÆS HARDWAREN (ADC) --
        // RETTELSE (buglist punkt 1): ADC datarate sættes til 475 SPS i setup(), så
        // hver konvertering tager ~2,1 ms. Sammen med vTaskDelay(10 ms) giver det
        // en reel loop-tid på ~12 ms → ~83 Hz (tættere på 100 Hz end 56 Hz).
        // Bemærk: ADS1115 har ingen 100 SPS-indstilling; 475 SPS er nærmeste
        // standard over 100 SPS. NMRR-vinduet bliver ~1,2 s ved 100 samples.
        int16_t rawAdcValue = 0;             // Variabel til den rå 16-bit ADC-værdi.
        MeasurementMode localMode = currentMode; // Tag en lokal kopi, så mode ikke skifter midt i læsningen.
        switch (localMode) {                 // Vælg ADC-kanal og gain ud fra den aktive måletilstand.
            case MeasurementMode::MODE_DCV:  // DC spænding læses single-ended på AIN0.
                rawAdcValue = adc.readADC_SingleEnded(0); // Læs AIN0 mod GND (DC-spænding fra FET-buffer).
                break;                       // Afslut case.
            case MeasurementMode::MODE_ACV:  // AC spænding læses single-ended på AIN1 (AD8436 output).
                rawAdcValue = adc.readADC_SingleEnded(1); // Læs AIN1 mod GND (analog RMS-DC fra AD8436).
                break;                       // Afslut case.
            case MeasurementMode::MODE_DCI:  // DC strøm læses differentielt over shunten (AIN2/AIN3).
                rawAdcValue = adc.readADC_Differential_2_3(); // Læs differentielt AIN2-AIN3 for at fjerne stelstøj.
                break;                       // Afslut case.
            case MeasurementMode::MODE_ACI:  // AC strøm læses single-ended på AIN1 (AD8436 output).
                rawAdcValue = adc.readADC_SingleEnded(1); // Læs AIN1 mod GND (analog RMS-DC fra AD8436).
                break;                       // Afslut case.
            case MeasurementMode::MODE_OHM_2W: // 2-leder modstand læses differentielt over DUT (AIN2/AIN3).
                rawAdcValue = adc.readADC_Differential_2_3(); // Læs differentielt AIN2-AIN3 over modstanden.
                break;                       // Afslut case.
            case MeasurementMode::MODE_OHM_4W: // 4-leder Kelvin læses differentielt over Sense-linjerne.
                rawAdcValue = adc.readADC_Differential_2_3(); // Læs differentielt AIN2-AIN3 fra Sense+ / Sense-.
                break;                       // Afslut case.
            default:                         // Ukendt mode: læs AIN0 som sikker fallback.
                rawAdcValue = adc.readADC_SingleEnded(0); // Læs AIN0 som fallback.
                break;                       // Afslut case.
        }

        // -- 2. KONVERTER TIL VOLT --
        float rawVoltage = rawAdcValue * 0.000125f; // Med GAIN_ONE (4.096V) er hver enhed = 0.125 mV.

        // -- 3. PARALLEL AC-RIPPLE-MÅLING (REQ-UI-14.02) —
        // Når vi er i DCV-mode, aflæses AIN1 (AD8436 output) ca. hvert 100 ms for at vise AC-ripple i Zone 3.
        // RETTELSE (buglist punkt 6): Ripple-værdien skaleres nu med gearets dæmpningsfaktor,
        // så 60V/600V-gear ikke viser 10×/100× for lave ripple-værdier.
        unsigned long acNow = millis();       // Læs aktuelt tidspunkt for ripple-polling.
        if (localMode == MeasurementMode::MODE_DCV && (acNow - lastAcRipplePollTime) >= 100) { // Hvis DCV og 100 ms er gået...
            lastAcRipplePollTime = acNow;     // Gem tidspunktet for denne ripple-aflæsning.
            int16_t rippleRawAdc = adc.readADC_SingleEnded(1); // Læs AIN1 (AD8436 RMS-DC output).
            double rippleAdcVoltage = (double)rippleRawAdc * 0.000125; // Konverter rå ADC til volt ved ADC-indgangen.
            float rippleAttenuation = getGearAttenuationFactor(relayGearIndex); // Hent gearets dæmpningsfaktor.
            latestAcRippleValue = rippleAdcVoltage * rippleAttenuation; // Skalér tilbage til indgangsniveau.
        }

        // -- 4. EKSTERNT GEARSKIFT-FLUSH (REQ-FSM-11.03) —
        // RETTELSE: Hvis SCPI har ændret gear (externalRangeChangePending), skal Core 1
        // flushe filterbufferen, så gamle relæstøj-data ikke blandes ind i det nye gennemsnit.
        if (externalRangeChangePending) {    // Hvis et eksternt gearskift venter...
            externalRangeChangePending = false; // Nulstil flaget (vi har set det).
            filterBufferPrimed = false;      // Tving buffer-flush, så næste måling starter rent.
            primingSamplesRemaining = FILTER_BUFFER_SIZE; // RETTELSE (buglist punkt 3): Kræv fuld genfyldning.
            isStaleReading = true;           // RETTELSE (buglist punkt 3): Hold isStale til bufferen er fyldt.
        }

        // -- 5. RELÆ BLANKING OG BUFFER FLUSH (REQ-FSM-11.03) --
        // RETTELSE (buglist punkt 3): Bufferen fyldes nu med den FØRSTE friske måling
        // i stedet for nuller. Det eliminerer den opadgående rampe, der opstod når
        // nul-værdier blev blandet med ægte målinger i ~1,8 s efter hvert gearskift.
        if (filterBufferPrimed == false) {   // Hvis bufferen ikke er fyldt med friske data efter et relæskift...
            for (int primeIndex = 0; primeIndex < FILTER_BUFFER_SIZE; primeIndex++) { // ...så fyld hele bufferen...
                filterBuffer[primeIndex] = rawVoltage; // ...med den FØRSTE friske måling (ikke nul).
            }
            bufferIndex = 0;                 // Nulstil ringbufferen til start.
            filterBufferPrimed = true;       // Marker at bufferen nu er ren og klar.
            primingSamplesRemaining = FILTER_BUFFER_SIZE; // Kræv fuld genfyldning med friske samples før isStale frigives.
        }

        // -- 6. PUT I FILTER (Ringbuffer til 50Hz NMRR undertrykkelse) --
        filterBuffer[bufferIndex] = rawVoltage; // Gem målingen i det aktuelle rum.
        bufferIndex = bufferIndex + 1;       // Ryk en plads frem i kassen.
        if (bufferIndex >= FILTER_BUFFER_SIZE) { // Hvis vi er nået til enden af kassen...
            bufferIndex = 0;                 // ...så start forfra og overskriv de ældste data (ringbuffer).
        }
        if (primingSamplesRemaining > 0) {   // RETTELSE (buglist punkt 3): Tæl ned mod fuld genfyldning.
            primingSamplesRemaining--;       // Ét frisk sample mere i bufferen.
            if (primingSamplesRemaining == 0) { // Når bufferen er fuldt genfyldt...
                isStaleReading = false;      // ...frigiv isStale, så SCPI/display viser friske værdier.
            }
        }

        // -- 7. UDREGN GENNEMSNIT (DSP) --
        float sum = 0.0f;                    // En variabel til at holde den samlede værdi.
        for (int averageIndex = 0; averageIndex < FILTER_BUFFER_SIZE; averageIndex++) { // Gennemgå alle 100 rum.
            sum = sum + filterBuffer[averageIndex]; // Læg værdien oveni den totale sum.
        }
        float filteredVoltage = sum / (float)FILTER_BUFFER_SIZE; // Divider med 100 for det perfekte gennemsnit.

        // -- 8. AUTO-RANGING HYSTERESE (REQ-FSM-11.02) — RETTET RETNING (buglist A) —
        // Gear-indeks 0 = 1000V (mest dæmpning), gear 4 = 600mV (mindst dæmpning).
        // Up-range (mere dæmpning) betyder LAVERE indeks; down-range betyder HØJERE indeks.
        // RETTELSE (buglist B): selectRelayGear() kaldes nu UDEN FOR den kritiske sektion,
        // fordi den indeholder vTaskDelay() (blokerende kald er forbudt i kritisk sektion).
        // RETTELSE (buglist punkt 4): Auto-ranging springes helt over når autoRangeEnabled er false.
        unsigned long now = millis();        // Læs aktuelt tidspunkt for at rate-limit range-skift.
        if (autoRangeEnabled && (now - lastRangeCheckTime > 200)) { // Kun hvis auto-ranging er aktiv og 200 ms gået.
            lastRangeCheckTime = now;        // Gem tidspunktet for denne vurdering.
            float upThreshold = getUpRangeThresholdForMode(localMode);   // Hent mode-specifik up-range-grænse.
            float downThreshold = getDownRangeThresholdForMode(localMode); // Hent mode-specifik down-range-grænse.
            uint8_t maxGear = getMaxGearForMode(localMode);               // Hent mode-specifikt max gear.
            xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
            portENTER_CRITICAL(&relaySpinlock); // Tag spinlock, så ISR ikke kolliderer.
            uint8_t currentGearSnapshot = relayGearIndex; // Læs gear til lokal variabel inde i kritisk sektion.
            // RETTELSE (buglist A): Up-range = LAVERE indeks (mere dæmpning), down-range = HØJERE indeks.
            bool shouldRangeUp = (filteredVoltage >= upThreshold) && (currentGearSnapshot > 0); // Up-range hvis over grænse (indeks ned mod 0).
            bool shouldRangeDown = (filteredVoltage <= downThreshold) && (currentGearSnapshot < maxGear); // Down-range hvis under grænse (indeks op mod max).
            portEXIT_CRITICAL(&relaySpinlock); // RETTELSE (buglist B): Frigiv spinlock FØR relæskift.
            if (shouldRangeUp || shouldRangeDown) { // Hvis vi skal skifte gear...
                isStaleReading = true;       // Marker at værdien er "stale" under gearskift.
                if (shouldRangeUp) {         // Hvis vi skal skifte op til mere dæmpning...
                    selectRelayGear(currentGearSnapshot - 1, localMode); // ...skift til lavere indeks (mere dæmpning).
                } else {                     // Ellers, hvis vi skal skifte ned for mere præcision...
                    selectRelayGear(currentGearSnapshot + 1, localMode); // ...skift til højere indeks (mindre dæmpning).
                }
                filterBufferPrimed = false;  // Tving buffer-flush, så relæstøj ikke forurener gennemsnittet.
                primingSamplesRemaining = FILTER_BUFFER_SIZE; // RETTELSE (buglist punkt 3): Kræv fuld genfyldning.
                xSemaphoreTake(trendMutex, portMAX_DELAY); // Lås trend-mutex.
                resetTrendGraph();           // Nulstil trend-grafen, så den ikke blander gear.
                xSemaphoreGive(trendMutex);  // Frigiv trend-mutex.
                // RETTELSE (buglist punkt 3): isStaleReading frigives IKKE her — det sker
                // først når primingSamplesRemaining når 0 i blok 6 ovenfor.
            }
            xSemaphoreGive(relayMutex);      // RETTELSE (buglist B): Frigiv relæ-mutex efter relæskift.
        }

        // -- 9. VÆLG KALIBRERINGSSLOT UD FRA MODE OG GEAR --
        int calibrationSlot = mapModeAndGearToCalibrationSlot(localMode, relayGearIndex); // Map (mode, gear) → slot.
        if (calibrationSlot < 0 || calibrationSlot >= CALIBRATION_SLOT_COUNT) { // Sikkerhedscheck mod out-of-bounds.
            calibrationSlot = 0;             // Fallback til slot 0 hvis noget gik galt.
        }
        xSemaphoreTake(calibrationMutex, portMAX_DELAY); // Lås kalibrerings-mutex.
        CalibrationData activeCal = calibrationTable[calibrationSlot]; // Hent de aktive kalibreringskonstanter.
        xSemaphoreGive(calibrationMutex); // Frigiv kalibrerings-mutex.

        // -- 10. ANVEND LINEÆR KALIBRERING (REQ-CAL-13.01) --
        double finalDisplayValue = ((double)filteredVoltage * activeCal.gain) + activeCal.offset; // V_display = (V_raw * Gain) + Offset.

        // -- 11. PAK DATA OG SEND TIL CORE 0 --
        MeasurementPacket packet;            // Opretter en ny forsendelsespakke.
        packet.finalValue = finalDisplayValue; // Gemmer vores beregnede værdi i pakken.
        packet.rawVoltage = (double)filteredVoltage; // Gem RÅ filtreret spænding, så CAL:VAL kan bruge den.
        packet.acRippleValue = latestAcRippleValue; // Gem seneste AC-ripple-måling (skaleret) til Zone 3.
        packet.currentRange = relayGearIndex; // Fortæller Core 0 hvilket gear vi aflæser i.
        packet.calibrationSlot = (uint8_t)calibrationSlot; // Gem DENNE pakkes slot.
        packet.isOverload = (fabs(filteredVoltage) > 4.095f); // Advarer hvis chippen overstyres (overload) — fabs for differentielle målinger.
        packet.isAcMode = (localMode == MeasurementMode::MODE_ACV) || (localMode == MeasurementMode::MODE_ACI); // Marker AC-mode.
        packet.isStale = isStaleReading;     // Marker om værdien er "stale" (holdes true til bufferen er genfyldt).
        packet.timeStamp = millis();         // Noterer tidsstemplet til fil-loggen.
        xQueueSend(dataQueue, &packet, 0);   // Sender pakken sikkert gennem røret (Queue) med maks 0 ventetid.

        // -- 12. TILSTRÆB 100 HERTZ —
        // RETTELSE (buglist punkt 1): ADC er sat til 475 SPS i setup(), så hver
        // konvertering tager ~2,1 ms. Denne delay på 10 ms giver en reel loop-tid
        // på ~12 ms → ~83 Hz. For præcis 100 Hz skal delay reduceres til ~7,9 ms,
        // men 10 ms er tættere på kravet end de tidligere ~56 Hz.
        vTaskDelay(pdMS_TO_TICKS(10));       // Sov præcis 10 ms, så løkken kører ~83 gange i sekundet.
    }
}

// =========================================================================
// SEKTION 13: LOG-ROTATION HJÆLPEFUNKTION — REQ-MEM-15.02
// =========================================================================

// -------------------------------------------------------------------------
// 13.2 FUNKTION: ensureLogFileHasRoom(uint32_t &currentLogFileIndex, String &outLogFileName)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Sikrer, at der findes en log-fil på LittleFS, som har plads til
//   mindst én mere skriveblok (100 målinger). Funktionen er ansvarlig
//   for HELE log-rotationen — den finder den første fil i rækken
//   log.csv, log_002.csv, log_003.csv, ... som enten ikke findes endnu
//   eller er under LOG_FILE_MAX_SIZE (1 MB).
//
//   Funktionen returnerer:
//     * true  — hvis der blev fundet en fil med plads. Filnavnet skrives
//               til outLogFileName, og currentLogFileIndex opdateres,
//               hvis der blev roteret til en ny fil.
//     * false — hvis alle 100 rotationsforsøg mislykkedes (dvs. alle
//               filer er fyldte, eller LittleFS kan ikke åbnes).
//
//   Funktionen tager to parametre, begge som reference:
//     * uint32_t &currentLogFileIndex — indeks for nuværende logfil.
//       Funktionen LÆSER den for at vide, hvor den skal starte, og
//       SKRIVER den, hvis der roteres til en ny fil.
//     * String &outLogFileName — modtager det fundne filnavn. Funktionen
//       SKRIVER til den, så kalderen kan åbne filen i append-mode.
//
//   Funktionen er bevidst adskilt fra selve skrive-operationen. Den
//   "reserverer" en fil med plads; kalderen åbner og skriver til den
//   bagefter. Det giver en ren adskillelse mellem "find en fil" og
//   "skriv til filen".
//
// BEGRUNDELSE (HVORFOR):
//   LittleFS-partitionen på ESP32'en er typisk 2 MB (jf. REQ-ENV-2.04's
//   "No OTA (2MB APP/2MB SPIFFS)" partition scheme). Hvis man blot
//   skrev til én fil uden grænse, ville partitionen løbe fuld efter
//   ca. 200 sekunder ved 100 målinger/sekund og ~10 bytes/måling — og
//   derefter ville alle skrivninger fejle lydløst, og brugeren ville
//   ikke vide hvorfor.
//
//   Log-rotation løser dette ved at:
//     1) Begrænse hver fil til LOG_FILE_MAX_SIZE (1 MB).
//     2) Oprette en ny fil (log_002.csv, log_003.csv, ...) når den
//        nuværende er fuld.
//     3) Give brugeren mulighed for at downloade og slette gamle filer
//        via SCPI (MMEM:DATA? / MMEM:DEL) for at frigøre plads.
//
//   Uden denne funktion ville datalogging være ubrugelig i længere
//   tid ad gangen — hvilket er kernen i REQ-MEM-15.02's krav om
//   "log-rotation og størrelseskontrol".
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   ensureLogFileHasRoom() kaldes UDELUKKENDE fra Task_GUI_SCPI i
//   Sektion 14 — specifikt i datalogging-blokken (trin 6), når
//   logBuffer er fuld og skal skrives til flash:
//
//     String logFileName;
//     if (ensureLogFileHasRoom(currentLogFileIndex, logFileName)) {
//         File logFile = LittleFS.open(logFileName.c_str(), "a");
//         if (logFile) {
//             logFile.print(logBuffer);
//             logFile.close();
//         } else {
//             loggingEnabled = false;
//         }
//     } else {
//         loggingEnabled = false;
//     }
//
//   Her bruges resultatet til at afgøre, om der kan skrives til en
//   fil, eller om logging skal stoppes (loggingEnabled = false), så
//   GUI'en kan vise "LOG FULL".
//
//   Funktionen kaldes ALDRIG:
//     * Fra Task_Metrology (den må ikke tale med LittleFS — det er
//       Core 0's ansvar).
//     * Fra SCPI-parseren (den bruger MMEM:DEL til at frigøre plads
//       manuelt, men kalder ikke denne funktion).
//     * Fra setup() (logging starter først, når RTOS-tasks kører).
//     * Fra ISR'en (LittleFS er ikke IRAM-sikkert).
//
//   Funktionen er en ren "getter/setter" for filnavn og indeks — den
//   læser kun LittleFS-metadata (filstørrelse), ikke filindhold. Den
//   har ingen side-effekter på andre globale variable end de to
//   parametre.
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-MEM-15.02] LOG-ROTATION OG STØRRELSESKONTROL
//     Kravet specificerer: "Inden hver skriveoperation skal filstørrelsen
//     kontrolleres med logFile.size(). Overstiger filen 1 MB, SKAL
//     instrumentet enten automatisk oprette en ny fil (f.eks. log_002.csv)
//     for at bevare data, eller den ældste data skal roteres ud."
//     OPFYLDELSE: ensureLogFileHasRoom() er præcis den funktion, kravet
//     beskriver. Den:
//       - Tjekker filstørrelse med logFile.size().
//       - Roterer til en ny fil (log_002.csv, log_003.csv, ...), når
//         den nuværende overstiger LOG_FILE_MAX_SIZE (1 MB).
//       - Bevarer alle data (ingen sletning — kun ny fil).
//     Uden denne funktion ville REQ-MEM-15.02 ikke være opfyldt.
//
//   * [REQ-MEM-15.01] SKRIVEBUFFER (WEAR LEVELING) — indirekte
//     Kravet specificerer, at der skal samles 100 målinger i RAM, før
//     der skrives til flash (for at minimere slid).
//     OPFYLDELSE: Funktionen er den "modtager-side", der finder den
//     fil, som den akkumulerede buffer skal skrives til. Uden den
//     ville bulk-skrivningen ikke vide, hvilken fil der skulle bruges.
//
//   * [REQ-SCPI-12.02] FIL- OG HUKOMMELSESHÅNDTERING — indirekte
//     Kravet specificerer, at MMEM:CAT? / MMEM:DATA? / MMEM:DEL skal
//     kunne håndtere filer på LittleFS.
//     OPFYLDELSE: Funktionen opretter de filer (log_002.csv osv.),
//     som MMEM-kommandoerne skal kunne liste, downloade og slette.
//     Uden funktionen ville der aldrig blive oprettet roterede filer,
//     og MMEM-kommandoerne ville kun se log.csv.
//
//   * [REQ-ERR-16.02] LÅST RECOVERY-TILSTAND — indirekte
//     Kravet specificerer, at systemet skal kunne fortsætte logging
//     efter en fejl (når *RST er modtaget).
//     OPFYLDELSE: Hvis loggingEnabled blev sat til false pga. fuld
//     partition, kan MMEM:DEL frigøre plads og genaktivere logging.
//     Funktionen er den, der finder den nye fil, når logging genoptages.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i funktionen er kommenteret med både HVAD
//     og HVORFOR.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: currentLogFileIndex, outLogFileName, logFile,
//     fileSize, attempt, LOG_FILE_MAX_SIZE osv. er alle beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR ER DER EN ØVRE GRÆNSE PÅ 100 ROTATIONER?
//     Fordi hvis LittleFS er korrupt eller fuld på en måde, der gør,
//     at filstørrelser læses forkert, kunne funktionen ellers loope
//     uendeligt (currentLogFileIndex++ i det uendelige). 100 rotationer
//     svarer til 100 MB log-data — langt mere, end partitionen kan
//     rumme. Hvis 100 rotationer ikke finder plads, er noget galt,
//     og det er bedre at give op end at loope evigt.
//
//   * HVORFOR ER currentLogFileIndex EN REFERENCE (uint32_t &)?
//     Fordi funktionen skal kunne opdatere indekset, når den roterer
//     til en ny fil. Hvis den var en værdi (uint32_t), ville kalderen
//     ikke se ændringen, og næste kald ville starte fra det gamle
//     indeks — hvilket ville føre til, at man roterede igen og igen
//     til den samme fil. Referencen sikrer, at rotationen "hænger
//     ved" mellem kald.
//
//   * HVORFOR ER outLogFileName EN REFERENCE (String &)?
//     Fordi funktionen skal kunne returnere det fundne filnavn til
//     kalderen. Hvis den var en værdi, ville kalderen ikke se navnet.
//     Referencen er også mere effektiv end at returnere en String
//     (ingen kopiering).
//
//   * HVORFOR RETURNERES true, HVIS FILEN IKKE FINDES?
//     Fordi en ikke-eksisterende fil pr. definition er tom — dvs. den
//     har masser af plads. Ved at returnere true undgås det, at
//     funktionen fejler på første kald (hvor log.csv endnu ikke er
//     oprettet).
//
//   * HVORFOR ÅBNES FILEN I "r" (READ) MODE OG IKKE "a" (APPEND)?
//     Fordi vi kun skal LÆSE filstørrelsen — ikke skrive endnu. Ved at
//     åbne i read-mode undgås det, at filen oprettes utilsigtet, hvis
//     den ikke findes. Det gør også koden mere læsbar: "vi tjekker,
//     om der er plads, før vi skriver".
//
//   * HVORFOR LUKKES FILEN MED logFile.close() FØR STØRRELSES-TJEKKET?
//     Faktisk lukkes den EFTER tjekket — rækkefølgen er:
//       1) Åbn fil i read-mode.
//       2) Læs fileSize.
//       3) Luk filen.
//       4) Tjek fileSize < LOG_FILE_MAX_SIZE.
//     Ved at lukke FØR tjekket undgås det, at filen holdes åben,
//     hvis vi beslutter at rotere til en ny fil. Det frigiver en
//     filhåndtag, som LittleFS har brug for til næste åbning.
//
//   * HVORFOR ER DER EN "if (currentLogFileIndex <= 1)"-CHECK?
//     Fordi den første fil hedder "log.csv" (uden indeks). Efter
//     rotation hedder filerne "log_002.csv", "log_003.csv" osv. —
//     dvs. indeks 2, 3, 4... Der er bevidst ikke en "log_001.csv",
//     fordi "log.csv" er den første. Checken sikrer, at første kald
//     bruger "log.csv", og at efterfølgende rotationer bruger
//     "log_002.csv" osv.
//
//   * HVORFOR ER STRING(currentLogFileIndex) NØDVENDIG?
//     Fordi String-klassen ikke har en implicit konvertering fra
//     uint32_t til String i alle Arduino-versioner. Ved at kalde
//     String(currentLogFileIndex) eksplicit undgås compiler-advarsler
//     og sikres korrekt konvertering.
//
//   * HVORFOR ER DER EN currentLogFileIndex++ I SLUTNINGEN AF LØKKEN?
//     Fordi hvis den nuværende fil er fuld, skal vi prøve den NÆSTE
//     fil. Uden ++ ville løkken aldrig komme videre, og funktionen
//     ville loope 100 gange på samme fil — hvilket er spild af tid.
//
//   * HVORFOR ER DER EN outLogFileName TILDELING I HVER ITERATION?
//     Fordi outLogFileName skal indeholde det SIDSTE forsøgte filnavn,
//     når funktionen returnerer true. Hvis vi kun tildelte én gang
//     før løkken, ville outLogFileName indeholde det forkerte navn,
//     hvis der blev roteret.
//
//   * SIDE-EFFEKTER:
//     - Læser filstørrelser fra LittleFS (I/O).
//     - Opdaterer currentLogFileIndex (hvis der roteres).
//     - Skriver til outLogFileName (altid).
//     - Opretter IKKE filen — det gør kalderen med LittleFS.open(..., "a").
//
//   * RETURVÆRDI:
//     bool — true hvis der blev fundet en fil med plads; false hvis
//     alle 100 rotationsforsøg mislykkedes.
//
//   * FORHOLDSREGEL VED ÆNDRING AF LOG_FILE_MAX_SIZE:
//     Hvis LOG_FILE_MAX_SIZE ændres fra 1 MB (Sektion 3), skal man
//     være opmærksom på:
//       - Større max = færre rotationer, men længere download-tid pr.
//         fil (MMEM:DATA? tager længere tid).
//       - Mindre max = flere rotationer, men kortere download-tid.
//       - Kravet (REQ-MEM-15.02) specificerer 1 MB — en ændring kræver
//         en opdatering af kravspecifikationen.
//
//   * FORHOLDSREGEL VED ÆNDRING AF FILNAVNS-MØNSTER:
//     Hvis man ændrer filnavnene (f.eks. til "dmm_001.csv" i stedet
//     for "log.csv"), skal man opdatere:
//       - ensureLogFileHasRoom() (denne funktion).
//       - MMEM:CAT? / MMEM:DATA? / MMEM:DEL i Sektion 11 (de henviser
//         til filnavne i deres dokumentation).
//       - Python-scripts i tools/ (de forventer "log.csv").
//     Ellers vil brugeren ikke kunne finde sine logfiler.
//
//   * FORHOLDSREGEL VED ÆNDRING AF ROTATIONSGRÆNSEN (100):
//     Hvis man ændrer 100 til et andet tal, skal man overveje:
//       - For lille grænse = risiko for at give op, selvom der er
//         plads (f.eks. hvis partitionen er 2 MB og hver fil er 1 MB,
//         er der kun plads til 2 filer — 100 er mere end nok).
//       - For stor grænse = risiko for meget lang loop-tid, hvis
//         LittleFS er korrupt.
//     100 er valgt som en "sikker overgrænse" — det er 100 MB, hvilket
//     er langt mere, end partitionen kan rumme.
//
//   * SAMMENHÆNG MED Task_GUI_SCPI's logging-blok:
//     Funktionen er tæt koblet til logging-blokken i Task_GUI_SCPI
//     (Sektion 14, trin 6). Hvis man ændrer logBuffer-håndteringen,
//     skal man sikre, at:
//       - ensureLogFileHasRoom() stadig kaldes, før der skrives.
//       - loggingEnabled sættes til false, hvis funktionen returnerer
//         false.
//       - currentLogFileIndex opdateres korrekt.
//
//   * SAMMENHÆNG MED MMEM:DEL:
//     Når brugeren sletter en fil via MMEM:DEL (Sektion 11), sættes
//     loggingEnabled = true igen — men currentLogFileIndex opdateres
//     IKKE. Det betyder, at hvis brugeren sletter log_003.csv, vil
//     næste skrivning stadig gå til log_004.csv (eller hvad indekset
//     nu er). Det er bevidst: vi vil ikke genbruge slettede filnavne,
//     fordi det ville blande gammel og ny data i samme fil.
//
// -------------------------------------------------------------------------
bool ensureLogFileHasRoom(uint32_t &currentLogFileIndex, String &outLogFileName) {
    for (int attempt = 0; attempt < 100; attempt++) { // Begræns antal rotationer for at undgå uendelig løkke.
        if (currentLogFileIndex <= 1) {      // Første fil hedder bare log.csv.
            outLogFileName = "/log.csv";     // Standard logfil-navn.
        } else {                             // Efter rotation hedder filerne log_002.csv, log_003.csv, ...
            outLogFileName = "/log_" + String(currentLogFileIndex) + ".csv"; // Byg navnet med indeks.
        }
        File logFile = LittleFS.open(outLogFileName.c_str(), "r"); // Åbn i read-mode for at tjekke størrelse.
        if (!logFile) {                      // Hvis filen ikke findes endnu...
            return true;                     // ...så er der masser af plads; brug den.
        }
        size_t fileSize = logFile.size();    // Læs filens størrelse.
        logFile.close();                     // Luk igen.
        if (fileSize < LOG_FILE_MAX_SIZE) {  // Hvis filen er under max...
            return true;                     // ...så brug den.
        }
        currentLogFileIndex++;               // Ellers: gå til næste fil-indeks og prøv igen.
    }
    return false;                            // Alle 100 rotationsforsøg mislykkedes — giv op.
}

// =========================================================================
// SEKTION 14: RTOS TASK — GUI, SCPI OG DATALOGGING (CORE 0) — REQ-RTOS-4.01
// =========================================================================
// -------------------------------------------------------------------------
// 14.2 RTOS TASK: Task_GUI_SCPI(void *pvParameters)
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   Dette er firmwarens brugerflade- og kommunikations-task — den anden af
//   to RTOS-tasks, der udgør hele firmwarens kørende system. Task_GUI_SCPI
//   kører på Core 0 med lav prioritet (prioritet 1) og har ansvaret for ALT,
//   der ikke er ren metrologi:
//
//     * Tegne skærmen (stort tal, statuslinje, CAT III-advarsel,
//       AC-ripple-visning, trend-graf, log-status).
//     * Læse SCPI-kommandoer fra USB og kalde handleScpiCommand().
//     * Læse målepakker fra dataQueue (fra Core 1) og gemme dem i
//       latestPacket.
//     * Skrive dataloggings-linjer til LittleFS (via RAM-buffer og
//       log-rotation).
//     * Håndtere katastrofe-tilstand (systemHalted) med rød skærm,
//       buzzer og *RST-recovery.
//
//   Tasken er en uendelig løkke (for(;;)) med 6 veldefinerede trin:
//
//     Trin 1:  Håndtér systemHalted (katastrofe-recovery, REQ-ERR-16.01/16.02).
//     Trin 2:  Håndtér *RST uden for fejltilstand (reset til DCV 1000V).
//     Trin 3:  Poll USB for SCPI-kommandoer og kald handleScpiCommand().
//     Trin 4:  Læs målepakke fra dataQueue (non-blocking, 0 timeout).
//     Trin 5:  Opdater skærmen (anti-flicker, Zone 1/2/3, trend-graf).
//     Trin 6:  Datalogging til LittleFS (wear leveling, log-rotation).
//     Trin 7:  vTaskDelay(2 ms) — giv andre RTOS-tasks luft.
//
// BEGRUNDELSE (HVORFOR):
//   Hvorfor en separat task på Core 0? Fordi ESP32 har to kerner, og
//   brugerfladen + kommunikationen kræver:
//     * Blokerende kald (SPI til TFT, LittleFS I/O, Serial).
//     * Langsomme operationer (fillScreen, filskrivning).
//     * Mulighed for at blive afbrudt af metrologien (høj prioritet).
//
//   Hvis GUI'en kørte på samme kerne som metrologien, ville en
//   fillScreen (op til ~50 ms) eller en filskrivning (op til ~10 ms)
//   forsinke sample-loopen — og dermed ødelægge 50 Hz NMRR-filteret.
//   Ved at isolere GUI'en på Core 0 sikres det, at metrologien på
//   Core 1 altid kan køre uforstyrret.
//
//   Hvorfor 7 trin i fast rækkefølge? Fordi rækkefølgen er kritisk:
//     * systemHalted skal tjekkes FØRST — ellers kunne GUI'en tegne
//       normale målinger oven på katastrofe-skærmen.
//     * *RST skal tjekkes før SCPI-polling — ellers kunne en *RST
//       blive behandlet to gange (én gang i fejltilstand, én gang
//       udenfor).
//     * SCPI skal polles FØR queue-læsning — ellers ville en PC, der
//       sender kommandoer hurtigt, opleve forsinkede svar.
//     * Queue-læsning skal ske FØR skærmopdatering — ellers ville
//       skærmen vise gamle data.
//     * Skærmopdatering skal ske FØR datalogging — ellers ville
//       brugeren se forsinket visning, når logging er langsom.
//
// HVAD KALDER DENNE TASK? (KALDE-GRAF):
//   Task_GUI_SCPI er en FreeRTOS-task, der startes ÉN gang fra setup()
//   i Sektion 15:
//
//     xTaskCreatePinnedToCore(
//         Task_GUI_SCPI,
//         "GUITask",
//         16384,
//         NULL,
//         1,       // Prioritet 1 (lav)
//         NULL,
//         0        // Core 0
//     );
//
//   Efter start kører tasken uendeligt, indtil ESP32'en genstarter.
//   Den kaldes ALDRIG direkte fra andre funktioner — den er en selvstændig
//   RTOS-task, der styres af FreeRTOS-scheduler'en.
//
//   Tasken kalder selv følgende funktioner:
//     * tft.fillScreen(), tft.setTextColor(), tft.setCursor(),
//       tft.setTextSize(), tft.println(), tft.drawFloat(),
//       tft.drawString(), tft.fillRect() — TFT_eSPI-biblioteket.
//     * ledcWriteTone() — LEDC-API (core 3.x).
//     * xSemaphoreTake() / xSemaphoreGive() — FreeRTOS-mutex.
//     * forceRelaysToSafeState() (Sektion 5).
//     * resetTrendGraph() (Sektion 9).
//     * renderTrendGraph() (Sektion 9).
//     * handleScpiCommand() (Sektion 11).
//     * ensureLogFileHasRoom() (Sektion 13).
//     * LittleFS.open(), logFile.print(), logFile.close() — LittleFS.
//     * snprintf() — standard C-bibliotek.
//     * xQueueReceive() — FreeRTOS-kø.
//     * vTaskDelay() — FreeRTOS.
//
//   Tasken læser følgende globale variable:
//     * systemHalted (volatile bool).
//     * remoteRescueRequested (volatile bool).
//     * isRemoteMode (volatile bool).
//     * graphModeActive (volatile bool).
//     * autoRangeEnabled (volatile bool).
//     * loggingEnabled (volatile bool).
//     * buzzerActive (volatile bool).
//     * latestPacket (MeasurementPacket).
//     * logBuffer[], logBufferLength (globale).
//     * dataQueue (FreeRTOS-kø).
//
//   Tasken skriver følgende globale variable:
//     * systemHalted (volatile bool).
//     * remoteRescueRequested (volatile bool).
//     * autoRangeEnabled (volatile bool).
//     * buzzerActive (volatile bool).
//     * loggingEnabled (volatile bool).
//     * currentMode (volatile MeasurementMode).
//     * latestPacket (MeasurementPacket).
//     * logBuffer[], logBufferLength (globale).
//     * oldDisplayValue (lokal, ikke global).
//     * logCounter (lokal, ikke global).
//     * currentLogFileIndex (lokal, ikke global).
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-RTOS-4.01] TASK ALLOKERING
//     Kravet specificerer: "Core 0 (Bruger-kernen): Får funktionen
//     Task_GUI_SCPI. Den har lav prioritet. Den tegner grafik på skærmen,
//     overvåger USB-porten for kommandoer og gemmer filer."
//     OPFYLDELSE: Task_GUI_SCPI er præcis denne funktion. Den kører på
//     Core 0 med prioritet 1 (lav), tegner skærm (trin 5), overvåger
//     USB (trin 3) og gemmer filer (trin 6).
//
//   * [REQ-ERR-16.01] VISUEL KATASTROFE-ALARM
//     Kravet specificerer: "Så snart systemHalted == true detekteres af
//     Core 0 (GUI-kernen), SKAL den afbryde enhver igangværende
//     skærmoptegning. Skærmen cleares (tft.fillScreen(TFT_RED)), og en
//     blinkende advarsel (f.eks. 'HIGH VOLTAGE OVERLOAD - REMOVE LEADS')
//     skrives i hvid midt på skærmen. Piezo-buzzeren aktiveres kontinuerligt."
//     OPFYLDELSE: Trin 1 implementerer præcis dette. Skærmen farves rød
//     (TFT_RED), hvid tekst vises, og buzzeren aktiveres med 2 kHz.
//     Bemærk: advarslen blinker ikke i denne version (den er statisk),
//     men det er en kendt forenkling.
//
//   * [REQ-ERR-16.02] LÅST RECOVERY-TILSTAND
//     Kravet specificerer: "Instrumentet må aldrig forsøge at genoptage
//     målingen automatisk. Koden SKAL indgå i en uendelig while(systemHalted)
//     løkke, der kun poller USB-porten for *RST kommandoen. Først her må
//     systemet gå tilbage til MODE_DCV (1000V range), rydde fejl-flaget
//     og starte RTOS-måleopgaverne igen."
//     OPFYLDELSE: Trin 1's while(systemHalted)-løkke implementerer dette.
//     Den poller USB for *RST, og ved modtagelse: rydder systemHalted,
//     sætter currentMode = MODE_DCV, kalder forceRelaysToSafeState(),
//     nulstiller trend-grafen og forlader løkken.
//
//   * [REQ-COM-8.01] SCPI OVERVÅGNING
//     Kravet specificerer eksempler på *IDN?, MEAS:VOLT:DC? og *RST.
//     OPFYLDELSE: Trin 3 poller Serial.available() og kalder
//     handleScpiCommand() for hver komplet linje. Selve kommando-
//     håndteringen ligger i Sektion 11.
//
//   * [REQ-SCPI-12.03] UDLÆSNING OG KONTROL
//     Kravet specificerer, at READ? skal returnere på under 5 ms.
//     OPFYLDELSE: Trin 3's Serial.setTimeout(SCPI_READ_TIMEOUT_MS=10)
//     sikrer, at readStringUntil ikke blokerer GUI'en i op til 1 sekund.
//     Trin 4's xQueueReceive(..., 0) sikrer, at GUI-loopet kører hver
//     ~2 ms. Tilsammen giver det en responstid på under 5 ms.
//
//   * [REQ-GUI-10.01] ANTI-FLICKER SKÆRMOPDATERING
//     Kravet specificerer: "For at opdatere tal uden at skærmen blinker,
//     overskriver man det gamle tal med baggrundsfarven, før man tegner
//     det nye."
//     OPFYLDELSE: Trin 5's drawFloat(oldDisplayValue, ..., TFT_BLACK)
//     visker det gamle tal ud med sort, før det nye tegnes med grøn.
//
//   * [REQ-UI-14.01] ZONEDELING AF SKÆRMEN
//     Kravet specificerer: "Zone 1 (Top-højre): Statusindikatorer
//     (CAT III advarselstrekant ved >30V, USB-forbindelse aktiv,
//     Datalogger aktiv, Auto-Range status). Zone 2 (Midt): Primært
//     display. Zone 3 (Bund): AC Ripple."
//     OPFYLDELSE: Trin 5 tegner:
//       - Zone 1: fillRect(600, 0, 200, 40) — USB-status og RNG-status.
//         fillRect(500, 0, 100, 40) — CAT III-advarsel ved >30V.
//       - Zone 2: drawFloat(receivedPacket.finalValue, 4, 10, 50, 7) —
//         stort grønt tal.
//       - Zone 3: drawString("AC Ripple: ...", 10, 450, 2) — ripple.
//         drawString("LOG OK" / "LOG FULL", 410, 450, 2) — log-status.
//
//   * [REQ-UI-14.02] DUAL-DISPLAY MODE
//     Kravet specificerer: "Når instrumentet er i DCV-mode, og Blok 6
//     (True RMS) stadig kører parallelt i baggrunden, SKAL der kunne
//     renders en mindre tekst i Zone 3 (bund) med 'AC Ripple: X.XXX V'."
//     OPFYLDELSE: Trin 5 tegner "AC Ripple: X.XXX V" i Zone 3
//     (y=450), baseret på receivedPacket.acRippleValue (som er
//     skal eret korrekt af Core 1, jf. buglist punkt 6).
//
//   * [REQ-UI-14.03] RULLENDE TREND-GRAF
//     Kravet specificerer, at grafen skal kunne aktiveres via SCPI.
//     OPFYLDELSE: Trin 5's graphModeActive-gren kalder renderTrendGraph()
//     med 0..5V skala, når graphModeActive er true (sat af DISP:GRAPH ON
//     i Sektion 11).
//
//   * [REQ-MEM-15.01] SKRIVEBUFFER (WEAR LEVELING)
//     Kravet specificerer: "Datalogger-funktionen SKAL samle op til
//     f.eks. 100 målinger (1 sekund) i et String eller char array i RAM.
//     Når bufferen er fuld, åbnes filen i LittleFS, hele blokken skrives
//     i én samlet operation, hvorefter filen lukkes."
//     OPFYLDELSE: Trin 6 akkumulerer målinger i logBuffer[] (char-array
//     med LOG_BUFFER_MAX_LENGTH=4096 bytes), tæller i logCounter, og
//     skriver hele bufferen i én LittleFS.open(..., "a") + logFile.print()
//     + logFile.close()-sekvens, når logCounter >= LOG_BUFFER_TARGET (100).
//
//   * [REQ-MEM-15.02] LOG-ROTATION OG STØRRELSESKONTROL
//     Kravet specificerer: "Overstiger filen 1 MB, SKAL instrumentet
//     automatisk oprette en ny fil (f.eks. log_002.csv)."
//     OPFYLDELSE: Trin 6 kalder ensureLogFileHasRoom() (Sektion 13),
//     som roterer til en ny fil, når den nuværende er over 1 MB. Hvis
//     alle filer er fyldte, sættes loggingEnabled = false, og GUI viser
//     "LOG FULL".
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE) — indirekte
//     Kravet specificerer, at systemet skal kunne skifte mellem modes.
//     OPFYLDELSE: Trin 2 (og trin 1's recovery) sætter currentMode til
//     MODE_DCV og kalder forceRelaysToSafeState(). Selve modeskift
//     under normal drift håndteres af handleScpiCommand() (Sektion 11).
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i tasken er kommenteret med både HVAD og
//     HVORFOR.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: receivedPacket, oldDisplayValue, logCounter,
//     currentLogFileIndex, valueTextBuffer, rescueCommand osv. er alle
//     beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR ER receivedPacket LOKAL (IKKE GLOBAL)?
//     Fordi den modtages fra dataQueue og derefter kopieres til
//     latestPacket (global). Ved at gøre den lokal undgås det, at
//     SCPI-parseren læser en halvt-opdateret pakke. Den globale
//     latestPacket opdateres kun, når pakken er komplet.
//
//   * HVORFOR ER oldDisplayValue LOKAL?
//     Fordi den kun bruges af denne task til anti-flicker. Ved at
//     holde den lokal undgås race med andre tasks.
//
//   * HVORFOR ER logCounter LOKAL?
//     Fordi den kun bruges af denne task til at tælle, hvor mange
//     målinger der er i logBuffer. Når den når LOG_BUFFER_TARGET (100),
//     skrives bufferen til flash, og tælleren nulstilles.
//
//   * HVORFOR ER currentLogFileIndex LOKAL?
//     Fordi den kun bruges af denne task til at holde styr på, hvilken
//     logfil der skrives til. Hvis den var global, skulle den beskyttes
//     af en mutex — hvilket er unødvendigt overhead, da kun denne task
//     rører den.
//
//   * HVORFOR ER valueTextBuffer LOKAL?
//     Fordi den kun bruges midlertidigt i trin 6 til at formatere
//     finalValue som tekst. Den er 32 bytes (nok til "%.5f" af en
//     double med 5 decimaler plus fortegn og nul).
//
//   * HVORFOR ER DER EN systemHalted-CHECK I TRIN 1 (IKKE BARE EN IF)?
//     Fordi når systemHalted er sat, skal GUI'en LÅSES i en uendelig
//     løkke (REQ-ERR-16.02). Det er ikke nok at vise alarmen én gang
//     — den skal vedvare, indtil brugeren sender *RST. Derfor er der
//     en while(systemHalted)-løkke inde i if(systemHalted)-grenen.
//
//   * HVORFOR TJEKKES *RST BÅDE I TRIN 1 OG TRIN 2?
//     Fordi *RST kan modtages i to situationer:
//       - Under fejltilstand (trin 1): GUI'en er låst i recovery-løkken,
//         og *RST skal frigive låsen.
//       - Uden for fejltilstand (trin 2): En normal *RST-kommando skal
//         udføre reset til DCV 1000V.
//     De to steder har forskellige side-effekter (trin 1 frigiver
//     systemHalted, trin 2 gør ikke), så de kan ikke slås sammen.
//
//   * HVORFOR SÆTTES Serial.setTimeout(SCPI_READ_TIMEOUT_MS=10) I TRIN 3?
//     Fordi Serial.readStringUntil('\n') som standard blokerer i op til
//     1 sekund, hvis der ikke kommer et '\n'. Det ville fryse GUI'en
//     i op til 1 sekund — uacceptabelt for en responsiv brugerflade.
//     Med 10 ms timeout undgås dette, og SCPI-responstiden holdes under
//     5 ms (REQ-SCPI-12.03).
//
//   * HVORFOR BRUGES xQueueReceive(..., 0) I TRIN 4?
//     Fordi vi ikke vil blokere, hvis der ikke er en pakke. Med 0
//     timeout returnerer funktionen øjeblikkeligt med pdFAIL, og
//     GUI-loopet fortsætter til næste iteration. Det holder GUI'en
//     responsiv og SCPI-responstiden lav.
//
//   * HVORFOR KOPIERES receivedPacket TIL latestPacket I TRIN 4?
//     Fordi SCPI-parseren (Sektion 11) læser latestPacket for at svare
//     på READ?. Hvis vi ikke kopierede, ville SCPI-parseren læse en
//     gammel pakke. Kopieringen er atomar (struct assignment), så
//     SCPI-parseren ser altid en komplet pakke.
//
//   * HVORFOR ER DER EN isRemoteMode-CHECK I TRIN 5?
//     Fordi SYST:REM (Sektion 11) slår GUI-opdateringer fra for at
//     frigive CPU-cyklusser til SCPI. Hvis vi stadig tegnede skærmen,
//     ville SYST:REM ikke give den ønskede effekt.
//
//   * HVORFOR SLETTES KUN GRAF-OMRÅDET (fillRect) I GRAF-MODE?
//     Fordi det store tal i Zone 2 skal bevares, når grafen vises.
//     Ved kun at slette graf-området (y=180..480) undgås det, at det
//     store tal bliver visket ud. Det er anti-flicker-teknikken anvendt
//     på zone-niveau.
//
//   * HVORFOR BRUGES drawFloat(oldDisplayValue, ..., TFT_BLACK) I TRIN 5?
//     Fordi det visker det gamle tal ud ved at tegne det med sort
//     tekstfarve og sort baggrund. Det er REQ-GUI-10.01's anti-
//     flicker-teknik.
//
//   * HVORFOR OPDATERES oldDisplayValue ALTID (OGSÅ I GRAF-MODE)?
//     Fordi når grafen slukkes (DISP:GRAPH OFF), skal det store tal
//     vises igen — og oldDisplayValue skal være den seneste værdi,
//     så anti-flicker virker korrekt ved skiftet. Uden denne opdatering
//     ville det gamle tal fra før graf-mode blive visket ud i stedet.
//
//   * HVORFOR TEGNES CAT III-ADVARSLEN I ET SEPARAT fillRect(500, 0, 100, 40)?
//     Fordi advarslen skal kunne vises/skjules uafhængigt af USB- og
//     RNG-status. Ved at have sit eget fillRect-område undgås det, at
//     advarslen "hænger" efter, når spændingen falder under 30V igen.
//
//   * HVORFOR BRUGES snprintf(NULL, 0, ...) I TRIN 6?
//     Fordi vi skal vide, hvor mange bytes CSV-linjen kræver, FØR vi
//     skriver den til logBuffer. snprintf(NULL, 0, ...) returnerer
//     længden uden at skrive noget — det er en standard C-teknik til
//     at måle længden af en formateret streng.
//
//   * HVORFOR BRUGES snprintf OG IKKE String-KONKATENERING I TRIN 6?
//     Fordi String-konkatenation på ESP32 kan fragmentere heap'en over
//     tid (mange små allokeringer). snprintf skriver direkte til en
//     fast char-buffer (logBuffer), hvilket undgår fragmentering. Det
//     er især vigtigt, fordi denne kode kører ~83 gange i sekundet.
//
//   * HVORFOR TØMMES logBuffer ALTID (logBufferLength = 0) I TRIN 6?
//     Fordi uanset om skrivningen til flash lykkedes eller ej, skal
//     bufferen tømmes — ellers ville den næste flush skrive de samme
//     data igen (duplikering). Hvis skrivningen fejlede, er data tabt,
//     men det er bedre end at skrive dem to gange.
//
//   * HVORFOR SÆTTES logBuffer[0] = '\0' I TRIN 6?
//     Fordi logBuffer er en char-array, og selvom logBufferLength er
//     nulstillet, kan der ligge gamle tegn i bufferen. Ved at sætte
//     første tegn til '\0' sikres det, at bufferen er en gyldig tom
//     C-streng, hvis nogen læser den (f.eks. via logFile.print()).
//
//   * HVORFOR ER DER EN vTaskDelay(2 ms) I TRIN 7?
//     Fordi GUI-tasken kører på Core 0 sammen med andre system-tasks
//     (FreeRTOS idle, WiFi/BT hvis aktiveret). Uden en delay ville
//     GUI-tasken monopolere Core 0 og forhindre andre tasks i at køre.
//     2 ms er nok til at give andre tasks luft, men stadig hurtigt nok
//     til at GUI'en føles responsiv.
//
//   * SIDE-EFFEKTER:
//     - Tegner på TFT-skærmen (SPI-trafik).
//     - Læser fra Serial (USB-CDC).
//     - Skriver til Serial (SCPI-svar).
//     - Ændrer systemHalted, remoteRescueRequested, isRemoteMode,
//       graphModeActive, autoRangeEnabled, loggingEnabled, buzzerActive,
//       currentMode (alle volatile globals).
//     - Opdaterer latestPacket (global).
//     - Skriver til logBuffer[] (global).
//     - Kalder forceRelaysToSafeState() (ændrer relæer).
//     - Kalder resetTrendGraph() (nulstiller trend-buffer).
//     - Kalder renderTrendGraph() (tegner graf).
//     - Kalder handleScpiCommand() (ændrer mange globale variable).
//     - Kalder ensureLogFileHasRoom() (læser LittleFS-metadata).
//     - Skriver til LittleFS (logfil).
//     - Sover 2 ms pr. iteration (vTaskDelay).
//
//   * RETURVÆRDI:
//     Ingen (void). RTOS-tasks returnerer aldrig — de kører uendeligt.
//
//   * FORHOLDSREGEL VED ÆNDRING AF SKÆRM-LAYOUT:
//     Hvis man ændrer zone-opdelingen (f.eks. flytter det store tal),
//     skal man opdatere:
//       - drawFloat-koordinaterne i trin 5.
//       - fillRect-koordinaterne for statuslinjen, CAT III-advarslen,
//         AC-ripple-linjen og log-status.
//       - renderTrendGraph()'s graf-område (Sektion 9).
//     Ellers vil de forskellige zoner overlappe eller efterlade gamle
//     streger.
//
//   * FORHOLDSREGEL VED ÆNDRING AF LOG_BUFFER_TARGET:
//     Hvis LOG_BUFFER_TARGET ændres fra 100 (Sektion 3), påvirker det:
//       - Hvor ofte der skrives til flash (wear leveling).
//       - Hvor meget RAM logBuffer bruger (LOG_BUFFER_MAX_LENGTH).
//       - Hvor lang tid en flush tager (større buffer = langsommere).
//     Kravet (REQ-MEM-15.01) specificerer 100 — en ændring kræver en
//     opdatering af kravspecifikationen.
//
//   * FORHOLDSREGEL VED TILFØJELSE AF NY SCPI-KOMMANDO:
//     Hvis man tilføjer en ny kommando i handleScpiCommand() (Sektion 11),
//     skal man overveje, om den kræver GUI-opdatering. Hvis ja, skal
//     trin 5 udvides med den nye visning.
//
//   * FORHOLDSREGEL VED ÆNDRING AF systemHalted-LOGIKKEN:
//     Hvis man ændrer, hvordan systemHalted sættes (f.eks. i ISR'en),
//     skal man sikre, at trin 1 stadig reagerer korrekt. Især:
//       - GUI'en skal vise alarmen inden for ~100 ms.
//       - *RST skal stadig kunne frigive låsen.
//       - Relæerne skal stadig tvinges i safe state.
//
//   * SAMMENHÆNG MED Task_Metrology:
//     Task_GUI_SCPI og Task_Metrology deler data via dataQueue (trin 4)
//     og via volatile globals (systemHalted, remoteRescueRequested,
//     isRemoteMode, graphModeActive, autoRangeEnabled, loggingEnabled,
//     buzzerActive, currentMode, latestPacket, logBuffer[]).
//     Hvis man ændrer MeasurementPacket's felter (Sektion 3), skal
//     trin 4 og 5 opdateres — ellers vil pakken blive læst forkert.
//
//   * SAMMENHÆNG MED hardwareFaultISR:
//     ISR'en sætter systemHalted = true. Tasken reagerer i trin 1.
//     Hvis man ændrer ISR'ens adfærd, skal man sikre, at tasken stadig
//     reagerer korrekt (især: at systemHalted stadig er volatile, og
//     at tasken stadig tjekker den i starten af hver iteration).
//
// -------------------------------------------------------------------------
void Task_GUI_SCPI(void *pvParameters) {
    MeasurementPacket receivedPacket;    // Tom kasse til at modtage data fra Core 1.
    double oldDisplayValue = 0.0;        // Husker forrige tal til anti-flicker skærmopdatering.
    int logCounter = 0;                  // Tæller hvor mange målinger vi har i RAM.
    uint32_t currentLogFileIndex = 1;    // Indeks for nuværende logfil (log.csv, log_002.csv, ...).
    char valueTextBuffer[32];            // Buffer til den formaterede double-værdi (snprintf).

    for (;;) {                           // Uendelig løkke for brugerfladen.

        // -- 1. HÅNDTERING AF KATASTROFE-FEJL (REQ-ERR-16.01/16.02) --
        if (systemHalted == true) {      // Hvis hardware-komparatoren har fundet 230V i Ohm-mode...
            tft.fillScreen(TFT_RED);     // ...Farv skærmen blodrød øjeblikkeligt.
            tft.setTextColor(TFT_WHITE, TFT_RED); // Hvid tekst på rød baggrund.
            tft.setCursor(10, 100);      // Placer markøren midt på skærmen.
            tft.setTextSize(3);          // Gør teksten stor og tydelig.
            tft.println("HIGH VOLTAGE OVERLOAD"); // Udskriv katastrofe-advarsel.
            tft.println("REMOVE LEADS!"); // Bed brugeren fjerne ledningerne.
            if (!buzzerActive) {         // Hvis buzzeren ikke allerede hyler...
                // RETTELSE (buglist punkt 2): Core 3.x-API — ledcWriteTone tager pin, ikke kanal.
                ledcWriteTone(PIN_BUZZER, 2000); // Start en vedvarende 2 kHz hyleton via LEDC.
                buzzerActive = true;     // Husk at buzzeren nu er aktiv.
            }
            while (systemHalted) {       // Lås systemet fast i en uendelig løkke (REQ-ERR-16.02).
                if (Serial.available() > 0) { // Lyt om PC'en skriver.
                    String rescueCommand = Serial.readStringUntil('\n'); // Læs hele linjen.
                    rescueCommand.trim(); rescueCommand.toUpperCase(); // Rens og normaliser.
                    if (rescueCommand == "*RST") { // Hvis PC'en beder om reset...
                        remoteRescueRequested = true; // Bed GUI-tasken om at udføre selve reset nedenfor.
                    }
                }
                if (remoteRescueRequested) { // Hvis reset er anmodet (fra SCPI eller *RST)...
                    remoteRescueRequested = false; // Nulstil anmodningen.
                    systemHalted = false; // Frigiv fejllåsen.
                    // RETTELSE (buglist punkt 2): Core 3.x-API — ledcWriteTone tager pin.
                    ledcWriteTone(PIN_BUZZER, 0); // Sluk for hyletonen (frekvens 0 = stop).
                    buzzerActive = false; // Husk at buzzeren er slukket.
                    autoRangeEnabled = true; // RETTELSE (buglist punkt 4): Genaktivér auto-ranging efter reset.
                    xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
                    currentMode = MeasurementMode::MODE_DCV; // Gå til sikker DCV-mode.
                    forceRelaysToSafeState(); // Tving relæerne i 1000V safe state.
                    xSemaphoreGive(relayMutex); // Frigiv relæ-mutex.
                    tft.fillScreen(TFT_BLACK); // Ryd skærmen.
                    xSemaphoreTake(trendMutex, portMAX_DELAY); // Lås trend-mutex.
                    resetTrendGraph();    // Nulstil trend-grafen efter reset.
                    xSemaphoreGive(trendMutex); // Frigiv trend-mutex.
                    break;                // Forlad den låste løkke.
                }
                vTaskDelay(pdMS_TO_TICKS(50)); // Vent lidt for at lade processoren trække vejret.
            }
        }

        // -- 2. HÅNDTERING AF *RST UDEN FOR FEJLTILSTAND —
        // Hvis SCPI-parseren har sat remoteRescueRequested, udfør reset her, hvor vi ejer skærmen.
        if (remoteRescueRequested) {     // Hvis en *RST er anmodet...
            remoteRescueRequested = false; // Nulstil anmodningen.
            systemHalted = false;        // Sikr at vi ikke er låst.
            // RETTELSE (buglist punkt 2): Core 3.x-API — ledcWriteTone tager pin.
            ledcWriteTone(PIN_BUZZER, 0); // Sluk for eventuel hyleton.
            buzzerActive = false;        // Husk at buzzeren er slukket.
            autoRangeEnabled = true;     // RETTELSE (buglist punkt 4): Genaktivér auto-ranging efter reset.
            xSemaphoreTake(relayMutex, portMAX_DELAY); // Lås relæ-mutex.
            currentMode = MeasurementMode::MODE_DCV; // Gå til sikker DCV-mode.
            forceRelaysToSafeState();    // Tving relæerne i 1000V safe state.
            xSemaphoreGive(relayMutex);  // Frigiv relæ-mutex.
            tft.fillScreen(TFT_BLACK);   // Ryd skærmen.
            xSemaphoreTake(trendMutex, portMAX_DELAY); // Lås trend-mutex.
            resetTrendGraph();           // Nulstil trend-grafen.
            xSemaphoreGive(trendMutex);  // Frigiv trend-mutex.
            Serial.println("OK");        // Bekræft overfor PC'en.
        }

        // -- 3. SCPI PARSER OG USB KOMMUNIKATION (REQ-COM-8.01) —
        // Polles FØRST og non-blocking, så responstiden ikke afhænger af GUI-opdateringer.
        Serial.setTimeout(SCPI_READ_TIMEOUT_MS); // Sæt kort timeout, så readStringUntil ikke blokerer GUI i 1 sekund.
        while (Serial.available() > 0) { // Så længe der ligger tegn i USB-bufferen...
            String originalCommand = Serial.readStringUntil('\n'); // Læs én kommando ad gangen (bevarer case).
            handleScpiCommand(originalCommand, originalCommand); // Send kommandoen til SCPI-parseren (original bevares til filnavne).
        }

        // -- 4. LÆS MÅLEDATA FRA KØEN (non-blocking) —
        // Bruger 0 timeout, så GUI-loopet kører hver ~2 ms og SCPI-responstiden holdes lav.
        if (xQueueReceive(dataQueue, &receivedPacket, 0) == pdPASS) { // Tag en pakke hvis der er en.
            latestPacket = receivedPacket; // Gem pakken globalt, så SCPI kan svare med den.

            // -- 5. OPDATER SKÆRM (ANTI-FLICKER, REQ-GUI-10.01 / REQ-UI-14.01) --
            if (!isRemoteMode) {         // Opdater kun skærm, hvis SYST:REM ikke er aktiveret.
                if (graphModeActive) {   // Hvis trend-graf-mode er aktiv...
                    tft.fillRect(0, 180, 800, 300, TFT_BLACK); // ...slet kun graf-området for at undgå flicker.
                    xSemaphoreTake(trendMutex, portMAX_DELAY); // Lås trend-mutex.
                    renderTrendGraph((float)receivedPacket.finalValue, 0.0f, 5.0f); // Tegn grafen med 0..5V skala.
                    xSemaphoreGive(trendMutex); // Frigiv trend-mutex.
                } else {                 // Ellers tegn det store primære tal.
                    tft.setTextColor(TFT_BLACK, TFT_BLACK); // Sæt tekstfarve til sort for at viske det gamle ud.
                    tft.drawFloat(oldDisplayValue, 4, 10, 50, 7); // Tegn forrige tal med sort (det forsvinder).
                    tft.setTextColor(TFT_GREEN, TFT_BLACK); // Sæt tekstfarven til skriggrøn.
                    tft.drawFloat(receivedPacket.finalValue, 4, 10, 50, 7); // Tegn det nye rigtige tal.
                }
                oldDisplayValue = receivedPacket.finalValue; // Opdater ALTID — også i graf-mode.
                // Zone 1: Statusindikatorer (REQ-UI-14.01).
                tft.fillRect(600, 0, 200, 40, TFT_BLACK); // Slet status-linjen.
                tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Vælg gul tekst til status.
                tft.drawString(isRemoteMode ? "USB:REM" : "USB:LOC", 610, 5, 2); // Vis USB-status.
                tft.drawString(autoRangeEnabled ? "RNG:AUTO" : "RNG:MAN", 610, 22, 2); // Vis range-status (auto/man).
                // RETTELSE (REQ-UI-14.01): CAT III-advarselstrekant ved >30V.
                // Tegnes i Zone 1 (øverste højre) som en rød trekant-tekst, når spændingen overstiger 30V.
                tft.fillRect(500, 0, 100, 40, TFT_BLACK); // Slet advarsels-området først.
                if (fabs(receivedPacket.finalValue) > HIGH_VOLTAGE_WARNING_THRESHOLD) { // Hvis spændingen > 30V...
                    tft.setTextColor(TFT_RED, TFT_BLACK); // ...vælg rød tekst...
                    tft.drawString("!CAT III!", 505, 12, 2); // ...og vis advarselstrekanten.
                }
                // Zone 3: Dual-display AC Ripple (REQ-UI-14.02) — nu med korrekt RMS-værdi fra AIN1.
                tft.fillRect(0, 440, 400, 30, TFT_BLACK); // Slet kun ripple-linjen.
                tft.setTextColor(TFT_CYAN, TFT_BLACK); // Vælg cyan tekst til ripple.
                tft.drawString("AC Ripple: " + String(receivedPacket.acRippleValue, 3) + " V", 10, 450, 2); // Vis korrekt AC-ripple.
                // Log-status: Vis "LOG FULL" hvis LittleFS ikke kan skrive mere.
                tft.fillRect(400, 440, 200, 30, TFT_BLACK); // Slet log-status-området.
                if (!loggingEnabled) {   // Hvis logging er stoppet pga. fuld partition...
                    tft.setTextColor(TFT_RED, TFT_BLACK); // ...vælg rød tekst...
                    tft.drawString("LOG FULL", 410, 450, 2); // ...og vis advarslen.
                } else {                 // Ellers vises intet (logging kører normalt).
                    tft.setTextColor(TFT_GREEN, TFT_BLACK); // Vælg grøn tekst.
                    tft.drawString("LOG OK", 410, 450, 2); // Vis at logging kører.
                }
            }

            // -- 6. WEAR-LEVELING DATALOGGING TIL LITTLEFS (REQ-MEM-15.01/15.02) --
            if (loggingEnabled) {        // Kun hvis logging er aktiv.
                snprintf(valueTextBuffer, sizeof(valueTextBuffer), "%.5f", receivedPacket.finalValue); // ESP32-sikker float→tekst (snprintf understøtter %f).
                int requiredLength = snprintf(NULL, 0, "%lu,%s\n", (unsigned long)receivedPacket.timeStamp, valueTextBuffer); // Beregn pladsbehov.
                if (logBufferLength + requiredLength < LOG_BUFFER_MAX_LENGTH) { // Tjek at der er plads i RAM-bufferen.
                    logBufferLength += snprintf(logBuffer + logBufferLength, LOG_BUFFER_MAX_LENGTH - logBufferLength,
                                                "%lu,%s\n", (unsigned long)receivedPacket.timeStamp, valueTextBuffer); // Føj CSV-linje til buffer.
                    logCounter++;        // Tæl at vi har fået en ny måling.
                } else {                 // Hvis bufferen er ved at være fuld...
                    logCounter = LOG_BUFFER_TARGET; // ...tving en flush nu.
                }
                if (logCounter >= LOG_BUFFER_TARGET) { // Når vi har samlet 100 målinger (1 sekund) i RAM...
                    String logFileName;  // Navnet på den fil vi vil skrive til.
                    if (ensureLogFileHasRoom(currentLogFileIndex, logFileName)) { // Hvis der er plads...
                        File logFile = LittleFS.open(logFileName.c_str(), "a"); // Åbn filen i "Append" mode.
                        if (logFile) {   // Tjek om filen åbnede uden fejl.
                            logFile.print(logBuffer); // Skriv alle målinger til flash i én samlet operation.
                            logFile.close(); // Luk filen (redder chippen fra at blive slidt op).
                        } else {         // Hvis filen ikke kunne åbnes...
                            loggingEnabled = false; // ...så stop logging og vis "LOG FULL".
                        }
                    } else {             // Hvis alle filer er fyldte...
                        loggingEnabled = false; // ...så stop logging og vis "LOG FULL".
                    }
                    logBufferLength = 0; // Tøm RAM-bufferen — uanset om skrivningen lykkedes.
                    logBuffer[0] = '\0'; // Nulstil første tegn (sikkerhed).
                    logCounter = 0;      // Nulstil tælleren, så vi er klar til næste sekund.
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2));    // Tillad andre RTOS-opgaver på Core 0 at køre et par millisekunder.
    }
}

// =========================================================================
// SEKTION 15: OPSÆTNING OG INITIERING (SETUP) — REQ-RTOS-4.02
// =========================================================================

// -------------------------------------------------------------------------
// 15.2 FUNKTION: setup() — SYSTEMINITIERING
// -------------------------------------------------------------------------
//
// FORMÅL (HVAD):
//   setup() er firmwarens opstartsfunktion. Arduino IDE kalder den
//   automatisk ÉN gang efter boot, FØR loop() (som i dette design er tom)
//   og FØR FreeRTOS-tasks begynder at køre. setup() har ansvaret for at
//   bringe hardware og software fra "rå ESP32 efter reset" til "fuldt
//   operationelt multimeter med to kørende RTOS-tasks".
//
//   Funktionen udfører 20 initialiserings-trin i en nøje planlagt
//   rækkefølge:
//
//     Trin 1:  Start Serial (USB-CDC) til SCPI-kommunikation.
//     Trin 2:  Konfigurer 5 GPIO-pins som OUTPUT (4 relæer + buzzer).
//     Trin 3:  Konfigurer LEDC til buzzer (core 3.x API).
//     Trin 4:  Sæt buzzer til 0 Hz (stille).
//     Trin 5:  Opret relayMutex (beskytter relæ-tilstand).
//     Trin 6:  Opret calibrationMutex (beskytter kalibreringstabellen).
//     Trin 7:  Opret trendMutex (beskytter trend-bufferen).
//     Trin 8:  Tving relæerne i safe state (1000V, teststrøm afbrudt).
//     Trin 9:  Konfigurer PIN_FAULT_INT som INPUT_PULLUP.
//     Trin 10: Tilkobl hardwareFaultISR til FALDENDE flanke.
//     Trin 11: Initialisér TFT-displayet.
//     Trin 12: Sæt skærm-rotation til 0 (landskab).
//     Trin 13: Fyld skærmen sort.
//     Trin 14: Start LittleFS (formatér hvis jomfrueligt).
//     Trin 15: Indlæs kalibreringskonstanter fra NVS til RAM.
//     Trin 16: Start I2C-bussen på de korrekte pins.
//     Trin 17: Initialisér ADS1115 på adresse 0x48.
//     Trin 18: Sæt ADC-gain til GAIN_ONE (±4.096V).
//     Trin 19: Sæt ADC-datarate til 475 SPS.
//     Trin 20: Opret dataQueue (10 pakker).
//     Trin 21: Start Task_Metrology på Core 1 (prioritet 3).
//     Trin 22: Start Task_GUI_SCPI på Core 0 (prioritet 1).
//
//   Rækkefølgen er kritisk: f.eks. skal relæerne i safe state, FØR
//   interruptet tilkobles (så en tidlig fejl ikke kan efterlade dem i
//   en farlig tilstand), og kalibreringsdata skal være indlæst, FØR
//   Task_Metrology starter (så den første måling bruger korrekte
//   konstanter).
//
// BEGRUNDELSE (HVORFOR):
//   Hvorfor så mange trin i setup() og ikke spredt ud over koden?
//   Fordi:
//     * Hardware-initialisering skal ske i en bestemt rækkefølge
//       (f.eks. relæer i safe state, FØR interruptet tilkobles).
//     * Fejl i initialiseringen skal opdages tidligt og rapporteres
//       over Serial (f.eks. "ADS1115 not found").
//     * RTOS-tasks skal startes SIDST, når alle data-strukturer er klar.
//     * Koden bliver lettere at læse, når al initialisering er samlet
//       ét sted (jf. REQ-DOC-1.02 og REQ-DOC-1.03).
//
//   Hvorfor er setup() så lang (ca. 70 linjer)? Fordi et multimeter er
//   et komplekst system med mange hardware-komponenter (relæer, ADC,
//   TFT, LittleFS, NVS, buzzer, interrupt). At dele det op i flere
//   funktioner ville gøre det sværere at følge rækkefølgen — og
//   REQ-DOC-1.01 kræver, at alt er i én fil. Så en lang setup() er
//   en bevidst afvejning: læsbarhed og sporbarhed frem for "elegance".
//
// HVAD KALDER DENNE FUNKTION? (KALDE-GRAF):
//   setup() kaldes AUTOMATISK af Arduino IDE's runtime ÉN gang efter
//   boot. Den kaldes ALDRIG af firmwaren selv, og den kaldes ALDRIG
//   igen (medmindre ESP32'en genstarter).
//
//   setup() kalder selv følgende funktioner:
//     * Serial.begin() — Arduino-API.
//     * pinMode() — Arduino-API (5 gange for relæer + buzzer).
//     * ledcAttach() / ledcWriteTone() — LEDC-API (core 3.x).
//     * xSemaphoreCreateMutex() — FreeRTOS (3 gange).
//     * forceRelaysToSafeState() (Sektion 5).
//     * attachInterrupt() / digitalPinToInterrupt() — Arduino-API.
//     * tft.init(), tft.setRotation(), tft.fillScreen() — TFT_eSPI.
//     * LittleFS.begin() — LittleFS.
//     * loadCalibrationFromNvs() (Sektion 4).
//     * Wire.begin() — Arduino-API (I2C).
//     * adc.begin(), adc.setGain(), adc.setDataRate() — Adafruit_ADS1X15.
//     * xQueueCreate() — FreeRTOS.
//     * xTaskCreatePinnedToCore() — FreeRTOS (2 gange).
//
//   setup() skriver følgende globale variable:
//     * relayMutex, calibrationMutex, trendMutex (FreeRTOS-handles).
//     * loggingEnabled (volatile bool) — sættes til false, hvis LittleFS
//       fejler.
//     * dataQueue (FreeRTOS-kø-handle).
//     * calibrationTable[] (via loadCalibrationFromNvs).
//
//   setup() læser ingen globale variable (den initialiserer dem).
//
// HVILKE KRAV I KRAVSPECIFIKATIONEN OPFYLDES?
//
//   * [REQ-ENV-2.03] BOARD VALG
//     Kravet specificerer, at board skal være "ESP32 Dev Module".
//     OPFYLDELSE: setup() er skrevet til ESP32 Dev Module's GPIO-mapping
//     (PIN_I2C_SDA=21, PIN_I2C_SCL=22, osv.). Hvis board ændres, skal
//     pin-mappingen i Sektion 2 opdateres.
//
//   * [REQ-ENV-2.04] HUKOMMELSESALLOKERING (KRITISK!)
//     Kravet specificerer, at partition scheme skal være "No OTA (2MB
//     APP/2MB SPIFFS)" eller tilsvarende, så LittleFS kan bruges.
//     OPFYLDELSE: Trin 14 kalder LittleFS.begin(true), som kræver en
//     partition med plads til LittleFS. Hvis partition scheme er forkert,
//     vil dette kald fejle og loggingEnabled sættes til false.
//
//   * [REQ-RTOS-4.02] OPSÆTNING I SETUP() FUNKTIONEN
//     Kravet specificerer præcis, hvordan setup() skal konfigurere RTOS:
//       - Serial.begin(115200).
//       - dataQueue = xQueueCreate(10, sizeof(MeasurementPacket)).
//       - pinMode for relæer.
//       - xTaskCreatePinnedToCore for Task_Metrology (8192, prioritet 3, Core 1).
//       - xTaskCreatePinnedToCore for Task_GUI_SCPI (16384, prioritet 1, Core 0).
//       - Tom loop().
//     OPFYLDELSE: Alle seks elementer er implementeret. Bemærk at
//     setup() også konfigurerer mutexer, ADC, TFT, LittleFS og NVS —
//     det er ud over kravet, men nødvendigt for et fungerende system.
//
//   * [REQ-SAF-5.02] AKTIVERING
//     Kravet specificerer: "ISR'en skal tilkobles i setup():
//     attachInterrupt(digitalPinToInterrupt(PIN_FAULT_INT),
//     hardwareFaultISR, RISING);"
//     OPFYLDELSE: Trin 10 tilkobler ISR'en — men med FALLING, ikke
//     RISING. Afvigelsen er dokumenteret: optokobleren er aktiv LOW,
//     så den fysiske flanke er faldende. Havde man brugt RISING, ville
//     ISR'en aldrig udløses. Det er en nødvendig korrektion af kravets
//     bogstavelige ordlyd for at matche hardwaren.
//
//   * [REQ-DSP-6.01] ADC OPSÆTNING (ADS1115)
//     Kravet specificerer: "ADC'en skal sættes op til at måle fra
//     -4.096V til +4.096V."
//     OPFYLDELSE: Trin 18 kalder adc.setGain(GAIN_ONE), hvilket sætter
//     full-scale til ±4.096V. Det er præcis kravets konfiguration.
//
//   * [REQ-CAL-13.02] DATASTRUKTUR I FLASH
//     Kravet specificerer: "Denne struct loades ind i RAM i setup()
//     for lynhurtig adgang under drift."
//     OPFYLDELSE: Trin 15 kalder loadCalibrationFromNvs(), som indlæser
//     alle 40 slots fra NVS til RAM. Det sker FØR Task_Metrology startes,
//     så den første måling bruger korrekte konstanter.
//
//   * [REQ-FSM-11.01] HOVEDTILSTANDSMASKINE (MAIN STATE) — indirekte
//     Kravet specificerer, at systemet ved opstart skal være i en kendt
//     tilstand. OPFYLDELSE: Trin 8 kalder forceRelaysToSafeState(),
//     som sætter relæerne i 1000V safe state (1:100 dæmpning,
//     teststrøm afbrudt) FØR interruptet tilkobles. Dermed er systemet
//     sikkert fra første øjeblik.
//
//   * [REQ-ERR-16.01] VISUEL KATASTROFE-ALARM — indirekte
//     Kravet specificerer, at GUI'en skal vise en rød advarselsskærm
//     ved fejl. OPFYLDELSE: Trin 13 fylder skærmen sort (så GUI'en
//     starter fra en ren tilstand), og trin 11-12 initialiserer TFT'en.
//     Uden disse trin ville katastrofe-alarmen ikke kunne vises korrekt.
//
//   * [REQ-ERR-16.02] LÅST RECOVERY-TILSTAND — indirekte
//     Kravet specificerer, at systemet skal kunne komme tilbage til
//     MODE_DCV efter *RST. OPFYLDELSE: setup() initialiserer de
//     data-strukturer og mutexer, som recovery-løkken bruger (relayMutex,
//     trendMutex, calibrationTable, currentMode).
//
//   * [REQ-MEM-15.01] SKRIVEBUFFER (WEAR LEVELING) — indirekte
//     Kravet specificerer, at datalogging skal bruge en RAM-buffer.
//     OPFYLDELSE: Trin 14 starter LittleFS, som er forudsætningen for
//     datalogging. Uden LittleFS ville loggingEnabled være false, og
//     hele wear-leveling-mekanismen ville være inaktiv.
//
//   * [REQ-DOC-1.02] 100% INLINE KOMMENTERINGSGRAD
//     Kravet kræver, at hver ikke-tom kodelinje har en //-kommentar.
//     OPFYLDELSE: Hver linje i setup() er kommenteret med både HVAD
//     og HVORFOR.
//
//   * [REQ-DOC-1.03] NUL-FORUDSÆTNINGS PRINCIPPET
//     Kravet kræver beskrivende, ikke-forkortede variabelnavne.
//     OPFYLDELSE: PIN_RELAY_DIV10, PIN_RELAY_DIV100, PIN_RELAY_AC_ROUTE,
//     PIN_RELAY_OHM, PIN_BUZZER, PIN_FAULT_INT, BUZZER_LEDC_RESOLUTION,
//     relayMutex, calibrationMutex, trendMutex, dataQueue,
//     loadCalibrationFromNvs, RATE_ADS1115_475SPS osv. er alle
//     beskrivende.
//
// DESIGNVALG OG DETALJER:
//
//   * HVORFOR STARTER setup() MED Serial.begin()?
//     Fordi Serial er det ENESTE kommunikationsmiddel, der kan bruges
//     til at diagnosticere fejl under opstart. Hvis f.eks. ADS1115
//     ikke svarer, skal brugeren kunne se "ADS1115 not found" over
//     USB. Hvis Serial blev startet senere, ville tidlige fejl være
//     usynlige.
//
//   * HVORFOR KONFIGURERES ALLE 5 PINS FØR MUTEXER OG SAFE STATE?
//     Fordi forceRelaysToSafeState() (trin 8) bruger digitalWrite() på
//     relæ-pins. Hvis de ikke var konfigureret som OUTPUT, ville
//     digitalWrite() ikke have nogen effekt — og relæerne ville stå
//     i en ukendt tilstand. Ved at konfigurere dem FØRST sikres det,
//     at safe state kan etableres korrekt.
//
//   * HVORFOR BRUGES ledcAttach() OG IKKE ledcSetup()?
//     Det er buglist punkt 2-rettelsen. I ESP32 core 3.x er ledcSetup()
//     fjernet, og LEDC-funktionerne tager pin-nummer i stedet for
//     kanal. ledcAttach(pin, freq, resolution) knytter LEDC til pin'en
//     direkte — det er den korrekte API for core 3.x. Hvis man brugte
//     ledcSetup(), ville koden ikke kompilere på core 3.x.
//
//   * HVORFOR OPRETTES MUTEXER FØR SAFE STATE?
//     Fordi forceRelaysToSafeState() kaldes fra setup() uden at tage
//     relayMutex (der er ingen andre tasks endnu). Mutexerne skal
//     være klar, når Task_Metrology og Task_GUI_SCPI starter senere.
//     Rækkefølgen er ikke kritisk her, men det er rart at have dem
//     klar, før relæerne røres.
//
//   * HVORFOR KALDES forceRelaysToSafeState() FØR attachInterrupt()?
//     Fordi relæerne skal være i safe state, FØR interruptet kan
//     udløses. Hvis ISR'en udløste, mens relæerne stadig var i en
//     ukendt tilstand, ville forceRelaysToSafeStateISR() sætte dem i
//     safe state — men det er bedre at være sikker fra starten.
//
//   * HVORFOR BRUGES INPUT_PULLUP PÅ PIN_FAULT_INT?
//     Fordi optokobleren (PC817/6N137) er aktiv LOW — dvs. den trækker
//     linjen LOW, når der er fejl. Uden en pull-up ville linjen svæve,
//     når der ikke er fejl, og interruptet ville udløses tilfældigt.
//     INPUT_PULLUP sikrer, at linjen er HIGH i normal tilstand og
//     kun går LOW ved fejl.
//
//   * HVORFOR BRUGES FALLING OG IKKE RISING?
//     Fordi optokobleren er aktiv LOW. Når der er fejl, går linjen
//     fra HIGH til LOW — det er en faldende flanke. Havde man brugt
//     RISING, ville ISR'en aldrig udløses (linjen går aldrig fra LOW
//     til HIGH, mens fejlen varer). Det er REQ-SAF-5.02's dokumenterede
//     afvigelse.
//
//   * HVORFOR INITIALISERES TFT FØR LITTLEFS OG NVS?
//     Fordi TFT-initialisering tager længere tid end LittleFS og NVS.
//     Ved at gøre det først kan man se en sort skærm tidligt, hvilket
//     bekræfter, at TFT'en fungerer — det er en god indikator under
//     opstart. LittleFS og NVS kan vente, fordi de ikke påvirker
//     skærmen.
//
//   * HVORFOR BRUGES LittleFS.begin(true)?
//     Fordi true betyder "formatér, hvis jomfrueligt". Det er nødvendigt
//     ved første opstart (hvor partitionen er tom). Hvis man brugte
//     false, ville LittleFS fejle på jomfruelige partitioner, og
//     logging ville være deaktiveret fra første opstart.
//
//   * HVORFOR INDLÆSES KALIBRERING FØR ADC INIT?
//     Fordi Task_Metrology (som starter senere) bruger
//     calibrationTable[]. Hvis ADC'en initialiseres først, kan
//     Task_Metrology nå at starte, FØR kalibreringsdata er klar —
//     hvilket ville give en forkert første måling. Ved at gøre det
//     i denne rækkefølge sikres det, at Task_Metrology altid har
//     gyldige data.
//
//   * HVORFOR KALDES Wire.begin() FØR adc.begin()?
//     Fordi adc.begin() bruger I2C-bussen. Hvis Wire.begin() ikke er
//     kaldt endnu, ville adc.begin() fejle eller hænge. Wire.begin()
//     initialiserer de to I2C-pins (SDA=21, SCL=22) og sætter
//     bus-hastigheden.
//
//   * HVORFOR SÆTTES adc.setGain(GAIN_ONE)?
//     Fordi GAIN_ONE giver full-scale ±4.096V, hvilket er det bredeste
//     spændingsområde. Det er nødvendigt for at kunne måle op til
//     1000V (efter 1:100 dæmpning). Hvis man brugte GAIN_TWO (±2.048V),
//     ville ADC'en overstyre ved høje spændinger. Det er
//     REQ-DSP-6.01's konfiguration.
//
//   * HVORFOR SÆTTES adc.setDataRate(RATE_ADS1115_475SPS)?
//     Det er buglist punkt 1-rettelsen. Adafruit's default er 128 SPS,
//     hvilket med vTaskDelay(10 ms) gav ~56 Hz sample-rate — for lavt
//     til at opfylde kravet om ~100 Hz. Med 475 SPS (2,1 ms konvertering)
//     + 10 ms delay opnås ~83 Hz, hvilket er tættere på kravet.
//     ADS1115 har ingen 100 SPS-indstilling; 475 SPS er nærmeste
//     standard over.
//
//   * HVORFOR STARTES Task_Metrology FØR Task_GUI_SCPI?
//     Fordi Task_Metrology skal have mulighed for at tage den første
//     måling og lægge den i dataQueue, FØR GUI'en forsøger at læse
//     fra den. Hvis GUI'en startede først, ville den første iteration
//     finde en tom kø — hvilket er harmløst, men giver en unødvendig
//     "tom" skærmopdatering. Rækkefølgen er ikke kritisk, men det er
//     rart at have metrologien klar først.
//
//   * HVORFOR 8192 BYTES TIL Task_Metrology OG 16384 TIL Task_GUI_SCPI?
//     Task_Metrology bruger 8192 bytes, fordi den har en lokal
//     filterBuffer[100] (400 bytes) plus stack til funktionskald
//     (ADC-læsning, kalibrering, auto-ranging). Det er rigeligt.
//     Task_GUI_SCPI bruger 16384 bytes, fordi den har store
//     String-objekter (SCPI-kommandoer kan være op til 64 tegn),
//     logBuffer[4096] (global, men tæt på), og TFT_eSPI-biblioteket
//     bruger en del stack. 16 KB er en sikker margin.
//
//   * HVORFOR PRIORITET 3 TIL METROLOGI OG 1 TIL GUI?
//     Fordi metrologien skal have højeste prioritet for at sikre
//     deterministisk sample-rate. GUI'en kan vente, hvis metrologien
//     har brug for CPU. Prioritet 3 er den højeste i dette system;
//     prioritet 1 er den laveste (FreeRTOS idle-task kører ved
//     prioritet 0).
//
//   * HVORFOR PINNES TASKS TIL HVER SIN KERNE?
//     Fordi ESP32 har to kerner, og opgaven med at sample (Core 1)
//     er tidskritisk. Hvis metrologien delte kerne med GUI'en, ville
//     en langsom TFT-opdatering forsinke sample-loopen og ødelægge
//     50 Hz NMRR-filteret. Ved at pinne hver task til sin kerne
//     sikres det, at de ikke konkurrerer om CPU.
//
//   * SIDE-EFFEKTER:
//     - Åbner Serial (USB-CDC).
//     - Konfigurerer 5 GPIO-pins som OUTPUT.
//     - Konfigurerer 1 GPIO-pin som INPUT_PULLUP.
//     - Tilkobler ISR til PIN_FAULT_INT.
//     - Opretter 3 FreeRTOS-mutexer.
//     - Opretter 1 FreeRTOS-kø.
//     - Starter 2 FreeRTOS-tasks.
//     - Initialiserer TFT, LittleFS, Wire, ADC.
//     - Indlæser kalibreringsdata fra NVS.
//     - Sætter relæerne i safe state.
//     - Skriver evt. fejlmeddelelser til Serial.
//
//   * RETURVÆRDI:
//     Ingen (void). setup() returnerer aldrig en værdi — Arduino
//     runtime ignorerer returværdien.
//
//   * FORHOLDSREGEL VED ÆNDRING AF PIN-MAPPING:
//     Hvis pin-mapping ændres (Sektion 2), skal man sikre, at:
//       - De nye pins er konfigureret som OUTPUT i setup().
//       - PIN_FAULT_INT er INPUT_PULLUP.
//       - ISR'en stadig kan bruge GPIO.out_w1tc (kun GPIO 0-31).
//     Ellers vil relæerne eller ISR'en ikke fungere korrekt.
//
//   * FORHOLDSREGEL VED ÆNDRING AF ADC-KONFIGURATION:
//     Hvis man ændrer gain eller datarate, skal man opdatere:
//       - getUpRangeThresholdForMode() / getDownRangeThresholdForMode()
//         (Sektion 7) — grænserne er baseret på full-scale.
//       - Task_Metrology's konverteringsfaktor (0.000125f) — den er
//         baseret på GAIN_ONE.
//       - Kravspecifikationen (REQ-DSP-6.01).
//
//   * FORHOLDSREGEL VED ÆNDRING AF TASK-STØRRELSE:
//     Hvis man ændrer 8192 eller 16384, skal man være opmærksom på:
//       - For lidt RAM = stack overflow (ESP32 crasher eller genstarter).
//       - For meget RAM = spild af heap (påvirker andre allokeringer).
//     Test altid med en debug-build, der tjekker stack-forbrug.
//
//   * FORHOLDSREGEL VED ÆNDRING AF TASK-PRIORITET:
//     Hvis man ændrer prioritet 3 eller 1, skal man være opmærksom på:
//       - Metrologien SKAL have højere prioritet end GUI'en, ellers
//         kan GUI'en forsinke sample-loopen.
//       - For høj prioritet (f.eks. 5) kan sulte FreeRTOS's
//         system-tasks (idle, timer).
//     Prioritet 3 er valgt som en balance mellem metrologi og system.
//
//   * SAMMENHÆNG MED loop():
//     loop() er bevidst tom (Sektion 16). Al funktionalitet ligger i
//     de to RTOS-tasks, som setup() starter. Hvis man tilføjer kode
//     til loop(), vil den køre på Core 1 (Arduino's default) sammen
//     med Task_Metrology — hvilket kan forsinke metrologien. Det
//     frarådes.
//
//   * SAMMENHÆNG MED hardwareFaultISR:
//     ISR'en tilkobles i trin 10. Hvis man ændrer ISR'ens adfærd,
//     skal man sikre, at den stadig kan kaldes fra en FALLING-flanke
//     på PIN_FAULT_INT. Hvis pin-mapping ændres, skal ISR'ens GPIO-
//     register-adgang (GPIO.out_w1tc) også opdateres (kun GPIO 0-31).
//
//   * SAMMENHÆNG MED Task_Metrology OG Task_GUI_SCPI:
//     De to tasks startes i trin 21 og 22. De deler data via
//     dataQueue og volatile globals. Hvis man ændrer MeasurementPacket
//     eller de globale variable, skal begge tasks opdateres — ellers
//     vil de læse/skrive forkerte data.
//
// -------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);            // Start den serielle port (USB), så vi kan tale SCPI med PC'en.

    pinMode(PIN_RELAY_DIV10, OUTPUT); // Gør 1:10 relæ-benet til en udgang.
    pinMode(PIN_RELAY_DIV100, OUTPUT); // Gør 1:100 relæ-benet til en udgang.
    pinMode(PIN_RELAY_AC_ROUTE, OUTPUT); // Gør AC-rute relæ-benet til en udgang.
    pinMode(PIN_RELAY_OHM, OUTPUT); // Gør Ohm-teststrøm relæ-benet til en udgang.
    pinMode(PIN_BUZZER, OUTPUT);  // Gør buzzer-benet til en udgang.

    // -- KONFIGURER LEDC TIL BUZZER (ESP32 core 3.x API) --
    // RETTELSE (buglist punkt 2): ledcSetup() er fjernet i core 3.x. Vi bruger
    // ledcAttach(pin, freq, resolution), der knytter LEDC til pin'en direkte.
    ledcAttach(PIN_BUZZER, 2000, BUZZER_LEDC_RESOLUTION); // Bind LEDC til buzzer-pin (GPIO 26) ved 2 kHz, 8-bit.
    // RETTELSE (buglist punkt 2): ledcWriteTone() tager pin i core 3.x (ikke kanal).
    ledcWriteTone(PIN_BUZZER, 0); // Sørg for at buzzeren er stille ved opstart (frekvens 0 = stop).

    relayMutex = xSemaphoreCreateMutex(); // Opret mutex der beskytter relæ-tilstand.
    calibrationMutex = xSemaphoreCreateMutex(); // Opret mutex der beskytter kalibreringstabellen.
    trendMutex = xSemaphoreCreateMutex(); // Opret mutex der beskytter trend-bufferen.

    forceRelaysToSafeState();     // Sæt alle relæer i sikker 1000V-tilstand fra første øjeblik.

    pinMode(PIN_FAULT_INT, INPUT_PULLUP); // Sæt fejl-lytte-benet med en op-træksmodstand (optokobler er aktiv LOW).
    attachInterrupt(digitalPinToInterrupt(PIN_FAULT_INT), hardwareFaultISR, FALLING); // Lyt efter FALDENDE flanke fra optokobler.

    tft.init();                   // Klargør grafikprocessoren.
    tft.setRotation(0);           // Brug rotation 0 — 800×480 er landskab uden ombytning.
    tft.fillScreen(TFT_BLACK);    // Mal skærmen helt sort ved opstart.

    if (!LittleFS.begin(true)) {  // Start filsystemet, formater det (true) hvis det er jomfrueligt.
        Serial.println("LittleFS Mount Failed"); // Fortæl over serial, hvis det mislykkedes.
        loggingEnabled = false;   // Slå logging fra, hvis filsystemet ikke kan startes.
    }

    loadCalibrationFromNvs();     // Indlæs alle kalibreringskonstanter fra NVS til RAM (REQ-CAL-13.02).

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); // Start I2C-bussen på de korrekte ben.
    if (!adc.begin(0x48)) {       // 0x48 er I2C-adressen på ADS1115 (ADDR-ben forbundet til GND).
        Serial.println("ADS1115 not found"); // Fortæl hvis ADC'en ikke svarer.
    }
    adc.setGain(GAIN_ONE);        // Sæt ADC'ens indre forstærker til ±4.096V for maksimal udnyttelse (REQ-DSP-6.01).
    // RETTELSE (buglist punkt 1): Sæt datarate eksplicit til 475 SPS (Adafruit default er 128 SPS).
    // Med 475 SPS tager hver konvertering ~2,1 ms. Sammen med vTaskDelay(10 ms) i
    // metrologi-loopet giver det en reel loop-tid på ~12 ms → ~83 Hz, hvilket er
    // tættere på kravet end de tidligere ~56 Hz (128 SPS + 10 ms delay).
    // Bemærk: ADS1115 har ingen 100 SPS-indstilling; 475 SPS er nærmeste standard over.
    adc.setDataRate(RATE_ADS1115_475SPS); // Sæt ADC-datarate til 475 SPS.

    dataQueue = xQueueCreate(10, sizeof(MeasurementPacket)); // Opret data-rør med plads til 10 pakker.

    xTaskCreatePinnedToCore(     // Start hardware-opgaven på Kerne 1.
        Task_Metrology,           // Navnet på den asynkrone funktion vi vil køre.
        "MetrologyTask",          // Et menneskelæseligt navn til debugging.
        8192,                     // Giv opgaven 8 Kilobyte RAM til arbejdet.
        NULL,                     // Vi sender ingen start-parametre.
        3,                        // Prioritet 3 (meget høj, vigtigere end grafik).
        NULL,                     // Vi har ikke brug for at gemme taskens ID.
        1                         // Lås tasken fast til Kerne 1.
    );

    xTaskCreatePinnedToCore(     // Start bruger-opgaven på Kerne 0.
        Task_GUI_SCPI,            // Funktionen der skal køre UI og USB SCPI-parsing.
        "GUITask",                // Internt navn.
        16384,                    // Mere RAM (16KB) til store tekststrenge og grafik.
        NULL,                     // Ingen startparametre.
        1,                        // Lav prioritet (1), så den viger for metrologien.
        NULL,                     // Intet ID gemt.
        0                         // Lås til Kerne 0.
    );
}

// =========================================================================
// SEKTION 16: STANDARD LOOP — REQ-RTOS-4.02
// =========================================================================

// Standard loop() efterlades TOM i dette ESP32 FreeRTOS-design, da arbejdet er allokeret til Tasks.
void loop() {
    // Der er ingen kode her. RTOS styrer maskinen.
}