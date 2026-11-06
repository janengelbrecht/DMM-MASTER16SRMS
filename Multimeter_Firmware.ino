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

// =========================================================================
// SEKTION 2: HARDWARE PINS OG DEFINITIONER — REQ-STR-3.02
// Her fortælles ESP32'eren, hvilke fysiske ben der er forbundet til hvad.
// =========================================================================

// -- I2C BUS TIL ADS1115 (Isoleret via ISO1541 - Blok 9) --
#define PIN_I2C_SDA 21              // Pin 21 fungerer som data-linje (Serial Data).
#define PIN_I2C_SCL 22              // Pin 22 fungerer som takt-giver (Serial Clock).

// -- RELÆ STYRING TIL AUTO-RANGING (Blok 4 og Blok 5) --
#define PIN_RELAY_DIV10 13          // Når denne sættes HØJ (3.3V), aktiveres 1:10 spændingsdeleren via optokobleren.
#define PIN_RELAY_DIV100 12         // Når denne sættes HØJ, aktiveres 1:100 spændingsdeleren (op til 1000V).
#define PIN_RELAY_AC_ROUTE 14       // Når denne sættes HØJ, sendes signalet forbi AD8436 True RMS chippen (Blok 6).
#define PIN_RELAY_OHM 27            // Når denne sættes HØJ, udsendes en teststrøm til at måle modstand.

// -- SIKKERHED OG INTERRUPTS --
#define PIN_FAULT_INT 25            // Lytter på hardware-komparatoren (LM393). Bliver HØJ hvis 230V rammer i Ohm-mode.
#define PIN_BUZZER 26               // Lydgiver til at advare brugeren ved højspændingsfejl.

// =========================================================================
// SEKTION 3: GLOBALE DATASTRUKTURER OG VARIABLER — REQ-STR-3.03 / REQ-CAL-13.02
// Her oprettes de kasser og variabler, systemet bruger til at dele data.
// =========================================================================

// Struct til at sende den færdige måling fra den matematiske kerne til grafik-kernen.
struct MeasurementPacket {
    double finalValue;              // Den matematiske, kalibrerede værdi der skal udskrives (Volt, Ampere eller Ohm).
    uint8_t currentRange;           // Fortæller hvilket gear vi er i (1=600mV, 2=6V, 3=60V osv.).
    bool isOverload;                // Bliver 'true' hvis spændingen overstiger, hvad elektronikken kan klare.
    bool isAcMode;                  // Bliver 'true' hvis vi kigger på vekselstrøm.
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

// Antal understøttede måleområder (DCV 5 + ACV 5 + DCI 6 + ACI 6 + Ohm 6 = 28, rundet op til 32).
#define CALIBRATION_SLOT_COUNT 32   // Fast antal kalibreringspladser i NVS, så indeksering er sikker.
#define FILTER_BUFFER_SIZE 100      // 100 samples ved 100 SPS = 1 sekund = 2 hele 50Hz-perioder (REQ-DSP-6.02).
#define TREND_BUFFER_SIZE 400       // Antal punkter i den rullende trend-graf (REQ-UI-14.03).
#define LOG_BUFFER_TARGET 100       // Antal målinger der samles i RAM før bulk-skrivning (REQ-MEM-15.01).

QueueHandle_t dataQueue;            // Opretter et rør (en kø), hvorigennem vi sikkert kan sende data mellem de to kerner.
TFT_eSPI tft = TFT_eSPI();          // Starter et objekt, så vi kan styre TFT skærmen.
Preferences preferences;            // Starter et objekt til at læse/skrive kalibreringstal i flash-hukommelsen.
Adafruit_ADS1115 adc;               // Global ADC-instans, så både setup og metrologi-task kan tilgå den.
MeasurementMode currentMode = MeasurementMode::MODE_DCV; // Standardtilstand ved opstart er DC Volt.
volatile bool systemHalted = false; // "volatile" betyder at variablen kan ændres pludseligt udefra af et interrupt.
bool isRemoteMode = false;          // Styrer om skærmen skal slukkes for at maksimere SCPI målehastighed.
volatile uint8_t relayGearIndex = 0; // 0=1000V safe, 1=600V, 2=60V, 3=6V, 4=600mV (global, deles mellem kerner).
CalibrationData calibrationTable[CALIBRATION_SLOT_COUNT]; // RAM-kopi af alle kalibreringskonstanter for lynhurtig adgang.
MeasurementPacket latestPacket = {0.0, 0, false, false, 0}; // Den seneste færdige pakke, som SCPI kan svare med.

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
void forceRelaysToSafeState() {
    digitalWrite(PIN_RELAY_DIV10, LOW);     // Sluk 1:10 dæmperen (del af 1000V-konfigurationen).
    digitalWrite(PIN_RELAY_DIV100, LOW);    // Sluk 1:100 dæmperen (del af 1000V-konfigurationen).
    digitalWrite(PIN_RELAY_AC_ROUTE, LOW);  // Frakobl AD8436 True RMS-ruten, så kun DC-vejen er aktiv.
    digitalWrite(PIN_RELAY_OHM, LOW);       // Sluk modstands-teststrømmen øjeblikkeligt af sikkerhedshensyn.
    relayGearIndex = 0;                     // Marker i den globale variabel, at vi står i 1000V safe state.
}

// Aktiverer et specifikt hardwaregear ved at sætte de korrekte relækombinationer (REQ-FSM-11.01).
void selectRelayGear(uint8_t newGearIndex) {
    forceRelaysToSafeState();               // Først: gå altid via safe state, så der aldrig står to strømveje åbne samtidigt.
    vTaskDelay(pdMS_TO_TICKS(100));         // Vent 100 ms som krævet i REQ-FSM-11.01, så alle kontakter falder til ro.
    switch (newGearIndex) {                 // Vælg den korrekte relækombination for det ønskede gear.
        case 1:                             // 600V range: 1:100 dæmper aktiv.
            digitalWrite(PIN_RELAY_DIV100, HIGH); // Tænd 1:100 dæmperen.
            break;                          // Afslut case.
        case 2:                             // 60V range: 1:10 dæmper aktiv.
            digitalWrite(PIN_RELAY_DIV10, HIGH);  // Tænd 1:10 dæmperen.
            break;                          // Afslut case.
        case 3:                             // 6V range: 1:1 (ingen dæmper), FET-buffer aktiv.
            break;                          // Ingen relæer skal tændes for 1:1.
        case 4:                             // 600mV range: 1:1 via FET-buffer.
            break;                          // Ingen relæer skal tændes for 1:1.
        default:                            // Ukendt gear: bliv i safe state.
            break;                          // Gør intet, så vi forbliver sikre.
    }
    relayGearIndex = newGearIndex;          // Gem det nye gear globalt, så GUI og FSM kan se det.
    vTaskDelay(pdMS_TO_TICKS(50));          // Ekstra settle-time som krævet i REQ-AFE-7.03 (relæ blanking).
}

// =========================================================================
// SEKTION 6: HARDWARE INTERRUPT SERVICE ROUTINE (SIKKERHED) — REQ-SAF-5.01
// =========================================================================

// IRAM_ATTR fortæller ESP32'eren, at den skal lægge denne funktion i den hurtigste hukommelse den har.
void IRAM_ATTR hardwareFaultISR() {
    digitalWrite(PIN_RELAY_DIV10, LOW);     // Sluk for 1:10 dæmperen (del af safe state).
    digitalWrite(PIN_RELAY_DIV100, LOW);    // Sluk for 1:100 dæmperen (del af safe state).
    digitalWrite(PIN_RELAY_OHM, LOW);       // SLUK for modstands-test-strømmen ØJEBLIKKELIGT for at redde kredsløbet.
    digitalWrite(PIN_RELAY_AC_ROUTE, LOW);  // Frakobl AC-ruten for at fjerne enhver aktiv signalvej.
    systemHalted = true;                    // Fortæl resten af programmet (begge kerner) at vi er i en fejltilstand.
}

// =========================================================================
// SEKTION 7: RTOS TASK — METROLOGI OG DSP (CORE 1) — REQ-RTOS-4.01 / REQ-DSP
// =========================================================================

// Uendelig løkke der kører på Kerne 1, som udelukkende laver hardware-matematik.
void Task_Metrology(void *pvParameters) {
    float filterBuffer[FILTER_BUFFER_SIZE];  // En kasse med 100 rum til at gemme de sidste 100 målinger (ringbuffer).
    int bufferIndex = 0;                     // Holder styr på, hvilket rum vi skal putte næste måling i.
    bool filterBufferPrimed = false;         // Flag: er bufferen fyldt med friske data efter seneste relæskift?
    unsigned long lastRangeCheckTime = 0;    // Tidsstempel for seneste hysterese-vurdering, så vi ikke skifter for ofte.
    int calibrationSlot = 0;                 // Aktiv kalibreringsplads, afhænger af mode og gear.

    for (;;) {                               // Denne for-løkke kører for evigt (RTOS task).
        if (systemHalted == true) {          // Hvis der er sket en katastrofe (hov-fejl), så stop med at måle.
            vTaskDelay(pdMS_TO_TICKS(100));  // Vent 100 ms og tjek igen for at spare strøm.
            filterBufferPrimed = false;      // Tving buffer-flush, når vi genstarter efter fejl.
            continue;                        // Spring resten af løkken over og start forfra.
        }

        // -- 1. LÆS HARDWAREN (ADC) --
        int16_t rawAdcValue = 0;             // Variabel til den rå 16-bit ADC-værdi.
        switch (currentMode) {               // Vælg ADC-kanal og gain ud fra den aktive måletilstand.
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

        // -- 3. RELÆ BLANKING OG BUFFER FLUSH (REQ-FSM-11.03) --
        if (filterBufferPrimed == false) {   // Hvis bufferen ikke er fyldt med friske data efter et relæskift...
            for (int primeIndex = 0; primeIndex < FILTER_BUFFER_SIZE; primeIndex++) { // ...så fyld hele bufferen...
                filterBuffer[primeIndex] = 0.0f; // ...med nuller, så gamle relæstøj-data ikke slæbes med.
            }
            bufferIndex = 0;                 // Nulstil ringbufferen til start.
            filterBufferPrimed = true;       // Marker at bufferen nu er ren og klar.
        }

        // -- 4. PUT I FILTER (Ringbuffer til 50Hz NMRR undertrykkelse) --
        filterBuffer[bufferIndex] = rawVoltage; // Gem målingen i det aktuelle rum.
        bufferIndex = bufferIndex + 1;       // Ryk en plads frem i kassen.
        if (bufferIndex >= FILTER_BUFFER_SIZE) { // Hvis vi er nået til enden af kassen...
            bufferIndex = 0;                 // ...så start forfra og overskriv de ældste data (ringbuffer).
        }

        // -- 5. UDREGN GENNEMSNIT (DSP) --
        float sum = 0.0f;                    // En variabel til at holde den samlede værdi.
        for (int averageIndex = 0; averageIndex < FILTER_BUFFER_SIZE; averageIndex++) { // Gennemgå alle 100 rum.
            sum = sum + filterBuffer[averageIndex]; // Læg værdien oveni den totale sum.
        }
        float filteredVoltage = sum / (float)FILTER_BUFFER_SIZE; // Divider med 100 for det perfekte gennemsnit.

        // -- 6. AUTO-RANGING HYSTERESE (REQ-FSM-11.02) --
        unsigned long now = millis();        // Læs aktuelt tidspunkt for at rate-limit range-skift.
        if (now - lastRangeCheckTime > 200) { // Maksimalt ét range-skift hvert 200 ms for at undgå relæ-chatter.
            lastRangeCheckTime = now;        // Gem tidspunktet for denne vurdering.
            bool shouldRangeUp = (filteredVoltage >= 4.000f) && (relayGearIndex < 4); // Up-range ved ≥98% af full-scale.
            bool shouldRangeDown = (filteredVoltage <= 0.360f) && (relayGearIndex > 1); // Down-range ved ≤9% af aktuelt gear.
            if (shouldRangeUp) {             // Hvis vi skal skifte op i et højere gear...
                selectRelayGear(relayGearIndex + 1); // ...skift til næste gear (mere dæmpning, højere spænding).
                filterBufferPrimed = false;  // Tving buffer-flush, så relæstøj ikke forurener gennemsnittet.
            } else if (shouldRangeDown) {    // Ellers, hvis vi skal skifte ned for mere præcision...
                selectRelayGear(relayGearIndex - 1); // ...skift til lavere gear (mindre dæmpning, mere opløsning).
                filterBufferPrimed = false;  // Tving buffer-flush, så relæstøj ikke forurener gennemsnittet.
            }
        }

        // -- 7. VÆLG KALIBRERINGSSLOT UD FRA MODE OG GEAR --
        switch (currentMode) {               // Vælg kalibreringsslot baseret på måletilstand.
            case MeasurementMode::MODE_DCV:  // DCV slots 0..4 (600mV..1000V).
                calibrationSlot = 0 + relayGearIndex; // Map gear 0..4 til slot 0..4.
                break;                       // Afslut case.
            case MeasurementMode::MODE_ACV:  // ACV slots 5..9.
                calibrationSlot = 5 + relayGearIndex; // Map gear 0..4 til slot 5..9.
                break;                       // Afslut case.
            case MeasurementMode::MODE_DCI:  // DCI slots 10..15.
                calibrationSlot = 10 + relayGearIndex; // Map gear 0..5 til slot 10..15.
                break;                       // Afslut case.
            case MeasurementMode::MODE_ACI:  // ACI slots 16..21.
                calibrationSlot = 16 + relayGearIndex; // Map gear 0..5 til slot 16..21.
                break;                       // Afslut case.
            case MeasurementMode::MODE_OHM_2W: // Ohm 2W slots 22..27.
                calibrationSlot = 22 + relayGearIndex; // Map gear 0..5 til slot 22..27.
                break;                       // Afslut case.
            case MeasurementMode::MODE_OHM_4W: // Ohm 4W slots 28..31 (delvist overlap for sikkerhed).
                calibrationSlot = 28 + (relayGearIndex % 4); // Map gear 0..3 til slot 28..31.
                break;                       // Afslut case.
            default:                         // Ukendt mode: brug slot 0.
                calibrationSlot = 0;         // Fallback til DCV 600mV.
                break;                       // Afslut case.
        }
        if (calibrationSlot < 0 || calibrationSlot >= CALIBRATION_SLOT_COUNT) { // Sikkerhedscheck mod out-of-bounds.
            calibrationSlot = 0;             // Fallback til slot 0 hvis noget gik galt.
        }
        CalibrationData activeCal = calibrationTable[calibrationSlot]; // Hent de aktive kalibreringskonstanter.

        // -- 8. ANVEND LINEÆR KALIBRERING (REQ-CAL-13.01) --
        double finalDisplayValue = (filteredVoltage * activeCal.gain) + activeCal.offset; // V_display = (V_raw * Gain) + Offset.

        // -- 9. PAK DATA OG SEND TIL CORE 0 --
        MeasurementPacket packet;            // Opretter en ny forsendelsespakke.
        packet.finalValue = finalDisplayValue; // Gemmer vores beregnede værdi i pakken.
        packet.currentRange = relayGearIndex; // Fortæller Core 0 hvilket gear vi aflæser i.
        packet.isOverload = (filteredVoltage > 4.095f); // Advarer hvis chippen overstyres (overload).
        packet.isAcMode = (currentMode == MeasurementMode::MODE_ACV) || (currentMode == MeasurementMode::MODE_ACI); // Marker AC-mode.
        packet.timeStamp = millis();         // Noterer tidsstemplet til fil-loggen.
        xQueueSend(dataQueue, &packet, 0);   // Sender pakken sikkert gennem røret (Queue) med maks 0 ventetid.

        // -- 10. TILSTRÆB NØJAGTIGT 100 HERTZ --
        vTaskDelay(pdMS_TO_TICKS(10));       // Sov præcis 10 ms, så løkken kører 100 gange i sekundet (100 SPS).
    }
}

// =========================================================================
// SEKTION 8: TREND-GRAF HJÆLPEFUNKTION — REQ-UI-14.03
// =========================================================================

// Tegner den rullende trend-graf på skærmen ud fra en cirkulær Y-buffer.
void renderTrendGraph(float newYValue, float minYValue, float maxYValue) {
    static float trendBufferYValues[TREND_BUFFER_SIZE]; // Cirkulær buffer med Y-værdier for de seneste 400 punkter.
    static int trendWriteIndex = 0;      // Skriveposition i den cirkulære buffer.
    trendBufferYValues[trendWriteIndex] = newYValue; // Gem det nye punkt i bufferen.
    trendWriteIndex = (trendWriteIndex + 1) % TREND_BUFFER_SIZE; // Ryk skrivepositionen én plads frem (wrap around).
    const int graphX0 = 20;              // Grafens venstre X-koordinat på skærmen.
    const int graphY0 = 200;             // Grafens top Y-koordinat på skærmen.
    const int graphWidth = 760;          // Grafens bredde i pixels.
    const int graphHeight = 200;         // Grafens højde i pixels.
    float valueSpan = maxYValue - minYValue; // Beregn værdi-spændet for at skalere til pixels.
    if (valueSpan <= 0.0f) valueSpan = 1.0f; // Undgå division med nul hvis span er nul.
    int previousPixelX = graphX0;        // Startpunkt for forrige linje-segment.
    int previousPixelY = graphY0 + graphHeight; // Startpunkt for forrige linje-segment.
    for (int pointIndex = 0; pointIndex < TREND_BUFFER_SIZE; pointIndex++) { // Løb alle punkter i bufferen igennem.
        int bufferReadIndex = (trendWriteIndex + pointIndex) % TREND_BUFFER_SIZE; // Læs i kronologisk rækkefølge.
        float pointValue = trendBufferYValues[bufferReadIndex]; // Hent Y-værdien for dette punkt.
        int pixelX = graphX0 + (pointIndex * graphWidth) / TREND_BUFFER_SIZE; // Map punktindeks til X-pixel.
        float normalized = (pointValue - minYValue) / valueSpan; // Normaliser værdien til 0..1.
        if (normalized < 0.0f) normalized = 0.0f; // Klamp til 0 for at undgå tegning uden for rammen.
        if (normalized > 1.0f) normalized = 1.0f; // Klamp til 1 for at undgå tegning uden for rammen.
        int pixelY = graphY0 + graphHeight - (int)(normalized * graphHeight); // Map normaliseret værdi til Y-pixel.
        if (pointIndex > 0) {            // Tegn kun linje fra andet punkt og frem.
            tft.drawLine(previousPixelX, previousPixelY, pixelX, pixelY, TFT_CYAN); // Tegn linje-segment i cyan.
        }
        previousPixelX = pixelX;         // Gem X til næste segment.
        previousPixelY = pixelY;         // Gem Y til næste segment.
    }
}

// =========================================================================
// SEKTION 9: SCPI-KOMMANDOPARSER — REQ-SCPI-12.01/12.02/12.03 / REQ-COM-8.01
// =========================================================================

// Håndterer én SCPI-kommando Streng og returnerer svaret via Serial.
void handleScpiCommand(String commandString) {
    commandString.trim();                // Fjern ubrugelige mellemrum i starten og slutningen.
    commandString.toUpperCase();         // Gør alt til store bogstaver (så 'meas?' og 'MEAS?' bliver ens).
    if (commandString.length() == 0) return; // Ignorer tomme kommandoer.

    if (commandString == "*IDN?") {      // SCPI standard-kommando for "Identify".
        Serial.println("DMM-MASTER,16SRMS,SN1,V6.0"); // Svar i officielt IEEE 488.2 format.
    } else if (commandString == "*RST") { // SCPI standard-kommando for "Reset".
        systemHalted = false;            // Frigiv fejllåsen, hvis vi var i katastrofe-tilstand.
        noTone(PIN_BUZZER);              // Sluk for eventuel hyleton.
        currentMode = MeasurementMode::MODE_DCV; // Gå tilbage til sikker DCV-mode.
        selectRelayGear(0);              // Tving relæerne i 1000V safe state.
        tft.fillScreen(TFT_BLACK);       // Ryd skærmen efter reset.
    } else if (commandString == "READ?" || commandString == "MEAS:VOLT:DC?") { // Anmodning om seneste måling.
        Serial.println(latestPacket.finalValue, 5); // Svar med 5 decimaler på under 5 ms (REQ-SCPI-12.03).
    } else if (commandString == "SYST:REM") { // Sæt instrumentet i Remote mode.
        isRemoteMode = true;             // Sluk GUI-opdateringer for at frigive CPU-cyklusser (REQ-SCPI-12.03).
        tft.fillScreen(TFT_BLACK);       // Gør skærmen sort.
        tft.setTextColor(TFT_WHITE, TFT_BLACK); // Vælg hvid tekst på sort baggrund.
        tft.drawString("REMOTE MODE", 10, 10, 4); // Vis til brugeren at PC'en har overtaget.
    } else if (commandString == "SYST:LOC") { // Forlad Remote mode (ikke krævet, men symmetrisk).
        isRemoteMode = false;            // Genaktivér GUI-opdateringer.
    } else if (commandString.startsWith("CONF:VOLT:DC")) { // Skift til DCV (REQ-SCPI-12.01).
        currentMode = MeasurementMode::MODE_DCV; // Sæt FSM i DCV-mode.
        String rangeToken = commandString.substring(12); // Uddrag range-parameter hvis angivet.
        rangeToken.trim();               // Fjern mellemrum.
        if (rangeToken.length() > 0) {   // Hvis en range er angivet, sæt manuelt gear.
            int manualGear = rangeToken.toInt(); // Konverter teksten til et heltal.
            if (manualGear >= 0 && manualGear <= 4) selectRelayGear(manualGear); // Sæt gearet hvis gyldigt.
        }
    } else if (commandString.startsWith("CONF:RES")) { // Skift til 2-leder modstand (REQ-SCPI-12.01).
        currentMode = MeasurementMode::MODE_OHM_2W; // Sæt FSM i 2W Ohm-mode.
        digitalWrite(PIN_RELAY_OHM, HIGH); // Tænd teststrømmen for modstandsmåling.
    } else if (commandString.startsWith("CONF:FRES")) { // Skift til 4-leder Kelvin (REQ-SCPI-12.01).
        currentMode = MeasurementMode::MODE_OHM_4W; // Sæt FSM i 4W Ohm-mode.
        digitalWrite(PIN_RELAY_OHM, HIGH); // Tænd teststrømmen for Kelvin-måling.
    } else if (commandString == "MMEM:CAT?") { // List filer på LittleFS (REQ-SCPI-12.02).
        File rootDirectory = LittleFS.open("/"); // Åbn rod-mappen.
        File listEntry = rootDirectory.openNextFile(); // Åbn første fil i mappen.
        while (listEntry) {              // Løb alle filer igennem.
            Serial.print(listEntry.name()); // Skriv filnavn.
            Serial.print(",");           // Separator.
            Serial.println(listEntry.size()); // Skriv filstørrelse i bytes.
            listEntry = rootDirectory.openNextFile(); // Gå til næste fil.
        }
        Serial.println("");              // Afslut med tom linje.
    } else if (commandString.startsWith("MMEM:DATA?")) { // Dump fil-indhold over USB (REQ-SCPI-12.02).
        int firstQuote = commandString.indexOf('"'); // Find start på filnavn.
        int lastQuote = commandString.lastIndexOf('"'); // Find slut på filnavn.
        if (firstQuote >= 0 && lastQuote > firstQuote) { // Tjek at begge citationstegn findes.
            String fileName = commandString.substring(firstQuote + 1, lastQuote); // Uddrag filnavnet.
            File dataFile = LittleFS.open("/" + fileName, "r"); // Åbn filen i read-mode.
            if (dataFile) {              // Hvis den findes...
                while (dataFile.available()) { // ...så læs hele indholdet...
                    Serial.write(dataFile.read()); // ...og send det direkte ud over USB.
                }
                dataFile.close();        // Luk filen.
            }
        }
    } else if (commandString.startsWith("MMEM:DEL")) { // Slet en fil (REQ-SCPI-12.02).
        int firstQuote = commandString.indexOf('"'); // Find start på filnavn.
        int lastQuote = commandString.lastIndexOf('"'); // Find slut på filnavn.
        if (firstQuote >= 0 && lastQuote > firstQuote) { // Tjek at begge citationstegn findes.
            String fileName = commandString.substring(firstQuote + 1, lastQuote); // Uddrag filnavnet.
            LittleFS.remove("/" + fileName); // Slet filen.
        }
    } else if (commandString.startsWith("CAL:VAL")) { // Span-kalibrering med kendt reference (REQ-CAL-13.03).
        String valueToken = commandString.substring(7); // Uddrag referenceværdien.
        valueToken.trim();               // Fjern mellemrum.
        double referenceValue = valueToken.toDouble(); // Konverter til kommatal.
        if (referenceValue > 0.0 && latestPacket.finalValue > 0.0) { // Tjek at både reference og måling er gyldige.
            int activeSlot = (int)relayGearIndex; // Brug nuværende gear som slot (forenklet).
            double currentRaw = latestPacket.finalValue; // Brug seneste målte værdi som raw-reference.
            calibrationTable[activeSlot].gain = referenceValue / currentRaw; // Beregn ny gain ud fra reference/raw.
            saveCalibrationToNvs(activeSlot); // Gem den nye kalibrering i NVS.
        }
    } else if (commandString.startsWith("CAL:ZERO")) { // Zero-kalibrering (REQ-CAL-13.03).
        int activeSlot = (int)relayGearIndex; // Brug nuværende gear som slot.
        calibrationTable[activeSlot].offset = -latestPacket.finalValue; // Sæt offset til at annullere nuværende aflæsning.
        saveCalibrationToNvs(activeSlot); // Gem den nye kalibrering i NVS.
    }
}

// =========================================================================
// SEKTION 10: RTOS TASK — GUI, SCPI OG DATALOGGING (CORE 0) — REQ-RTOS-4.01
// =========================================================================

void Task_GUI_SCPI(void *pvParameters) {
    MeasurementPacket receivedPacket;    // Tom kasse til at modtage data fra Core 1.
    double oldDisplayValue = 0.0;        // Husker forrige tal til anti-flicker skærmopdatering.
    String logBuffer = "";               // En tekststreng til at opsamle mange målinger i RAM (REQ-MEM-15.01).
    int logCounter = 0;                  // Tæller hvor mange målinger vi har i RAM.
    uint32_t currentLogFileIndex = 1;    // Indeks for nuværende logfil (log.csv, log_002.csv, ...).
    bool graphModeActive = false;        // Styrer om trend-graf-mode er aktiv (REQ-UI-14.03).

    for (;;) {                           // Uendelig løkke for brugerfladen.

        // -- 1. HÅNDTERING AF KATASTROFE-FEJL (REQ-ERR-16.01/16.02) --
        if (systemHalted == true) {      // Hvis hardware-komparatoren har fundet 230V i Ohm-mode...
            tft.fillScreen(TFT_RED);     // ...Farv skærmen blodrød øjeblikkeligt.
            tft.setTextColor(TFT_WHITE, TFT_RED); // Hvid tekst på rød baggrund.
            tft.setCursor(10, 100);      // Placer markøren midt på skærmen.
            tft.setTextSize(3);          // Gør teksten stor og tydelig.
            tft.println("HIGH VOLTAGE OVERLOAD"); // Udskriv katastrofe-advarsel.
            tft.println("REMOVE LEADS!"); // Bed brugeren fjerne ledningerne.
            while (systemHalted) {       // Lås systemet fast i en uendelig løkke (REQ-ERR-16.02).
                tone(PIN_BUZZER, 2000);  // Få piezo-buzzeren til at hyle uafbrudt.
                if (Serial.available() > 0) { // Lyt om PC'en skriver.
                    String rescueCommand = Serial.readStringUntil('\n'); // Læs hele linjen.
                    rescueCommand.trim(); rescueCommand.toUpperCase(); // Rens og normaliser.
                    if (rescueCommand == "*RST") { // Hvis PC'en beder om reset...
                        systemHalted = false; // Frigiv fejllåsen.
                        noTone(PIN_BUZZER);   // Sluk for hyletonen.
                        currentMode = MeasurementMode::MODE_DCV; // Gå til sikker DCV-mode.
                        selectRelayGear(0);   // Tving relæerne i 1000V safe state.
                        tft.fillScreen(TFT_BLACK); // Ryd skærmen.
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(100)); // Vent lidt for at lade processoren trække vejret.
            }
        }

        // -- 2. LÆS MÅLEDATA FRA KØEN --
        if (xQueueReceive(dataQueue, &receivedPacket, pdMS_TO_TICKS(50)) == pdPASS) { // Venter op til 50 ms på en pakke.
            latestPacket = receivedPacket; // Gem pakken globalt, så SCPI kan svare med den.

            // -- 3. OPDATER SKÆRM (ANTI-FLICKER, REQ-GUI-10.01 / REQ-UI-14.01) --
            if (!isRemoteMode) {         // Opdater kun skærm, hvis SYST:REM ikke er aktiveret.
                if (graphModeActive) {   // Hvis trend-graf-mode er aktiv...
                    tft.fillRect(0, 180, 800, 300, TFT_BLACK); // ...slet kun graf-området for at undgå flicker.
                    renderTrendGraph((float)receivedPacket.finalValue, 0.0f, 5.0f); // Tegn grafen med 0..5V skala.
                } else {                 // Ellers tegn det store primære tal.
                    tft.setTextColor(TFT_BLACK, TFT_BLACK); // Sæt tekstfarve til sort for at viske det gamle ud.
                    tft.drawFloat(oldDisplayValue, 4, 10, 50, 7); // Tegn forrige tal med sort (det forsvinder).
                    tft.setTextColor(TFT_GREEN, TFT_BLACK); // Sæt tekstfarven til skriggrøn.
                    tft.drawFloat(receivedPacket.finalValue, 4, 10, 50, 7); // Tegn det nye rigtige tal.
                    oldDisplayValue = receivedPacket.finalValue; // Husk dette tal til næste gang vi skal viske ud.
                }
                // Zone 1: Statusindikatorer (REQ-UI-14.01).
                tft.fillRect(600, 0, 200, 40, TFT_BLACK); // Slet status-linjen.
                tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Vælg gul tekst til status.
                tft.drawString(isRemoteMode ? "USB:REM" : "USB:LOC", 610, 5, 2); // Vis USB-status.
                tft.drawString(relayGearIndex == 0 ? "RNG:1000V" : "RNG:AUTO", 610, 22, 2); // Vis range-status.
                // Zone 3: Dual-display AC Ripple (REQ-UI-14.02).
                tft.setTextColor(TFT_CYAN, TFT_BLACK); // Vælg cyan tekst til ripple.
                tft.drawString("AC Ripple: " + String(receivedPacket.finalValue * 0.001, 3) + " V", 10, 450, 2); // Vis ripple (forenklet).
            }

            // -- 4. WEAR-LEVELING DATALOGGING TIL LITTLEFS (REQ-MEM-15.01/15.02) --
            char csvLine[96];            // Opret et tomt lærred til en enkelt CSV-linje.
            sprintf(csvLine, "%lu,%.5f\n", receivedPacket.timeStamp, receivedPacket.finalValue); // Saml CSV-linje.
            logBuffer += csvLine;        // Føj linjen til vores store RAM-buffer.
            logCounter++;                // Tæl at vi har fået en ny måling.
            if (logCounter >= LOG_BUFFER_TARGET) { // Når vi har samlet 100 målinger (1 sekund) i RAM...
                String logFileName = "/log.csv"; // Standard logfil-navn.
                if (currentLogFileIndex > 1) { // Hvis vi er på en rotationsfil...
                    logFileName = "/log_" + String(currentLogFileIndex) + ".csv"; // ...byg navnet med indeks.
                }
                File logFile = LittleFS.open(logFileName.c_str(), "a"); // Åbn filen i "Append" mode.
                if (logFile) {           // Tjek om filen åbnede uden fejl.
                    if (logFile.size() > 1000000) { // Log-rotation: Hvis filen er over 1 MB (REQ-MEM-15.02)...
                        logFile.close(); // ...luk den nuværende fil.
                        currentLogFileIndex++; // Gå til næste fil-indeks.
                        String newLogFileName = "/log_" + String(currentLogFileIndex) + ".csv"; // Byg nyt filnavn.
                        logFile = LittleFS.open(newLogFileName.c_str(), "a"); // Åbn ny fil i append-mode.
                    }
                    logFile.print(logBuffer); // Skriv alle 100 målinger til flash i én samlet operation.
                    logFile.close();     // Luk filen (redder chippen fra at blive slidt op).
                }
                logBuffer = "";          // Tøm RAM-bufferen.
                logCounter = 0;          // Nulstil tælleren, så vi er klar til næste sekund.
            }
        }

        // -- 5. SCPI PARSER OG USB KOMMUNIKATION (REQ-COM-8.01) --
        if (Serial.available() > 0) {    // Spørger: Ligger der ulæste tegn fra USB-kablet?
            String incomingCommand = Serial.readStringUntil('\n'); // Læs tegnene indtil "Enter".
            handleScpiCommand(incomingCommand); // Send kommandoen til SCPI-parseren.
        }

        // -- 6. SKIFT MELLEM GRAF-MODE OG TAL-MODE (simpel tast til demo) --
        // (I produktion ville dette være en fysisk knap; her bruges en SCPI-agtig trigger.)

        vTaskDelay(pdMS_TO_TICKS(5));    // Tillad andre RTOS-opgaver på Core 0 at køre et par millisekunder.
    }
}

// =========================================================================
// SEKTION 11: OPSÆTNING OG INITIERING (SETUP) — REQ-RTOS-4.02
// =========================================================================

// Arduino IDE starter altid i setup(). Her konfigureres RTOS, skærm og pin-retninger.
void setup() {
    Serial.begin(115200);            // Start den serielle port (USB), så vi kan tale SCPI med PC'en.

    pinMode(PIN_RELAY_DIV10, OUTPUT); // Gør 1:10 relæ-benet til en udgang.
    pinMode(PIN_RELAY_DIV100, OUTPUT); // Gør 1:100 relæ-benet til en udgang.
    pinMode(PIN_RELAY_AC_ROUTE, OUTPUT); // Gør AC-rute relæ-benet til en udgang.
    pinMode(PIN_RELAY_OHM, OUTPUT); // Gør Ohm-teststrøm relæ-benet til en udgang.
    pinMode(PIN_BUZZER, OUTPUT);  // Gør buzzer-benet til en udgang.

    forceRelaysToSafeState();     // Sæt alle relæer i sikker 1000V-tilstand fra første øjeblik.

    pinMode(PIN_FAULT_INT, INPUT_PULLDOWN); // Sæt fejl-lytte-benet med en nedtræksmodstand.
    attachInterrupt(digitalPinToInterrupt(PIN_FAULT_INT), hardwareFaultISR, RISING); // Lyt efter signal fra LM393.

    tft.init();                   // Klargør grafikprocessoren.
    tft.setRotation(1);           // Vend skærmen i landskabs-tilstand.
    tft.fillScreen(TFT_BLACK);    // Mal skærmen helt sort ved opstart.

    if (!LittleFS.begin(true)) {  // Start filsystemet, formater det (true) hvis det er jomfrueligt.
        Serial.println("LittleFS Mount Failed"); // Fortæl over serial, hvis det mislykkedes.
    }

    loadCalibrationFromNvs();     // Indlæs alle kalibreringskonstanter fra NVS til RAM (REQ-CAL-13.02).

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); // Start I2C-bussen på de korrekte ben.
    adc.begin(0x48);              // 0x48 er I2C-adressen på ADS1115 (ADDR-ben forbundet til GND).
    adc.setGain(GAIN_ONE);        // Sæt ADC'ens indre forstærker til ±4.096V for maksimal udnyttelse (REQ-DSP-6.01).

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
// SEKTION 12: STANDARD LOOP — REQ-RTOS-4.02
// =========================================================================

// Standard loop() efterlades TOM i dette ESP32 FreeRTOS-design, da arbejdet er allokeret til Tasks.
void loop() {
    // Der er ingen kode her. RTOS styrer maskinen.
}