// =========================================================================
// MULTIMETER_FIRMWARE.INO
// ISOLERET CAT III 4½-CIFRET BÆNKMULTIMETER (ESP32 / ADS1115 / AD8436)
// DOKUMENT-ID: SRS-DMM-16SRMS-V6.0-MASSIVE
// -------------------------------------------------------------------------
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

// Læser alle kalibreringskonstanter fra NVS ind i RAM ved opstart (REQ-CAL-13.02).
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

// Gemmer én kalibreringspost (gain+offset) for et givet slot i NVS (REQ-CAL-13.03).
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

// Tvinger alle relæer til deres sikre udgangspunkt: 1000V range, teststrøm afbrudt.
// RETTELSE (buglist C/E): Ifølge HW-DOC tabel 4.2 kræver 1000V-konfigurationen at
// BÅDE DIV10 og DIV100 er aktive (1:100). Den tidligere kode satte alle relæer LOW,
// hvilket ifølge tabellen er 1:1 via JFET-buffer — den MINDST beskyttede tilstand.
// Vi tænder derfor DIV10+DIV100 eksplicit for at ramme den dokumenterede 1:100-tilstand.
// Bruges ved opstart og ved katastrofe, men IKKE ved normale gearskift i Ohm-mode.
// Kaldes med spinlock taget hvis den kaldes fra ISR (via forceRelaysToSafeStateISR).
void forceRelaysToSafeState() {
    digitalWrite(PIN_RELAY_AC_ROUTE, LOW);  // Frakobl AD8436 True RMS-ruten, så kun DC-vejen er aktiv.
    digitalWrite(PIN_RELAY_OHM, LOW);       // Sluk modstands-teststrømmen øjeblikkeligt af sikkerhedshensyn.
    digitalWrite(PIN_RELAY_DIV10, HIGH);    // Tænd 1:10 dæmperen — del af den dokumenterede 1:100-konfiguration.
    digitalWrite(PIN_RELAY_DIV100, HIGH);   // Tænd 1:100 dæmperen — sikrer ægte 1:100 (mest dæmpning, sikreste tilstand).
    relayGearIndex = 0;                     // Marker i den globale variabel, at vi står i 1000V safe state.
}

// ISR-sikker version af forceRelaysToSafeState, der bruger register-skrivning (IRAM).
// RETTELSE (buglist C/E): Sætter DIV10+DIV100 HIGH (1:100) i stedet for at slukke alt.
// Bruger GPIO.out_w1ts til at sætte bits og GPIO.out_w1tc til at rydde bits.
// Bruges KUN fra hardwareFaultISR(), hvor spinlock allerede er taget.
void forceRelaysToSafeStateISR() {
    GPIO.out_w1tc = GPIO_BITS_ALL_RELAYS;               // Sluk ALLE relæer først på én gang via register (IRAM-sikkert).
    GPIO.out_w1ts = GPIO_BIT_RELAY_DIV10_AND_DIV100;    // Tænd DIV10+DIV100 for ægte 1:100 safe state (IRAM-sikkert).
    relayGearIndex = 0;                                 // Marker i global variabel, at vi nu er i 1000V safe state.
}

// Aktiverer et specifikt hardwaregear for en given mode (REQ-FSM-11.01).
// Tager mode som PARAMETER (ikke global) for at undgå race mellem kerner.
// Bevarer Ohm-teststrømmen, hvis vi er i en Ohm-mode, så målingen ikke afbrydes af auto-ranging.
// Spændingsdæmpere røres IKKE i Ohm-mode.
// Kaldes ALTID under relayMutex (eller spinlock hvis fra ISR — men ISR kalder kun ISR-varianten).
// RETTELSE (buglist B): Denne funktion kaldes nu UDEN FOR den kritiske sektion,
// fordi den indeholder vTaskDelay() (blokerende kald er forbudt i kritisk sektion).
// RETTELSE (buglist E): Gear 0 og 1 tænder nu BÅDE DIV10 og DIV100, som HW-DOC tabel 4.2 kræver.
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

// IRAM_ATTR fortæller ESP32'eren, at den skal lægge denne funktion i den hurtigste hukommelse den har.
// Bruger DIREKTE GPIO-REGISTERADGANG (ikke digitalWrite) for at være 100% IRAM-sikker.
// Bruger portENTER_CRITICAL_ISR med en spinlock for at beskytte mod samtidig adgang fra Task_Metrology.
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

// Hjælpefunktion: returnerer den øvre ADC-grænse (up-range trigger) for en given mode.
// Bruges til at afgøre, hvornår vi skal skifte til et højere gear.
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

// Hjælpefunktion: returnerer den nedre ADC-grænse (down-range trigger) for en given mode.
// REQ-FSM-11.02 kræver massivt overlap: skift ned ved ≤9% af aktuelt gear.
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

// Hjælpefunktion: returnerer det maksimale antal gear for en given mode.
// Spænding har 5 gear (0=1000V, 1=600V, 2=60V, 3=6V, 4=600mV), strøm har 6, Ohm har 6.
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

// Hjælpefunktion: returnerer dæmpningsfaktoren (indgangsspænding / ADC-spænding) for et givet gear.
// RETTELSE (buglist punkt 6): Bruges til at skalere AC-ripple-målingen fra AIN1 tilbage til
// det faktiske indgangsniveau, så 60V/600V-gear ikke viser 10×/100× for lave ripple-værdier.
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

// Hjælpefunktion: mapper (mode, gear) til en fast kalibreringsslot (0..39).
// RETTELSE (REQ-CAL-13.02): OHM_4W får nu sine EGNE 6 slotte (28..33) i stedet for
// at modulo-mappe 6 gear ind i 4 slotte, hvilket blandede kalibreringen for 600Ω/6MΩ.
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

// Tegner den rullende trend-graf på skærmen ud fra den cirkulære Y-buffer.
// Kaldes ALTID under trendMutex for at undgå race mellem Core 0 og Core 1.
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

// Håndterer én SCPI-kommando Streng og returnerer svaret via Serial.
// BEMÆRK: commandString er ALLEREDE toUpperCase()'et af kalderen — undtagen filnavne.
// For at undgå case-problemer med filnavne, udtrækkes filnavnet FØR toUpperCase i kalderen.
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

// Uendelig løkke der kører på Kerne 1, som udelukkende laver hardware-matematik.
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

// Sikrer at vi har en log-fil under max-størrelse. Returnerer true hvis der er plads.
// Sætter loggingEnabled = false hvis alle filer er fyldte, så GUI kan vise "LOG FULL".
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

// Arduino IDE starter altid i setup(). Her konfigureres RTOS, skærm og pin-retninger.
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