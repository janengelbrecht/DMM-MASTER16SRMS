========================================================================================================================================================================
SOFTWARE & FIRMWARE REQUIREMENTS SPECIFICATION (SRS) - ULTIMATE MASTER EDITION
========================================================================================================================================================================
DOKUMENT-ID:      SRS-DMM-16SRMS-V6.0-MASSIVE        										KLASSIFIKATION: OFFENTLIG / OPEN SOURCE
PROJEKT:          ISOLERET CAT III 4½-CIFRET BÆNKMULTIMETER (ESP32 / ADS1115 / AD8436)
SYSTEM-ARKITEKTUR: MCU / EXTERN ADC / TRUE RMS FRONTEND
------------------------------------------------------------------------------------------------------------------------------------------------------------------------
FAGOMRÅDE:        FIRMWARE & SOFTWARE CO-DESIGN       										LIVSCYKLUS:     MASTER SPECIFICATION
FORFATTER:        R&D & TEKNISK DOKUMENTATION         										IKRAFTTRÆDEN:   5. NOVEMBER 2026
MÅLGRUPPE:        UDVIKLERE
========================================================================================================================================================================

========================================================================================================================================================================
## 1.0 FUNDAMENTALE DOKUMENT- OG KODESTANDARDER (REQ-DOC)
========================================================================================================================================================================
Dette afsnit definerer de absolutte lovkrav for, hvordan koden skal udformes. Der kan *ikke* dispenseres fra disse krav.

* **\[REQ-DOC-1.01\] Den Monolitiske Arkitektur (Én Fil):**
  For at undgå den forvirring, der opstår ved at have variabler spredt over mange filer (header-filer, cpp-filer), 
  SKAL hele firmwaren skrives i **én og kun én tekstfil**. Denne fil skal navngives eksakt `Multimeter_Firmware.ino`. 
  Al opsætning, logik og brugerflade samles her.

* **\[REQ-DOC-1.02\] 100% Inline Kommenteringsgrad:**
  Det forudsættes, at den der læser koden i fremtiden, aldrig har set C++ før. 
  Derfor SKAL hver eneste kodelinje (undtagen tomme linjer til formatering) have en kommentar indledt med `//`.

  * **Regel:** Kommentaren skal forklare **HVAD** koden gør rent teknisk, og **HVORFOR** den gør det relateret til multimeterets hardware.

* **\[REQ-DOC-1.03\] Nul-Forudsætnings Princippet:**
  Ingen "smarte" programmør-forkortelser. En variabel må ikke hedde `int r;`, den skal hedde `int relayDivider10Pin;`. 
  Al logik skal kunne læses næsten som almindeligt engelsk/dansk.

========================================================================================================================================================================
## 2.0 ARDUINO IDE OPSÆTNING & MILJØ (REQ-ENV)
========================================================================================================================================================================
Dette afsnit guider udvikleren skridt for skridt gennem opsætningen af udviklingsmiljøet, inden koden overhovedet påbegyndes.

* **\[REQ-ENV-2.01\] IDE Version:** Udviklingen skal foregå i *Arduino IDE 2.x* (nyeste version).

* **\[REQ-ENV-2.02\] Installation af ESP32 Core:**

  1. Åbn `File -> Preferences`.

  2. I feltet "Additional Boards Manager URLs" indsættes: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`.

  3. Gå til `Tools -> Board -> Boards Manager`, søg på "esp32", og installer pakken.

* **\[REQ-ENV-2.03\] Board Valg:** Under `Tools -> Board` skal vælges præcist **ESP32 Dev Module**.

* **\[REQ-ENV-2.04\] Hukommelsesallokering (Kritisk!):**
  ESP32'eren har indbygget flash-hukommelse. Standardindstillingen tillader ikke filsystemer (LittleFS), som vi skal bruge til datalogging.

  * Under `Tools -> Partition Scheme` SKAL der vælges: **No OTA (2MB APP/2MB SPIFFS)** eller **Custom Partition Table (CSV)**.

* **\[REQ-ENV-2.05\] Tredjeparts Biblioteker:**
  Udvikleren skal åbne `Sketch -> Include Library -> Manage Libraries` og installere:

  1. **Adafruit ADS1X15** (v2.4.0+): Driver til 16-bit analog-til-digital konverteren (Blok 8).

  2. **TFT_eSPI** (af Bodmer, v2.5.0+): Biblioteket til skærmen (Blok 7).

========================================================================================================================================================================
## 3.0 STRUKTUR I DEN ENESTE FIL (REQ-STR)
========================================================================================================================================================================
For at sikre overblik i en meget stor `.ino` fil, skal den stramt opdeles i sektioner markeret med gigantiske kommentar-blokke.

11.0 TILSTANDSMASKINE & AUTO-RANGING (REQ-FSM)
For at orkestrere skiftet mellem de forskellige målefunktioner (DCV, ACV, Ohm, etc.) og undgå mekanisk kaos, 
SKAL koden implementere en robust, centraliseret tilstandsmaskine (Finite State Machine - FSM).

[REQ-FSM-11.01] Hovedtilstandsmaskine (Main State):
Der skal defineres en enum class for hovedfunktionerne: MODE_DCV, MODE_ACV, MODE_DCI, MODE_ACI, MODE_OHM_2W, MODE_OHM_4W. 
Når der skiftes mellem disse, SKAL processoren tvinge alle relæer i deres "Safe State" (1000V range, alle teststrømme afbrudt), 
vente 100 ms, og derefter koble de nye relæer ind.

[REQ-FSM-11.02] Hysterese-algoritme (Anti-Chatter):
For at forhindre at relæerne "klaprer" frem og tilbage hvis spændingen svinger lige på grænsen mellem to gear, SKAL der implementeres hysterese.

Up-range (Skift op): Triggers når filteredVoltage er ≥98% af ADC'ens Full-Scale (f.eks. ≥4.000𝑉).

Down-range (Skift ned): Triggers først når spændingen er faldet til ≤9% af det nuværende gear (f.eks. ≤0.360𝑉). Dette giver et massivt, sikkert overlap.

[REQ-FSM-11.03] Flush af DSP-buffer ved relæskift:
Som beskrevet i [REQ-AFE-7.03] skal relæet have tid til at falde til ro. 
Koden SKAL ikke bare vente, men eksplicit slette alle tidligere værdier i den 100-samples store filterBuffer array og fylde den med målinger foretaget efter relæets settle-time. 
Ellers vil relæ-støj blive draget med ind i det nye gennemsnit.

12.0 KOMPLET SCPI-KOMMANDOSTRUKTUR (REQ-SCPI)
ESP32'ens SCPI-parser skal udvides til at understøtte det fulde IEEE 488.2 / SCPI-1999 standardbibliotek for bænkmultimetre. 
Parseren skal bygge på en token-baseret string-analyzer over USB-CDC forbindelsen.

[REQ-SCPI-12.01] Måleopsætning (Configuration):

CONF:VOLT:DC [<range>] – Skifter til DCV. Angives <range>, sættes gearet manuelt og auto-ranging deaktiveres.

CONF:RES [<range>] – Skifter til 2-leder modstand.

CONF:FRES [<range>] – Skifter til 4-leder (Kelvin) modstand.

[REQ-SCPI-12.02] Fil- og Hukommelseshåndtering:

MMEM:CAT? – Udlæser en liste af alle filer og filstørrelser på LittleFS-partitionen.

MMEM:DATA? "log.csv" – Dumper hele indholdet af en specifik logfil over USB.

MMEM:DEL "log.csv" – Sletter filen fra hukommelsen.

[REQ-SCPI-12.03] Udlæsning og Kontrol:

SYST:REM – Sætter apparatet i "Remote". Deaktiverer (valgfrit) opdatering af TFT-skærmen for at frigive CPU-cyklusser til maksimering af målefrekvens over USB.

READ? – Returnerer seneste formaterede måling på under 5 ms.

13.0 KALIBRERING OG NVS-LAGRING (REQ-CAL)
Da instrumentet mangler fysiske potentiometre for ultimativ stabilitet, bæres kalibreringen 100% af softwaren via Non-Volatile Storage (NVS) ved hjælp af Preferences.h.

[REQ-CAL-13.01] Den Matematiske Model:
Hvert eneste "gear" (hver relækombination) skal have sit eget sæt af kalibreringskonstanter.
Koden bruger en lineær transformation:
V_display = (V_raw * Gain) + Offset

[REQ-CAL-13.02] Datastruktur i Flash:
Der SKAL oprettes en struct CalibrationData bestående af to double variabler (Gain og Offset) for de cirka 25 forskellige måleområder (DCV 600mV...1000V, ACV 600mV...750V, DCI, Ohm osv.). 
Denne struct loades ind i RAM i setup() for lynhurtig adgang under drift.

[REQ-CAL-13.03] Kalibreringsprocedure (Kodeniveau):
Der skal kodes en under-menu eller en SCPI-rutine, der tillader to punkter:

Zero-punkt: Måleledningerne kortsluttes. Brugeren aktiverer nulstilling, og MCU'en gemmer den rå ADC-værdi direkte i Offset for at fjerne parasitisk spænding.

Span-punkt: En kendt reference (f.eks. eksakt 5.0000 V) påtrykkes. Brugeren indtaster CAL:VAL 5.0 via USB, 
og ESP32'eren beregner den nye Gain-faktor baseret på forskellen mellem V_raw og 5.0.

14.0 GUI DESIGN & GRAFISK ENGINE (REQ-UI)
Brugerfladen kører isoleret på Core 0 for ikke at forstyrre signalbehandlingen, men skal have faste designrammer.

[REQ-UI-14.01] Zonedeling af skærmen:
TFT-displayet (f.eks. 800x480 pixels via SPI/Parallel) opdeles i zoner:

Zone 1 (Top-højre): Statusindikatorer (CAT III advarselstrekant ved >30𝑉 , USB-forbindelse aktiv, Datalogger aktiv, Auto-Range status).

Zone 2 (Midt): Primært display. Font-størrelse 7/8. Antal decimaler styres dynamisk af det aktive måleområde via en switch-case, der lytter til MeasurementPacket.currentRange.

[REQ-UI-14.02] Dual-Display Mode:
Når instrumentet er i DCV-mode, og Blok 6 (True RMS) stadig kører parallelt i baggrunden, SKAL der kunne renders en mindre tekst i Zone 3 (bund) med "AC Ripple: X.XXX V".

[REQ-UI-14.03] Rullende Trend-Graf:
I et dedikeret graf-mode optegnes en linje baseret på de seneste 400 punkter. Dette kræver en cirkulær buffer i RAM, der holder Y-koordinater. 
For hver ny pakke slettes grafen, bufferen skiftes én plads til venstre, og linjen tegnes op igen med funktionen tft.drawLine(). 
For at mindske flickering optimeres dette ved kun at opdatere de berørte linjer.

15.0 LITTLEFS SLITAGE & HUKOMMELSESHÅNDTERING (REQ-MEM)
Flash-hukommelse (SPIFFS/LittleFS) dør, hvis man skriver til den for mange gange. Den oprindelige kode fra Blok 9 åbnede og lukkede filen 100 gange i sekundet, 
hvilket vil ødelægge ESP32'eren på under 2 uger.

[REQ-MEM-15.01] Skrivebuffer (Wear Leveling):
Datalogger-funktionen SKAL samle op til f.eks. 100 målinger (1 sekund) i et String eller char array i RAM. 
Når bufferen er fuld, åbnes filen i LittleFS, hele blokken skrives i én samlet operation, hvorefter filen lukkes. 
Dette reducerer slid på hukommelsen med en faktor 100.

[REQ-MEM-15.02] Log-rotation og størrelseskontrol:
Inden hver skriveoperation skal filstørrelsen kontrolleres med logFile.size(). 
Overstiger filen 1 MB, SKAL instrumentet enten automatisk oprette en ny fil (f.eks. log_002.csv) for at bevare data, 
eller den ældste data skal roteres ud (Ring-logger mode), alt efter brugerens indstilling.

16.0 FEJLHÅNDTERING & SYSTEM RECOVERY (REQ-ERR)
Når LM393 hardware-komparatoren (Blok 5) fanger en 230V "Hov-fejl" i Ohm-mode og udløser hardwareFaultISR() interruptet.

[REQ-ERR-16.01] Visuel Katastrofe-alarm:
Så snart systemHalted == true detekteres af Core 0 (GUI-kernen), SKAL den afbryde enhver igangværende skærmoptegning. 
Skærmen cleares (tft.fillScreen(TFT_RED)), og en blinkende advarsel (f.eks. "HIGH VOLTAGE OVERLOAD - REMOVE LEADS") skrives i hvid midt på skærmen. 
Piezo-buzzeren aktiveres kontinuerligt.

[REQ-ERR-16.02] Låst Recovery-tilstand:
Instrumentet må aldrig forsøge at genoptage målingen automatisk. 
Koden SKAL indgå i en uendelig while(systemHalted) lykke, der kun poller USB-porten for *RST kommandoen eller afventer et fysisk knaptryk fra brugeren. 
Først her må systemet gå tilbage til MODE_DCV (1000V range), rydde fejl-flaget og starte RTOS-måleopgaverne igen.

* **\[REQ-STR-3.01\] Sektion 1: Biblioteker (Includes):**
  Koden skal starte med at hente alle de nødvendige byggeklodser ind.

  ```
  // =========================================================================
  // SEKTION 1: BIBLIOTEKER (INCLUDES)
  // Her henter vi færdigskrevet kode ind, så vi slipper for at opfinde den dybe tallerken.
  // =========================================================================
  #include <Arduino.h>          // Henter grundlæggende funktioner til ESP32 processoren.
  #include <Wire.h>             // Aktiverer I2C-kommunikation (bruges til at tale med ADC-chippen).
  #include <SPI.h>              // Aktiverer SPI-kommunikation (bruges til lynhurtig grafik på TFT-skærmen).
  #include <Adafruit_ADS1X15.h> // Henter Adafruits kode, så vi nemt kan læse volt fra ADS1115 chippen.
  #include <TFT_eSPI.h>         // Henter grafik-motoren til vores display.
  #include <LittleFS.h>         // Aktiverer ESP32'ens indbyggede "harddisk" så vi kan gemme CSV filer.
  #include <Preferences.h>      // Aktiverer en lille hukommelse, der overlever strømafbrydelser (til kalibrering).
  
  ```

* **\[REQ-STR-3.02\] Sektion 2: Hardware Pin-Mapping (#define):**
  Alt hardware fra specifikationens Blok 1 til 10 skal bindes til ESP32'ens fysiske ben (GPIOs).

  ```
  // =========================================================================
  // SEKTION 2: HARDWARE PINS OG DEFINITIONER
  // Her fortæller vi ESP32'eren, hvilke fysiske ben der er forbundet til hvad på printpladen.
  // =========================================================================
  
  // -- I2C BUS TIL ADS1115 (Isoleret via ISO1541 - Blok 9) --
  #define PIN_I2C_SDA 21        // Pin 21 fungerer som data-linje (Serial Data).
  #define PIN_I2C_SCL 22        // Pin 22 fungerer som takt-giver (Serial Clock).
  
  // -- RELÆ STYRING TIL AUTO-RANGING (Blok 4 og Blok 5) --
  #define PIN_RELAY_DIV10 13    // Når denne sættes HØJ (3.3V), aktiveres 1:10 spændingsdeleren via optokobleren.
  #define PIN_RELAY_DIV100 12   // Når denne sættes HØJ, aktiveres 1:100 spændingsdeleren (op til 1000V).
  #define PIN_RELAY_AC_ROUTE 14 // Når denne sættes HØJ, sendes signalet forbi AD8436 True RMS chippen (Blok 6).
  #define PIN_RELAY_OHM 27      // Når denne sættes HØJ, udsendes en teststrøm til at måle modstand.
  
  // -- SIKKERHED OG INTERRUPTS --
  #define PIN_FAULT_INT 25      // Lytter på hardware-komparatoren (LM393). Bliver HØJ hvis 230V rammer i Ohm-mode.
  
  ```

* **\[REQ-STR-3.03\] Sektion 3: Globale Datastrukturer:**
  Her defineres de kasser (structs), som de to processorkerner (Core 0 og Core 1) bruger til at dele information.

  ```
  // Struct til at sende den færdige måling fra den matematiske kerne til grafik-kernen
  struct MeasurementPacket {
      double finalValue;        // Den matematiske, kalibrerede værdi der skal udskrives (i Volt, Ampere eller Ohm).
      uint8_t currentRange;     // Fortæller hvilket gear vi er i (1=600mV, 2=6V, 3=60V osv.).
      bool isOverload;          // Bliver 'true' hvis spændingen overstiger, hvad elektronikken kan klare.
      bool isAcMode;            // Bliver 'true' hvis vi kigger på vekselstrøm.
      uint32_t timeStamp;       // Antal millisekunder siden apparatet blev tændt (bruges til log-filen).
  };
  
  // QueueHandle_t opretter et rør (en kø), hvorigennem vi sikkert kan sende data mellem de to kerner.
  QueueHandle_t dataQueue;      
  
  ```
========================================================================================================================================================================
## 4.0 REAL-TIME OPERATING SYSTEM (REQ-RTOS)
========================================================================================================================================================================
ESP32 processoren har to hjerner (kerner). For at skærmen ikke fryser, mens vi laver tung matematik, 
og for at vi kan måle præcist 100 gange i sekundet (SPS), skal arbejdet deles.

* **\[REQ-RTOS-4.01\] Task Allokering:**

  * **Core 1 (Hardware-kernen):** Får funktionen `Task_Metrology`. Den har højeste prioritet. Den måler strøm, udregner gennemsnit (DSP), kalibrerer og styrer relæerne.

  * **Core 0 (Bruger-kernen):** Får funktionen `Task_GUI_SCPI`. Den har lav prioritet. Den tegner grafik på skærmen, overvåger USB-porten for kommandoer og gemmer filer.

* **\[REQ-RTOS-4.02\] Opsætning i setup() funktionen:** Arduino IDE starter altid i `setup()`. Her konfigurerer vi RTOS.

  ```
  void setup() {
      // 1. Start den serielle port (USB) så vi kan tale med LabVIEW eller Python senere (SCPI)
      Serial.begin(115200); 
  
      // 2. Opret vores data-rør (Queue) med plads til 10 pakker ad gangen.
      dataQueue = xQueueCreate(10, sizeof(MeasurementPacket)); 
  
      // 3. Konfigurer ESP32'ens indbyggede ben til at styre relæerne
      pinMode(PIN_RELAY_DIV10, OUTPUT);  // Gør benet til en udgang
      pinMode(PIN_RELAY_DIV100, OUTPUT); // Gør benet til en udgang
      // ... (Gør det samme for de andre relæer)
  
      // 4. Start Hardware-opgaven på Kerne 1 (Core 1)
      xTaskCreatePinnedToCore(
          Task_Metrology,    // Navnet på funktionen vi vil køre
          "MetrologyTask",   // Et menneskelæseligt navn til debugging
          8192,              // Giv opgaven 8 Kilobyte RAM til arbejdet
          NULL,              // Ingen start-parametre
          3,                 // Prioritet 3 (Meget høj, vigtigere end grafik)
          NULL,              // Vi har ikke brug for at gemme taskens ID
          1                  // Lås fast til Kerne 1
      );
  
      // 5. Start Bruger-opgaven på Kerne 0 (Core 0)
      xTaskCreatePinnedToCore(Task_GUI_SCPI, "GUITask", 16384, NULL, 1, NULL, 0);
  }
  
  // Standard loop() funktionen skal efterlades TOM i ESP32 RTOS, da vi bruger Tasks i stedet.
  void loop() { }
  
  ```
========================================================================================================================================================================
## 5.0 HARDWARE SIKKERHED & INTERRUPTS (REQ-SAF)
========================================================================================================================================================================
Dette system skal kunne håndtere 600V (CAT III). Hvis brugeren tilslutter 230V fra stikkontakten ind i Multimeteret mens det står til at måle Modstand (Ohm), 
vil PTC'en og hardware-klamperne fange det (HW Blok 5). Men firmwaren *skal* reagere øjeblikkeligt.

* **\[REQ-SAF-5.01\] IRAM-Låst Interrupt Service Routine:** Når komparatoren på printet opdager 230V i ohm-mode, sender den et 5V signal til `PIN_FAULT_INT`. 
Koden skal afbryde *alt* andet og reagere på under 1 mikrosekund. Dette gøres ved at gemme funktionen i lynhurtig IRAM.

  ```
  volatile bool systemHalted = false; // "volatile" betyder at variablen kan ændres pludseligt udefra
  
  // IRAM_ATTR fortæller ESP32'eren, at den skal lægge denne funktion i den hurtigste hukommelse den har.
  void IRAM_ATTR hardwareFaultISR() {
      // 1. Tving ALLE relæer tilbage til udgangspunktet (højeste spændingsområde = mest sikkert)
      digitalWrite(PIN_RELAY_DIV10, LOW);  // Sluk for 6V dæmperen
      digitalWrite(PIN_RELAY_DIV100, LOW); // Sluk for 60V dæmperen
      digitalWrite(PIN_RELAY_OHM, LOW);    // SLUK for modstands-test-strømmen ØJEBLIKKELIGT
  
      // 2. Fortæl resten af programmet (grafik og matematik) at vi er i en fejltilstand
      systemHalted = true; 
  }
  
  ```

* **\[REQ-SAF-5.02\] Aktivering:** ISR'en skal tilkobles i `setup()`: `attachInterrupt(digitalPinToInterrupt(PIN_FAULT_INT), hardwareFaultISR, RISING);`
========================================================================================================================================================================
## 6.0 DIGITAL SIGNALBEHANDLING (REQ-DSP)
========================================================================================================================================================================
Multimeterets præcision opstår her. `Task_Metrology` læser konstant ADC'en (Analog til Digital Konverter) og tryller rå støjende tal om til ultra-stabile målinger.

* **\[REQ-DSP-6.01\] ADC Opsætning (ADS1115):**
  ADC'en skal sættes op til at måle fra -4.096V til +4.096V.

* **\[REQ-DSP-6.02\] Moving Average Filter (Ringbuffer):** Stikkontakter udsender 50Hz støj. 
For at fjerne dette (NMRR - Normal Mode Rejection Ratio), 
skal vi samle en række målinger (samples) og tage gennemsnittet af præcis én bølgelængde (eller flere).

  ```
  // Uendelig lykke der kører på Kerne 1
  void Task_Metrology(void *pvParameters) {
      Adafruit_ADS1115 adc; // Opretter forbindelse til vores ADC chip
      adc.begin(0x48);      // 0x48 er I2C adressen på ADS1115 chippen (ADDR ben forbundet til GND)
      adc.setGain(GAIN_ONE); // Sæt ADC'ens indre forstærker til at måle op til 4.096V
  
      float filterBuffer[100]; // En kasse med 100 rum til at gemme de sidste 100 målinger
      int bufferIndex = 0;     // Holder styr på, hvilket rum vi skal putte næste måling i
  
      for(;;) { // Denne for-lykke kører for evigt
          // Hvis der er sket en katastrofe (hov-fejl), så stop med at måle
          if (systemHalted == true) {
              vTaskDelay(pdMS_TO_TICKS(100)); // Vent et kvart sekund og tjek igen
              continue; // Spring resten af lykken over
          }
  
          // 1. LÆS HARDWAREN
          // Bed chippen om at måle forskellen mellem AIN0 og GND (Single ended måling)
          int16_t rawAdcValue = adc.readADC_SingleEnded(0); 
  
          // 2. KONVERTER TIL VOLT
          // Fordi vi bruger GAIN_ONE (4.096V), er hver "enhed" fra chippen lig med 0.125 milliVolt.
          float rawVoltage = rawAdcValue * 0.000125; 
  
          // 3. PUT I FILTER
          filterBuffer[bufferIndex] = rawVoltage; // Gem målingen i det aktuelle rum
          bufferIndex = bufferIndex + 1;          // Ryk en plads frem
          if (bufferIndex >= 100) {               // Hvis vi er nået til enden af kassen...
              bufferIndex = 0;                    // ...så start forfra og overskriv de ældste data (Ringbuffer)
          }
  
          // 4. UDREGN GENNEMSNIT (DSP)
          float sum = 0; // En variabel til at holde den samlede værdi
          for (int i = 0; i < 100; i++) {         // Gennemgå alle 100 rum
              sum = sum + filterBuffer[i];        // Læg værdien oveni
          }
          float filteredVoltage = sum / 100.0;    // Divider med 100 for at få det perfekte gennemsnit
  
          // 5. TILSTREB NØJAGTIGT 100 HERTZ
          // FreeRTOS vTaskDelay sørger for, at tasken sover præcis længe nok til at hele lykken tager 10 ms.
          vTaskDelay(pdMS_TO_TICKS(10)); 
      }
  }
  
  ```
========================================================================================================================================================================
## 7.0 AUTO-RANGING TILSTANDSMASKINE (REQ-AFE)
========================================================================================================================================================================
Hvis brugeren måler på et AA-batteri (1.5V), skal relæerne være slukket (6V området). 
Måler brugeren derefter stikkontakten (230V), vil ADC'en melde fejl (overload). 
Firmwaren skal automatisk gribe ind og tænde dæmper-relæerne (Blok 4).

* **\[REQ-AFE-7.01\] Skift Op (Up-Ranging):**
  Hvis `filteredVoltage` overstiger 4.000V (tæt på ADS1115's max på 4.096V), skal systemet skifte gear.

* **\[REQ-AFE-7.02\] Skift Ned (Down-Ranging):**
  Hvis den udregnede displayværdi falder under 0.500V, og vi er i et højt gear, skal vi skifte ned for at få flere decimalers præcision.

* **\[REQ-AFE-7.03\] Relæ Blanking:**
  Mekaniske relæer bruger tid (millisekunder) på at bevæge sig, og kontaktsættet "hopper" (bouncing).

  * **Krav:** Lige efter et relæ er skiftet via `digitalWrite`, SKAL der kaldes `vTaskDelay(pdMS_TO_TICKS(50));`. 
  Derefter SKAL ringbufferen (`filterBuffer`) fyldes op med 0'er, så de hoppende forstyrrelser ikke blandes ind i de pæne gennemsnit.

========================================================================================================================================================================
## 8.0 SCPI PARSER & USB KOMMUNIKATION (REQ-COM)
========================================================================================================================================================================
Professionelle multimetre styres via computeren ved hjælp af et sprog der hedder SCPI (Standard Commands for Programmable Instruments). 
ESP32's indbyggede USB port fungerer som Virtuel COM Port.

* **\[REQ-COM-8.01\] SCPI Overvågning (I Task_GUI_SCPI):** Hver gang UI-kernen har tegnet skærmen, skal den tjekke om PC'en har skrevet noget.

  ```
  if (Serial.available() > 0) {                      // Spørger: Ligger der ulæste tegn fra USB-kablet i hukommelsen?
      String cmd = Serial.readStringUntil('\n');     // Læs alle tegn indtil der trykkes "Enter" på PC'en.
      cmd.trim();                                    // Fjern ubrugelige mellemrum i starten og slutningen.
      cmd.toUpperCase();                             // Gør alt til store bogstaver (så 'meas?' og 'MEAS?' er ens).
  
      if (cmd == "*IDN?") {                          // SCPI standard-kommando for "Identify"
          // PC'en vil vide hvem vi er. Svar tilbage i det officielle IEEE 488.2 format:
          Serial.println("DMM-MASTER,16SRMS,SN1,V6.0"); 
      }
      else if (cmd == "MEAS:VOLT:DC?") {             // SCPI kommando for "Send mig den seneste måling af jævnstrøm"
          // Udskriv den seneste gemte måling med 5 decimaler og send det over USB kablet
          Serial.println(latestPacket.finalValue, 5); 
      }
      else if (cmd == "*RST") {                      // SCPI kommando for "Reset"
          // Logik til at genstarte relæer til 1000V range
      }
  }
  
  ```
========================================================================================================================================================================
## 9.0 LITTLEFS DATALOGGING (REQ-GUI)
========================================================================================================================================================================
Instrumentet skal kunne gemme lange tidsserier af målinger direkte på dets egen chip.

* **\[REQ-GUI-9.01\] Åbn fil i "Append" mode:** Hver gang der modtages en ny `MeasurementPacket` i Queuen (100 gange i sekundet), 
skal den konverteres til CSV-tekst og gemmes.

  ```
  // Åbn filen "log.csv" på den indbyggede harddisk. "a" betyder append (sæt ind i bunden uden at slette det gamle).
  File logFile = LittleFS.open("/log.csv", "a"); 
  if (logFile) {                                   // Tjek at filen faktisk blev åbnet med succes
      char csvBuffer[64];                          // Opret et tomt lærred til at bygge vores tekststreng på
      // sprintf samler teksten: %lu = timestamp (unsigned long), %f = kommatal
      sprintf(csvBuffer, "%lu,DCV,%f\n", latestPacket.timeStamp, latestPacket.finalValue); 
      logFile.print(csvBuffer);                    // Gem teksten fysisk på chippen
      logFile.close();                             // Luk filen igen for at sikre at data ikke går tabt hvis strømmen ryger
  }
  
  ```

  *(Note for udvikler: I den endelige kode bør `logFile.close()` kun kaldes hver 100. gang for at undgå at slide Flash-hukommelsen op)*.

========================================================================================================================================================================
## 10.0 SKÆRMOPDATERING (ANTI-FLICKER)
========================================================================================================================================================================
Bænkmultimetre har gigantiske tal på skærmen. Hvis man rydder skærmen (`fillScreen(BLACK)`) hver gang tallet ændres, vil skærmen blinke ulideligt (flicker).

* **\[REQ-GUI-10.01\] Sort-overskrivnings teknikken:** For at opdatere tal uden at skærmen blinker, 
overskriver man det gamle tal med baggrundsfarven, før man tegner det nye.

  ```
  // 1. Visk det gamle ud:
  // Sæt tekstfarven til sort, og baggrunden til sort
  tft.setTextColor(TFT_BLACK, TFT_BLACK); 
  // Tegn det gamle tal (som vi gemte i forrige runde) præcis der hvor det står, i skrifttype 7. Det forsvinder nu.
  tft.drawFloat(oldDisplayValue, 4, 10, 50, 7); 
  
  // 2. Tegn det nye:
  // Sæt tekstfarven til grøn (eller gul for volt), og baggrunden til sort
  tft.setTextColor(TFT_GREEN, TFT_BLACK); 
  // Tegn det nye rigtige tal oven på det sted vi lige har visket ud.
  tft.drawFloat(newDisplayValue, 4, 10, 50, 7); 
  
  // 3. Gem til næste gang:
  oldDisplayValue = newDisplayValue; // Husk det nye tal, så vi ved hvad vi skal viske ud næste gang lykken kører.
  
  ```

**AFSLUTTENDE GODKENDELSESKRAV TIL UDVIKLEREN:**
Når du påbegynder udarbejdelsen af `Multimeter_Firmware.ino`, vil dit arbejde blive afvist hvis:

1. Du deler koden op i flere filer.

2. Du udelader en eneste linje fra at have en `//` kommentar med funktions- og formålsforklaring.

3. Koden forudsætter forhåndsviden (f.eks. udelader kommentarer på `Serial.begin` fordi "det ved alle hvad er").