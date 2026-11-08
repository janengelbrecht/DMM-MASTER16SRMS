================================================================================
KALIBRERINGSPROCEDURE — DMM-MASTER 16SRMS
================================================================================
DOKUMENT-ID:      CAL-PROC-V1.0
KLASSIFIKATION:   INTERN / OPEN-SOURCE
PROJEKT:          ISOLERET CAT III 4½-CIFRET BÆNKMULTIMETER
SYSTEMARKITEKTUR: ESP32 / ADS1115 / AD8436
DATO:             08. NOVEMBER 2026
EJER:             JEP-ELECTRONICS R&D
STATUS:           PROCEDURE IKKE AFRØVET PÅ FYSISK HARDWARE
================================================================================

⚠️  VIGTIG BEMÆRKNING
--------------------------------------------------------------------------------
Denne procedure er endnu IKKE afprøvet på fysisk hardware. Den beskriver den
tilsigtede fremgangsmåde baseret på firmwarens implementerede logik
(CAL:ZERO, CAL:VAL, mapModeAndGearToCalibrationSlot()) og skal verificeres
på den færdige prototype, før den erklæres gældende.


================================================================================
1. FORMÅL OG OMFANG
================================================================================
Denne procedure beskriver, hvordan DMM-MASTER 16SRMS kalibreres via
SCPI-kommandoerne CAL:ZERO og CAL:VAL. Proceduren dækker alle understøttede
måletilstande og gear.

  ┌────────────────┬───────────────┬────────────────────┬──────────────────────┐
  │ Mode           │ Gear (indeks) │ Kalibreringsslots   │ Fysisk område        │
  ├────────────────┼───────────────┼────────────────────┼──────────────────────┤
  │ MODE_DCV       │ 0–4           │ 0–4                │ 1000V,600V,60V,6V,   │
  │                │               │                    │ 600mV                │
  │ MODE_ACV       │ 0–4           │ 5–9                │ 750V,600V,60V,6V,    │
  │                │               │                    │ 600mV                │
  │ MODE_DCI       │ 0–5           │ 10–15              │ 10A,6A,600mA,60mA,   │
  │                │               │                    │ 6mA,600µA            │
  │ MODE_ACI       │ 0–5           │ 16–21              │ 10A,6A,600mA,60mA,   │
  │                │               │                    │ 6mA,600µA            │
  │ MODE_OHM_2W    │ 0–5           │ 22–27              │ 60MΩ,6MΩ,600kΩ,      │
  │                │               │                    │ 60kΩ,6kΩ,600Ω        │
  │ MODE_OHM_4W    │ 0–5           │ 28–33              │ 60MΩ,6MΩ,600kΩ,      │
  │                │               │                    │ 60kΩ,6kΩ,600Ω        │
  └────────────────┴───────────────┴────────────────────┴──────────────────────┘

  BEMÆRK OM GEAR-INDEKS:
  Gear 0 er den mest dæmpede (højeste spænding/strøm/modstand).
  Gear 5 (eller 4 for spænding) er den mindst dæmpede (laveste område).
  Dette følger firmwarens selectRelayGear() og
  mapModeAndGearToCalibrationSlot().


================================================================================
2. KALIBRERINGSMODEL
================================================================================
For hvert måleområde (slot) gemmes to double-konstanter i NVS via
Preferences.h:

    V_display = (V_raw × Gain) + Offset

  ┌──────────┬────────────────────────────────────┬──────────────────────────┐
  │ Konstant │ Betydning                          │ Sådan beregnes den       │
  ├──────────┼────────────────────────────────────┼──────────────────────────┤
  │ Gain     │ Multiplikationsfaktor for          │ Gain = V_reference /     │
  │          │ hældningsfejl (span)               │ V_raw                    │
  │ Offset   │ Nulpunktsjustering (parasitisk     │ Offset = −(V_raw × Gain) │
  │          │ spænding)                          │ ved kortsluttet indgang  │
  └──────────┴────────────────────────────────────┴──────────────────────────┘

  Standardværdier ved første opstart:
    Gain   = 1.0
    Offset = 0.0
  (læses fra NVS af loadCalibrationFromNvs()).

  Lagring:
    Hver kalibreringspost gemmes som nøglerne "gain<slot>" og "offs<slot>"
    i NVS-namespacet "calibration" (se saveCalibrationToNvs()).

  ⚠️ VIGTIGT:
  CAL:VAL og CAL:ZERO bruger latestPacket.calibrationSlot — den slot der
  var aktiv i den SENESTE MeasurementPacket. Sørg for at instrumentet er i
  den ønskede mode og det ønskede gear, og at isStale = false, før
  kalibrering udføres. Ellers kan kommandoen returnere ERROR.


================================================================================
3. FORUDSÆTNINGER
================================================================================

3.1 UDSTYR
--------------------------------------------------------------------------------
  • Referencemultimeter med min. 6½ cifre
    (f.eks. Fluke 8846A, Keysight 34461A)
  • Præcisions-spændingsreference (f.eks. 5,0000 V, 0,01 %)
  • Præcisionsmodstande: 100Ω, 1kΩ, 10kΩ, 100kΩ, 1MΩ, 10MΩ (0,01 %)
  • Præcisionsshunt / strømreference til DCI/ACI (0,01 %)
  • AC-spændingsreference med kendt RMS-værdi og frekvens
    (typisk 50 Hz eller 1 kHz)
  • Stabil temperatur: 23 ± 2 °C
  • Opvarmningstid: min. 30 minutter
    (både DMM-MASTER og referencemultimeter)

3.2 FIRMWARE-FORUDSÆTNINGER
--------------------------------------------------------------------------------
  • Multimeter_Firmware.ino uploadet og kørende
  • User_Setup.h konfigureret til RA8875 (SPI, 20 MHz)
  • LittleFS formateret (sker automatisk ved første opstart)
  • SCPI-forbindelse via USB-CDC ved 115200 baud

3.3 SCPI-OPSÆTNING
--------------------------------------------------------------------------------
  Send:

      SYST:REM

  Sætter instrumentet i Remote-mode, så GUI-opdateringer deaktiveres og
  CPU-cyklusser frigives til måling. Skærmen fryser (bevidst), men
  SCPI-svar fortsætter.

  NYTTIGE KOMMANDOER UNDER KALIBRERING:

  ┌──────────────────────────┬──────────────────────────────────────────────┐
  │ Kommando                 │ Formål                                       │
  ├──────────────────────────┼──────────────────────────────────────────────┤
  │ CONF:VOLT:DC <gear>      │ Skift til DCV, manuelt gear (0–4)            │
  │ CONF:VOLT:AC <gear>      │ Skift til ACV, manuelt gear (0–4)            │
  │ CONF:CURR:DC <gear>      │ Skift til DCI, manuelt gear (0–5)            │
  │ CONF:CURR:AC <gear>      │ Skift til ACI, manuelt gear (0–5)            │
  │ CONF:RES <gear>          │ Skift til 2W Ohm, manuelt gear (0–5)         │
  │ CONF:FRES <gear>         │ Skift til 4W Ohm, manuelt gear (0–5)         │
  │ CONF:VOLT:DC?            │ Læs nuværende gear                           │
  │ READ?                    │ Læs seneste kalibrerede værdi (5 decimaler)  │
  │ CAL:ZERO                 │ Gem nuværende rå værdi som offset for        │
  │                          │ aktiv slot                                   │
  │ CAL:VAL <reference>      │ Beregn og gem ny gain for aktiv slot         │
  │ *RST                     │ Nulstil til DCV safe state, genaktivér       │
  │                          │ auto-ranging                                 │
  └──────────────────────────┴──────────────────────────────────────────────┘


================================================================================
4. KALIBRERINGS-RÆKKEFØLGE (ANBEFALET)
================================================================================
Kalibrer i denne rækkefølge for at undgå at skulle gentage trin:

  1. DCV       (alle 5 gear) — fundamentet for spændingsdeleren
  2. ACV       (alle 5 gear) — afhænger af DCV-front-end
  3. DCI       (alle 6 gear) — shunts og differentiel ADC
  4. ACI       (alle 6 gear) — AD8436-kæden
  5. OHM_2W    (alle 6 gear) — teststrømskilde
  6. OHM_4W    (alle 6 gear) — Kelvin-matrix

  BEMÆRK:
  Der er 6 gear for DCI/ACI/Ohm, men kun 5 for DCV/ACV. Gear-indekset i
  CONF:-kommandoen skal matche det fysiske område, du påtrykker.


================================================================================
5. GENEREL KALIBRERINGS-SEKVENS PR. SLOT
================================================================================
For hvert område (mode + gear) udføres TO punkter: zero og span.

5.1 ZERO-PUNKT (OFFSET)
--------------------------------------------------------------------------------
  Formål: Fjerne parasitisk offsetspænding
          (termisk EMF, bias-strøm, ADC-offset).

  Trin:
    1. Skift til det ønskede område:
           CONF:VOLT:DC 4        (eksempel: 600 mV-området)
    2. Kortslut indgangen (V/Ω og COM) med en kort, tyk ledning.
    3. Vent ≥ 2 sekunder til filterbufferen er fyldt med friske samples
       (isStale = false).
    4. Verificér med READ?, at værdien er stabil
       (varierer < 1 count over 5 aflæsninger).
    5. Send:
           CAL:ZERO
    6. Forventet svar: OK.
       Ved ERROR: tjek at latestPacket.calibrationSlot er gyldig og at
       rawVoltage ≠ 0.

  ⚠️ BEMÆRK:
  CAL:ZERO beregner Offset = −(V_raw × Gain) med den NUVÆRENDE gain.
  Hvis du kalibrerer et nyt område første gang (gain = 1.0), er offset
  korrekt relativt til gain = 1.0. Kør derfor ALTID zero FØRST, derefter
  span — span-kalibreringen ændrer gain, men offset forbliver korrekt,
  fordi den er beregnet relativt til den gain, der var aktiv på det
  tidspunkt.

5.2 SPAN-PUNKT (GAIN)
--------------------------------------------------------------------------------
  Formål: Rette hældningsfejl i spændingsdeler, shunt eller RMS-konverter.

  Trin:
    1. Påtryk en kendt reference på indgangen. Vælg en værdi tæt på
       fuld skala for området (typisk 80–100 % af området).
    2. Vent ≥ 2 sekunder til filterbufferen er fyldt.
    3. Verificér med READ?, at værdien er stabil.
    4. Send:
           CAL:VAL 5.0000
       (hvor 5.0000 er referenceværdien i den aktuelle enhed —
        volt, ampere eller ohm)
    5. Forventet svar: OK.
       Ved ERROR: tjek at referenceValue > 0 og at |rawVoltage| > 1e-9.

  ⚠️ KRITISK:
  CAL:VAL bruger rawVoltage (den RÅ filtrerede spænding FØR kalibrering)
  og den AKTIVE slot fra latestPacket. Hvis du skifter gear eller mode
  mellem måling og kommando, kan du kalibrere den forkerte slot. Send
  READ? umiddelbart før CAL:VAL for at bekræfte, at du er i det rigtige
  område.

5.3 VERIFIKATION EFTER HVERT SLOT
--------------------------------------------------------------------------------
  1. Send READ? og sammenlign med referencemultimeteret.
  2. Acceptkriterium: afvigelse < ±(0,025 % af læsning + 5 counts)
     for DCV, eller den specifikation der gælder for området.
  3. Hvis afvigelsen er for stor: gentag zero og span. Hvis problemet
     fortsætter, undersøg hardware (relækontaktmodstand, loddefejl,
     termisk EMF).


================================================================================
6. OMRÅDESPECIFIKKE REFERENCEVÆRDIER
================================================================================

6.1 DCV (SLOTS 0–4)
--------------------------------------------------------------------------------
  ┌───────┬───────────────┬─────────────────────┬─────────────────────────┐
  │ Gear  │ Fysisk område │ Zero                │ Span-reference          │
  ├───────┼───────────────┼─────────────────────┼─────────────────────────┤
  │ 0     │ 1000 V        │ Kortslut            │ 900,000 V (eller 1000V) │
  │ 1     │ 600 V         │ Kortslut            │ 500,000 V               │
  │ 2     │ 60 V          │ Kortslut            │ 50,000 V                │
  │ 3     │ 6 V           │ Kortslut            │ 5,0000 V                │
  │ 4     │ 600 mV        │ Kortslut            │ 0,50000 V               │
  └───────┴───────────────┴─────────────────────┴─────────────────────────┘

  Eksempel på CAL:VAL-kommando:
    Gear 4:  CAL:VAL 0.50000
    Gear 3:  CAL:VAL 5.0000
    Gear 2:  CAL:VAL 50.000
    Gear 1:  CAL:VAL 500.000
    Gear 0:  CAL:VAL 900.000

  ⚠️ BEMÆRK:
  1000 V-området kræver en højspændingsreference, som de færreste
  labouratorier har. Alternativt kan 600 V-området kalibreres med 500 V
  og 1000 V-området med samme reference, fordi begge bruger 1:100-
  dæmpning (DIV10+DIV100). Firmwaren behandler dem dog som separate
  slots (0 og 1), så begge skal kalibreres.

6.2 ACV (SLOTS 5–9)
--------------------------------------------------------------------------------
  ┌───────┬───────────────┬──────────────────────┬────────────────────────┐
  │ Gear  │ Fysisk område │ Zero                 │ Span-reference         │
  ├───────┼───────────────┼──────────────────────┼────────────────────────┤
  │ 5     │ 750 V         │ Kortslut (AC-koblet) │ 600,000 V RMS          │
  │ 6     │ 600 V         │ Kortslut             │ 500,000 V RMS          │
  │ 7     │ 60 V          │ Kortslut             │ 50,000 V RMS           │
  │ 8     │ 6 V           │ Kortslut             │ 5,0000 V RMS           │
  │ 9     │ 600 mV        │ Kortslut             │ 0,50000 V RMS          │
  └───────┴───────────────┴──────────────────────┴────────────────────────┘

  Frekvens: 50 Hz eller 1 kHz (skal være stabil og ren sinus).

  ⚠️ BEMÆRK:
  AC-kalibrering kræver en ren sinusreference med kendt RMS-værdi.
  AD8436's nøjagtighed er ±0,25 %, så ACV-områderne kan ikke forventes
  at nå DCV's ±0,025 %. Brug en reference med bedre nøjagtighed end
  AD8436 (f.eks. 0,05 %) for at undgå at kalibrere mod en fejlbehæftet
  kilde.

6.3 DCI (SLOTS 10–15)
--------------------------------------------------------------------------------
  ┌───────┬───────────────┬───────────────────────┬───────────────────────┐
  │ Gear  │ Fysisk område │ Zero                  │ Span-reference        │
  ├───────┼───────────────┼───────────────────────┼───────────────────────┤
  │ 10    │ 10 A          │ Åben indgang (0 A)    │ 8,0000 A              │
  │ 11    │ 6 A           │ Åben indgang          │ 5,0000 A              │
  │ 12    │ 600 mA        │ Åben indgang          │ 500,00 mA             │
  │ 13    │ 60 mA         │ Åben indgang          │ 50,000 mA             │
  │ 14    │ 6 mA          │ Åben indgang          │ 5,0000 mA             │
  │ 15    │ 600 µA        │ Åben indgang          │ 500,00 µA             │
  └───────┴───────────────┴───────────────────────┴───────────────────────┘

  Shunt: 0,01 Ω Manganin (10 A/6 A), 1 Ω (600 mA/60 mA),
         100 Ω (6 mA/600 µA).

  ⚠️ BEMÆRK:
  Ved strømkalibrering skal referencemultimeteret være i serie med
  DMM-MASTER, eller en præcisionsstrømkilde bruges direkte. Sørg for, at
  strømmen er stabil (lav ripple) og at shuntens temperatur er stabil
  (10 A-shunten dissiperer 1 W ved 10 A).

6.4 ACI (SLOTS 16–21)
--------------------------------------------------------------------------------
  ┌───────┬───────────────┬───────────────────────┬───────────────────────┐
  │ Gear  │ Fysisk område │ Zero                  │ Span-reference        │
  ├───────┼───────────────┼───────────────────────┼───────────────────────┤
  │ 16    │ 10 A          │ Åben indgang          │ 8,0000 A RMS          │
  │ 17    │ 6 A           │ Åben indgang          │ 5,0000 A RMS          │
  │ 18    │ 600 mA        │ Åben indgang          │ 500,00 mA RMS         │
  │ 19    │ 60 mA         │ Åben indgang          │ 50,000 mA RMS         │
  │ 20    │ 6 mA          │ Åben indgang          │ 5,0000 mA RMS         │
  │ 21    │ 600 µA        │ Åben indgang          │ 500,00 µA RMS         │
  └───────┴───────────────┴───────────────────────┴───────────────────────┘

  Frekvens: 50 Hz (stabil og ren sinus).

6.5 OHM_2W (SLOTS 22–27)
--------------------------------------------------------------------------------
  ┌───────┬───────────────┬───────────────────────┬───────────────────────┐
  │ Gear  │ Fysisk område │ Zero                  │ Span-reference        │
  ├───────┼───────────────┼───────────────────────┼───────────────────────┤
  │ 22    │ 60 MΩ         │ Åben indgang          │ 50,000 MΩ (0,01 %)    │
  │ 23    │ 6 MΩ          │ Åben indgang          │ 5,0000 MΩ             │
  │ 24    │ 600 kΩ        │ Åben indgang          │ 500,00 kΩ             │
  │ 25    │ 60 kΩ         │ Åben indgang          │ 50,000 kΩ             │
  │ 26    │ 6 kΩ          │ Åben indgang          │ 5,0000 kΩ             │
  │ 27    │ 600 Ω         │ Kortslut (0 Ω)        │ 500,00 Ω              │
  └───────┴───────────────┴───────────────────────┴───────────────────────┘

  ⚠️ BEMÆRK:
  For 600 Ω-området er zero-punktet en KORTSLUTNING (0 Ω), ikke åben
  indgang. For de højere områder er zero-punktet åben indgang (uendelig
  modstand), fordi teststrømmen løber gennem luft/åben kreds. CAL:ZERO
  gemmer den rå spænding, som i åben tilstand kan være tæt på nul, men
  ikke nøjagtigt nul på grund af lækstrømme.

6.6 OHM_4W (SLOTS 28–33)
--------------------------------------------------------------------------------
  Samme referenceværdier som OHM_2W, men med Kelvin-forbindelse:

  ┌───────┬───────────────┬───────────────────────┬───────────────────────┐
  │ Gear  │ Fysisk område │ Zero                  │ Span-reference        │
  ├───────┼───────────────┼───────────────────────┼───────────────────────┤
  │ 28    │ 60 MΩ         │ Åben (Sense+/Sense−)  │ 50,000 MΩ             │
  │ 29    │ 6 MΩ          │ Åben                  │ 5,0000 MΩ             │
  │ 30    │ 600 kΩ        │ Åben                  │ 500,00 kΩ             │
  │ 31    │ 60 kΩ         │ Åben                  │ 50,000 kΩ             │
  │ 32    │ 6 kΩ          │ Åben                  │ 5,0000 kΩ             │
  │ 33    │ 600 Ω         │ Kortslut Sense+/−     │ 500,00 Ω              │
  └───────┴───────────────┴───────────────────────┴───────────────────────┘

  ⚠️ VIGTIGT:
  I 4W-mode skal Force+ / Force− og Sense+ / Sense− forbindes korrekt
  til referencemodstanden. Zero-punktet for 600 Ω-området kræver, at
  Sense+ og Sense− kortsluttes VED MODSTANDEN, ikke ved instrumentet.


================================================================================
7. EKSEMPEL: FULD DCV-KALIBRERING (MANUELT VIA SCPI)
================================================================================

    SYST:REM

    ; --- Gear 4: 600 mV ---
    CONF:VOLT:DC 4
    ; (kortslut indgangen, vent 2 s)
    READ?                          -> f.eks. 0.00012
    CAL:ZERO                       -> OK
    ; (påtryk 0.50000 V)
    READ?                          -> f.eks. 0.49987
    CAL:VAL 0.50000                -> OK
    READ?                          -> 0.50000 (verificér)

    ; --- Gear 3: 6 V ---
    CONF:VOLT:DC 3
    ; (kortslut, vent)
    CAL:ZERO                       -> OK
    ; (påtryk 5.0000 V)
    CAL:VAL 5.0000                 -> OK

    ; --- Gear 2: 60 V ---
    CONF:VOLT:DC 2
    CAL:ZERO                       -> OK
    CAL:VAL 50.000                 -> OK

    ; --- Gear 1: 600 V ---
    CONF:VOLT:DC 1
    CAL:ZERO                       -> OK
    CAL:VAL 500.000                -> OK

    ; --- Gear 0: 1000 V ---
    CONF:VOLT:DC 0
    CAL:ZERO                       -> OK
    CAL:VAL 900.000                -> OK

  ⚠️ BEMÆRK:
  Efter hvert CONF:VOLT:DC <gear> sættes autoRangeEnabled = false, og
  externalRangeChangePending = true sættes, så Core 1 flusher
  filterbufferen. Vent derfor ALTID ≥ 2 sekunder (helst 3) efter et
  gearskift, før du sender CAL:ZERO eller CAL:VAL, så isStale = false og
  latestPacket.rawVoltage er frisk.


================================================================================
8. VERIFIKATION EFTER FULD KALIBRERING
================================================================================

8.1 FUNKTIONSKONTROL PR. MODE
--------------------------------------------------------------------------------
  ┌────────────────────┬────────────────────────────────┬──────────────────┐
  │ Test               │ Fremgangsmåde                  │ Acceptkriterium  │
  ├────────────────────┼────────────────────────────────┼──────────────────┤
  │ DCV, alle gear     │ Påtryk kendt DC-spænding i     │ < ±(0,025 % +    │
  │                    │ hvert gear, sammenlign med     │ 5 counts)        │
  │                    │ reference                      │                  │
  │ ACV, alle gear     │ Påtryk kendt AC RMS (50 Hz     │ < ±(0,5 % +      │
  │                    │ og 1 kHz), sammenlign          │ 10 counts)       │
  │ DCI, alle gear     │ Seriestrøm med reference,      │ < ±(0,05 % +     │
  │                    │ sammenlign                     │ 5 counts)        │
  │ ACI, alle gear     │ AC-strøm med reference,        │ < ±(0,5 % +      │
  │                    │ sammenlign                     │ 10 counts)       │
  │ OHM_2W, alle gear  │ Præcisionsmodstande,           │ < ±(0,05 % +     │
  │                    │ sammenlign                     │ 5 counts)        │
  │ OHM_4W, alle gear  │ Kelvin-forbindelse til         │ < ±(0,05 % +     │
  │                    │ præcisionsmodstande            │ 5 counts)        │
  │ Zero-stabilitet    │ Kortslut indgang, mål drift    │ < 5 counts drift │
  │                    │ over 1 time                    │                  │
  │ Gearskift-         │ Skift gear 10 gange, mål       │ < ±2 counts      │
  │ repetition         │ samme reference                │ variation        │
  └────────────────────┴────────────────────────────────┴──────────────────┘

8.2 LAGRINGSKONTROL
--------------------------------------------------------------------------------
  Efter kalibrering kan NVS-indholdet verificeres ved at genstarte
  instrumentet (power cycle) og gentage READ?. Kalibreringen skal være
  bevaret, fordi loadCalibrationFromNvs() indlæser den ved opstart.

  NVS-INSPEKTION (VALGFRIT):
  NVS-namespacet "calibration" indeholder nøglerne gain0…gain39 og
  offs0…offs39. De kan læses med et lille ESP32-program eller via
  Preferences-API'et.


================================================================================
9. FEJLFINDING
================================================================================
  ┌────────────────────────────┬────────────────────────────┬────────────────┐
  │ Symptom                    │ Mulig årsag                │ Løsning        │
  ├────────────────────────────┼────────────────────────────┼────────────────┤
  │ CAL:ZERO returnerer ERROR  │ calibrationSlot uden for   │ Tjek at mode/  │
  │                            │ 0–39, eller rawVoltage er  │ gear er        │
  │                            │ NaN                        │ gyldigt; send  │
  │                            │                            │ READ? og       │
  │                            │                            │ verificér      │
  │ CAL:VAL returnerer ERROR   │ referenceValue ≤ 0, eller  │ Tjek at        │
  │                            │ |rawVoltage| ≤ 1e-9        │ reference > 0  │
  │                            │                            │ og at indgang  │
  │                            │                            │ har signal     │
  │ Kalibrering "forsvinder"   │ NVS-skrivning fejlede,     │ Tjek           │
  │ efter genstart             │ eller forkert slot blev     │ saveCalibrati- │
  │                            │ kalibreret                 │ onToNvs()-     │
  │                            │                            │ return;        │
  │                            │                            │ verificér slot │
  │                            │                            │ med CONF:...?  │
  │ Stor afvigelse efter span  │ Zero kørt efter span       │ Kør altid zero │
  │                            │ (offset beregnet med       │ FØR span       │
  │                            │ gammel gain)               │                │
  │ ACV-kalibrering ustabil    │ AC-kilden har ripple eller │ Brug ren       │
  │                            │ frekvensdrift              │ sinus, stabil  │
  │                            │                            │ frekvens, vent │
  │                            │                            │ på C_AVG-      │
  │                            │                            │ indswingning   │
  │                            │                            │ (≥ 5 s)        │
  │ OHM-kalibrering drifter    │ Termisk EMF i              │ Vent på        │
  │                            │ teststrømskredsen          │ temperatur-    │
  │                            │                            │ stabilitet     │
  │                            │                            │ (30 min), brug │
  │                            │                            │ kortslutning   │
  │                            │                            │ med samme      │
  │                            │                            │ metal          │
  │ READ? returnerer NAN       │ isStale = true (under      │ Vent ≥ 2 s     │
  │                            │ gearskift)                 │ efter          │
  │                            │                            │ gearskift      │
  └────────────────────────────┴────────────────────────────┴────────────────┘


================================================================================
10. KENDTE BEGRÆNSNINGER
================================================================================
  1. Ingen automatiseret kalibreringssekvens — tools/calibration/ er
     endnu ikke implementeret (se tools/calibration/README.md).
     Proceduren udføres manuelt via SCPI.

  2. Ingen fysisk hardware verificeret — hele proceduren er skrevet ud
     fra firmwarens implementerede logik (CAL:ZERO, CAL:VAL,
     mapModeAndGearToCalibrationSlot()), ikke fra målinger på en
     prototype.

  3. ACV-nøjagtighed er begrænset af AD8436 (±0,25 %) og kan ikke
     forbedres ved software-kalibrering alene.

  4. 1000 V DC-området kræver højspændingsreference; alternativt kan
     600 V-området bruges som proxy, men slot 0 og 1 skal kalibreres
     separat.

  5. Temperaturkoefficient for kalibreringskonstanterne er ikke
     specificeret. Ved brug uden for 23 ± 2 °C bør kalibreringen
     verificeres.

  6. Kalibreringskonstanter gemmes som double i NVS, men
     Preferences.putDouble() har begrænset skrivecyklus-levetid. Undgå
     unødvendige gentagne CAL:-kald.


================================================================================
11. REFERENCER
================================================================================
  • Multimeter_Firmware.ino
      - handleScpiCommand() (CAL:VAL, CAL:ZERO)
      - saveCalibrationToNvs()
      - loadCalibrationFromNvs()
      - mapModeAndGearToCalibrationSlot()

  • kravspecifikation_firmware.md
      - REQ-CAL-13.01 / 13.02 / 13.03
      - REQ-SCPI-12.01

  • dmm_master_16srms_hardware_dokumentation.md
      - Afsnit 4.2 (spændingsdeler)
      - Afsnit 4.3 (Ohm)
      - Afsnit 4.5 (ADC)
      - Afsnit 4.6 (shunts)

  • tools/calibration/README.md
      - Planlagte kalibreringsscripts


================================================================================
SLUT PÅ DOKUMENT — CAL-PROC-V1.0
================================================================================