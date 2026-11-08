
---

## `hardware/README.md`

```markdown
# Hardware — DMM-MASTER 16SRMS

> ⚠️ **STATUS: IKKE FÆRDIG**
>
> Hardware-delen af projektet er **endnu ikke færdigudviklet og ikke publiceret**.
> Denne mappe indeholder placeholder-mapper med beskrivelser af, hvad der
> mangler, og hvilke krav der gælder.

## Statusoversigt

| Mappe | Indhold | Status |
| --- | --- | --- |
| `schematics/` | KiCad-skemaer (Blok 1–10) | ❌ Mangler |
| `pcb/` | PCB-layout, Gerber X2, NC Drill, CPL | ❌ Mangler |
| `bom/` | Bill of Materials med MPN'er | ❌ Mangler |
| `enclosure/` | Kabinet (.STEP/.STL) | ❌ Mangler |
| `datasheets/` | Datablade for kritiske komponenter | ❌ Mangler |
| `docs/` | HW-SW-SPEC-V6.1 (RA8875-opdateret) | ✅ Opdateret 2026-11-08 |

## Designkrav (uddrag)

Fulde krav findes i [`docs/HW-SW-SPEC-V6.0-MASTER.md`](../docs/HW-SW-SPEC-V6.0-MASTER.md).

- **Isolation:** ≥ 8 mm krybeafstand mellem Zone A og Zone B, >4000 V RMS
- **PCB:** Anbefalet 4-lags, 1 oz indre / 2 oz ydre kobber, FR4 ≥1,6 mm
- **Isolationsslidser:** Skal fræses under optokoblere og ISO1541
- **Guard-rings:** Omkring højimpedansbaner i Blok 4
- **Termisk design:** Manganin-shunt og regulatorer skal have thermiske vias
- **Kabinet:** UL94 V-0 brandhæmmende plast med interne barrierer
- **Display (OPDATERET 2026-11-08):**
  5" 800×480 TFT LCD med **RA8875-controller** via SPI (20 MHz).
  Anbefalet modul: Buydisplay ER-TFTM050-3 (eller tilsvarende RA8875-baseret).

  > ⚠️ **BREAKING CHANGE — SSD1963 ER AFLYST**
  >
  > | Aspekt | Fra (udgået) | Til (gældende) |
  > |--------|--------------|----------------|
  > | Controller | SSD1963 | **RA8875** |
  > | Interface | 8/16-bit parallel | **SPI (20 MHz)** |
  > | GPIO-forbrug | 12+ pins | **6 pins** (MOSI, SCLK, CS, DC, RST, evt. BL) |
  > | TFT_eSPI-driver | `SSD1963_8DRIVER` (parallel) | `RA8875_DRIVER` (SPI) |
  > | `User_Setup.h` | `#define SSD1963_DRIVER` ❌ | `#define RA8875_DRIVER` ✅ |
  >
  > **Årsag:** TFT_eSPI har **ingen** SSD1963 SPI-driver. SSD1963 understøttes
  > udelukkende i 8-bit/16-bit parallel-tilstand, hvilket ville kræve 12 frie
  > GPIOs — mere end ESP32 har til rådighed, når relæer (GPIO 12/13/14/27),
  > interrupt (GPIO 25) og I2C (GPIO 21/22) er reserveret. RA8875 er den eneste
  > 800×480-controller, som TFT_eSPI reelt understøtter i SPI-mode.
  >
  > **Konsekvens for hardware-design:**
  > - PCB-footprint skal matche RA8875 SPI-modul (ikke 8-bit parallel).
  > - 6 GPIOs reserveres i pin-mapping (se `User_Setup.h` og HW-DOC §1.4).
  > - Backlight (TFT_BL) er valgfri — mange RA8875-moduler har hardware-backlight.
  > - SPI-frekvens må ikke overstige 20 MHz for stabil drift på lange baner.

## Hvordan du kan hjælpe

Se [`CONTRIBUTING.md`](../CONTRIBUTING.md) — især afsnittene om hardware-design
og 3D-kabinet.

Prioriteret rækkefølge:

1. **KiCad-skemaer** for alle 10 blokke
2. 2. **PCB-layout** med isolationsslidser og guard-rings.
   **VIGTIGT:** Display-connector SKAL designes til RA8875 SPI-pinout
   (6-polet: MOSI, SCLK, CS, DC, RST, GND), IKKE SSD1963 8-bit parallel.
3. **BOM** med verificerede MPN'er og tolerancer
4. **Kabinet** med CAT III-barrierer og 19 mm bøsningsafstand

1. DC Spændingsmåling (DCV)Signalet dæmpes via et præcisionsnetværk, bufres for højimpedans og måles single-ended.Plaintext[V/Ω Indgang] ──┐
                │
         [GDT 600V + MOV S10K300] (Transientbeskyttelse)
                │
                ▼
  [10 MΩ Spændingsdeler] ── (10 stk. 1MΩ 1206, 0.1%)
                │
                ├── [Relæ K1] (DIV10)  ──> GND_ISO
                ├── [Relæ K2] (DIV100) ──> GND_ISO
                │
                ▼
        [JFET-Buffer] (LSK170 / ADA4530, for >1 GΩ impedans)
                │
                ▼ (DC-koblet)
  [ADS1115 ADC : AIN0]
  
Første forsvarslinje er en 10 mm varistor i serie med et gasudladningsrør, der klamper transienter over 560 V. 
Herefter nedskaleres spændingen gennem et 10 MΩ daisy-chain modstandsnetværk styret af ultra-lav-lækage Omron G6K relæer, 
før signalet for de to laveste områder (600 mV og 6 V) dirigeres gennem en JFET-buffer for at sikre >1 GΩ indgangsimpedans. 
Det rå DC-signal føres direkte til ADC'ens AIN0-kanal.   2. AC Spændingsmåling (ACV - True RMS)Signalet følger samme indgangstrin som DC, 
men dirigeres AC-koblet til den analoge RMS-konverter.Plaintext[V/Ω Indgang] ──> [Transientbeskyttelse] ──> [10 MΩ Spændingsdeler]
                                                    │
                                                    ▼
                                            [JFET-Buffer]
                                                    │
                                                    ▼ (AC-koblet)
                                            [Relæ K3] (AC_ROUTE)
                                                    │
                                                    ▼
                                      [AD8436 True RMS Konverter] 
                                      (med 3.3 µF C_AVG kondensator)
                                                    │
                                                    ▼ (0-4V DC output)
                                            [ADS1115 ADC : AIN1]
											
Signalet benytter den samme spændingsdeler og FET-buffer som DC-målingen, hvorefter det routes via AC_ROUTE relæet ind i en AD8436 True RMS-IC. 
AD8436-chippen udnytter en 3.3 µF lav-lækage gennemsnitskondensator til integration og konverterer AC-signalet (op til 100 kHz båndbredde) 
til et rent DC-spændingssignal. Dette proportionale DC-signal aflæses på ADC'ens AIN1-kanal.   

3. DC Strømmåling (DCI)Strømmen ledes gennem fysiske shunts, og spændingsfaldet måles differentielt for maksimal nøjagtighed.
Plaintext[mA/µA Indgang] ────> [PTC + Elektronisk Klamper] ─────┐
																│
		 [10A Indgang] ──────> [11A HRC Keramisk Sikring] ──────┤
																│
																▼
								   [Shunts (Blok 10)] ◄─────────┴─────────> [COM Indgang]
														• 100 Ω (µA)                           
														• 1 Ω (mA)
														• 0.01 Ω 5W Manganin (10A)
																│
																│ (Spændingsfald)
																▼
													[ADS1115 ADC : AIN2 & AIN3] (Differentielt input)
													
Lavstrømsindgangen er beskyttet af en PTC-termistor og en elektronisk klamper (BJT/MOSFET-baseret), 
mens 10 A-indgangen afbrydes mekanisk af en Bussmann 11 A / 1000 V HRC-sikring ved overstrøm. 
Strømmen afsætter et spændingsfald over enten præcisionsmodstandene (100 Ω / 1 Ω) eller den ultra-stabile 0.01 Ω Manganin-shunt. 
Spændingsfaldet aflæses differentielt direkte over chippens AIN2 og AIN3.   

4. AC Strømmåling (ACI)Spændingsfaldet fra den valgte shunt multiplexes til RMS-konverteren frem for direkte til ADC'en.
Plaintext[Strømindgange] ──> [Beskyttelse & HRC Sikring] ──> [Shunts] ──> [COM Indgang]
																				│
																				│ (Spændingsfald)
																				▼
																	[Relæ Multiplexing]
																				│
																				▼
																	[AD8436 True RMS Konverter]
																				│
																				▼ (0-4V DC output)
																		[ADS1115 ADC : AIN1]
I stedet for at læse strømsignalets spændingsfald direkte på ADC'en, routes signalet i ACI-mode gennem systemets True RMS IC (AD8436) via relæ-styring. 
AD8436 leverer derefter DC-ækvivalenten ind til ADC'ens AIN1.   

5. Modstandsmåling (Ohms)Designet benytter en 4-leder Kelvin-topologi parret med en "Hov-fejl" detektor for aktiv beskyttelse mod netspænding.Plaintext[Konstantstrømskilde]
																	(LT1006 OP-amp + LM336-2.5V Ref)
																				│
																				▼ (100nA - 1mA teststrøm)
													[PTC 100mA, 600V] ──────────┬──────────> ["Force" ud via V/Ω] ──> [DUT] ──> [COM retur]
															│                                                │
								[Klamperdioder 1N4148] ◄────┤                                                │
								(til VCC_ISO / GND_ISO)     │                                                │
															▼                                                │
								[LM393 Komparator] ──> [Hardware Interrupt til ESP32 via 6N137 Opto]         │
								(Ved >4.5V på node)                                                          │
																											 │
															┌────────────────────────────────────────────────┘
															│ (Kelvin Sense linjer)
															▼
												[ADS1115 ADC : AIN2 & AIN3] (Differentielt input)
En LT1006 præcisions-OP-amp parret med en stabil 2.5 V reference udstyrer terminalerne med en kontrolleret teststrøm, 
mens Kelvin-matrixen sender spændingsfaldet direkte tilbage til AIN2 og AIN3 differentielt. 
For at beskytte mod 230 V indsat ved en fejl ("hov-fejl"), klamper 1N4148-dioder spændingen til strømforsyningens rails, 
hvorefter PTC-termistoren hurtigt begrænser strømmen. Samtidig overvåger en LM393 komparator spændingsknudepunktet
og udløser på under et mikrosekund et hardware-interrupt via en 6N137 optokobler over til ESP32, så relæerne kan frakobles og skaden begrænses. 

Blok-for-blok vurdering
Blok 1 — Digital strømforsyning (Zone A)
Kan tegnes: 🟡 Delvist

Element	Status
Transformator 9 V AC → 4× 1N4007 → 1000 µF → LM7805 → AMS1117-3.3	✅ Kan tegnes direkte
Snubber-kondensatorer 10 nF over dioder	✅ Specificeret
Afkobling (100 nF, 10 µF)	✅ Specificeret
Men ingeniøren skal selv:

Vælge transformator-MPN (dokumentet siger "f.eks. Myrra 44083" — ikke verificeret for viklingsadskillelse)

Dimensionere LM7805-køleplade (åbent punkt #6: "termisk belastning ikke analyseret")

Beslutte om AMS1117-3.3's 800 mA er nok til TFT + ESP32 + ISO1541 (åbent punkt #3: TFT-MPN ikke valgt)

Konklusion: Skemaet kan tegnes, men strømbudget og termik er ikke verificeret.

Blok 2 — Isoleret analog strømforsyning (Zone B)
Kan tegnes: 🟡 Delvist

Element	Status
9V-0-9V center-tapped → dobbelt broudretter → LM7805/LM7905 → ±5 V_ISO	✅ Kan tegnes direkte
Tantal 10 µF + keramisk 100 nF afkobling	✅ Specificeret
Men ingeniøren skal selv:

Sikre at transformatorens to viklinger har tilstrækkelig fysisk adskillelse (≥8 mm) — dette er et mekanisk valg, ikke et skemavalg, men det påvirker valget af transformator

Beslutte om LM7905's negative rail skal have ekstra beskyttelse (dokumentet nævner det ikke)

Konklusion: Kan tegnes, men transformatorens viklingsadskillelse er ikke dokumenteret.

Blok 3 — Transientbeskyttelse
Kan tegnes: 🔴 Nej, ikke uden at løse en konflikt

Dokumentet siger:

MOV: S10K300 (300 V AC RMS max kontinuerlig)

GDT: 600 V DC sparkover

Serieforbindelse

Problemet: HW-DOC §4.1 siger selv: "S10K300's maksimale kontinuerlige AC-spænding er 300 V RMS, hvilket er tilstrækkeligt til 600 V DC-måling, men ikke til kontinuerlig 600 V AC RMS."

Og åbent punkt #5: "1000 V DC-området bruger samme MOV som 600 V AC. MOV kan overbelastes ved 600 V AC RMS."

Ingeniøren skal selv:

Vælge mellem S10K300 (til DC-only) eller S10K420 (til AC 600 V RMS)

Beslutte om GDT'ens 600 V sparkover er forenelig med den valgte MOV

Dimensionere PCB-baner (100 mil) — det er layout, ikke skema

Konklusion: Kan ikke tegnes entydigt — der er en uløst modstrid mellem DC- og AC-krav.

Blok 4 — Auto-ranging, spændingsdeler, FET-buffer
Kan tegnes: 🟡 Delvist

Element	Status
10× 1 MΩ 1206 i serie (10 MΩ)	✅ Specificeret
Omron G6K-2P-Y relæer (K1, K2, K3)	✅ Specificeret
LSK170 JFET-buffer	🟡 Nævnt, men bias-netværk ikke dimensioneret
Men ingeniøren skal selv:

Dimensionere LSK170 bias: gate-modstand, source-modstand, drain-modstand, afkobling — dokumentet siger kun "skal bias'es korrekt"

Beslutte om ADA4530-1 skal bruges i stedet (dokumentet nævner det som alternativ, men footprint er anderledes)

Dimensionere relæ-driverkredsløb (PC817 + base-modstand + eventuel Darlington) — dokumentet nævner PC817, men ikke driver-topologi

Beslutte hvordan AC-kobling til AD8436 sker (kondensatorværdi, bias)

Tegne guard-ring — det er layout, men påvirker skemaets netlabels

Konklusion: Kan delvist tegnes — topologien er klar, men flere kritiske komponentværdier mangler.

Blok 5 — Modstandsmåling og "hov-fejl"
Kan tegnes: 🔴 Nej, ikke uden egne designbeslutninger

Element	Status
LT1006 + LM336-2.5 konstantstrømskilde	🟡 Topologi nævnt, men ingen værdier
PTC 100 mA / 600 V	✅ Specificeret
1N4148 klamper til VCC_ISO/GND_ISO	✅ Specificeret
LM393 komparator	🟡 Nævnt, men reference-deler (R18–R21) ikke dimensioneret
6N137 optokobler	✅ Specificeret
Men ingeniøren skal selv:

Dimensionere LT1006-strømkilden: feedback-modstande, reference-modstand, range-skift (100 nA til 1 mA) — dokumentet siger "omskiftelig teststrøm", men ikke hvordan

Dimensionere LM393-reference: R18–R21 er "efter dimensionering" — dvs. ingeniøren skal selv vælge 4,5 V reference-deler

Dimensionere 6N137 LED-modstand (R22) — "afhænger af VCC_ISO og LED-strøm"

Beslutte hvordan range-skift af teststrøm styres (flere relæer? MOSFET?)

Beslutte hvordan Kelvin-matrixen (Force+/Force−/Sense+/Sense−) rout es — dokumentet nævner det, men tegner det ikke

Konklusion: Kan ikke tegnes entydigt — for mange værdier er udeladt.

Blok 6 — True RMS (AD8436)
Kan tegnes: 🟡 Delvist

Element	Status
AD8436 (QSOP-20)	✅ Specificeret
C_AVG = 3,3 µF tantal/film	✅ Specificeret
Gain-modstande R22/R23	🟡 "Efter datablad" — ikke dimensioneret
Men ingeniøren skal selv:

Dimensionere R22/R23 ud fra AD8436-databladet og ønsket gain (0–4 V output for 0–750 V RMS input)

Beslutte indgangs-kobling (AC-koblingskondensator, bias til mid-rail)

Beslutte om AD8436's interne buffer skal bruges eller ekstern op-amp

Konklusion: Kan tegnes delvist — topologien er klar, men gain-dimensionering mangler.

Blok 7 — ESP32, TFT, buzzer
Kan tegnes: ✅ Ja, stort set

Element	Status
ESP32-WROOM-32 modul	✅ Specificeret
Pin-mapping (I2C, relæer, interrupt, SPI)	✅ Specificeret i HW-DOC §1.4
Buzzer på GPIO 26	✅ Specificeret
Reset-knap	✅ Nævnt
Men ingeniøren skal selv:

Vælge endelig TFT-MPN (RA8875, 800×480, SPI) — dokumentet siger "f.eks. Buydisplay ER-TFTM050-3", ikke verificeret

Beslutte TFT-backlight-driver (dokumentet nævner TFT_BL som valgfri)

Beslutte buzzer-driver (transistor eller direkte GPIO — dokumentet siger "strøm ≤12 mA")

Sikre at ESP32's strapping-pins ikke bruges til relæer (GPIO 12 er strapping — dokumentet nævner det ikke eksplicit)

Konklusion: Kan tegnes, men TFT-MPN skal vælges først.

Blok 8 — ADS1115
Kan tegnes: ✅ Ja

Element	Status
ADS1115 (VSSOP-10)	✅ Specificeret
I2C pull-ups 10 kΩ	✅ Specificeret
Afkobling 100 nF + 1 µF	✅ Specificeret
ADDR til GND (0x48)	✅ Specificeret
Konklusion: Kan tegnes direkte.

Blok 9 — Digital isolation
Kan tegnes: 🟡 Delvist

Element	Status
ISO1541 (SOIC-8)	✅ Specificeret
6N137 (DIP-8)	✅ Specificeret
PC817 ×4 (DIP-4)	✅ Specificeret
LED-modstande R3–R7	🟡 "Dimensioneres efter VCC_ISO" — ikke dimensioneret
Men ingeniøren skal selv:

Dimensionere R3 (6N137 LED, 220 Ω er nævnt — men skal verificeres mod VCC_ISO)

Dimensionere R4–R7 (PC817 LED, 1 kΩ er nævnt — men skal verificeres mod ESP32 GPIO-strøm)

Beslutte pull-up på ISO1541's måleside (dokumentet nævner 10 kΩ til VCC_ISO)

Løse åbent punkt #1: ISO1541's UL-rating er 2500 V RMS, ikke 4000 V RMS — dette er en blokerende konflikt for CAT III-certificering

Konklusion: Kan tegnes delvist — men isoleringsrating-konflikten skal løses først.

Blok 10 — Strømshunts
Kan tegnes: 🟡 Delvist

Element	Status
100 Ω (µA), 1 Ω (mA), 0,01 Ω Manganin (10 A)	✅ Værdier specificeret
HRC-sikring Bussmann DMM-B-11A	✅ Specificeret
Klampertransistorer Q2–Q5	🟡 "Efter dimensionering" — ikke dimensioneret
PTC2	🟡 "Efter dimensionering" — ikke dimensioneret
Men ingeniøren skal selv:

Dimensionere klampertransistorer (BJT/MOSFET-topologi, værdier)

Dimensionere PTC2

Beslutte hvordan shunt-multiplexing til AD8436 sker (relæer? dokumentet nævner "relæ-multiplexing", men tegner det ikke)

Løse åbent punkt #7: Manganin-shunt med dokumenteret TCR <50 ppm/°C

Konklusion: Kan tegnes delvist — men flere beskyttelseskomponenter mangler dimensionering.

Samlet oversigt
Blok	Kan tegnes direkte?	Kræver egne beslutninger
1 — Digital strøm	🟡 Delvist	Trafo-MPN, køleplade, strømbudget
2 — Analog strøm	🟡 Delvist	Trafo-viklingsadskillelse
3 — Transient	🔴 Nej	MOV-rating-konflikt (DC vs. AC)
4 — Auto-ranging	🟡 Delvist	LSK170 bias, relæ-driver, AC-kobling
5 — Ohm & hov-fejl	🔴 Nej	LT1006-strømkilde, LM393-reference, 6N137-modstand
6 — True RMS	🟡 Delvist	AD8436 gain-modstande, indgangs-kobling
7 — ESP32/TFT	✅ Ja	TFT-MPN, buzzer-driver
8 — ADS1115	✅ Ja	—
9 — Isolation	🟡 Delvist	LED-modstande, isolationsrating-konflikt
10 — Shunts	🟡 Delvist	Klampertransistorer, PTC2, multiplexing
Resultat: 2 blokke kan tegnes direkte, 6 delvist, 2 slet ikke.

Hvad ingeniøren konkret skal gøre selv
Dimensionering (ikke specificeret i dokumentationen)
LSK170 bias-netværk (Blok 4) — gate/source/drain-modstande

AD8436 gain-modstande R22/R23 (Blok 6) — ud fra datablad

LM393 reference-deler R18–R21 (Blok 5) — 4,5 V reference

LT1006 strømkilde-feedback (Blok 5) — range-skift 100 nA–1 mA

6N137 LED-modstand R3 (Blok 9) — verificér mod VCC_ISO

PC817 LED-modstande R4–R7 (Blok 9) — verificér mod ESP32 GPIO-strøm

Klampertransistorer Q2–Q5 (Blok 10) — topologi og værdier

PTC2 (Blok 10) — dimensionering

Relæ-driverkredsløb (Blok 4) — PC817 + base-modstand + eventuel Darlington

AC-koblingskondensator til AD8436 (Blok 4/6) — værdi og bias

MPN-valg (ikke fastlagt)
TFT-display — RA8875, 800×480, SPI, 3,3 V

Transformator — med dokumenteret viklingsadskillelse ≥8 mm

Manganin-shunt — med dokumenteret TCR <50 ppm/°C

Sikkerhedsbøsninger — 4 mm, 19 mm centerafstand

PTC-termistorer — 100 mA hold, 600 V CAT III

Konfliktløsning (blokerende)
MOV-rating — S10K300 vs. S10K420 (DC vs. AC)

ISO1541 vs. 4000 V RMS — vælg højere rating eller dokumentér 8 mm-barriere

ISR-flanke — SRS siger RISING, firmware siger FALLING

SSD1963 vs. RA8875 — firmware_README.md er ikke opdateret

Termisk/EMC (verifikation)
LM7805/LM7905 køleplade — termisk analyse

Manganin-shunt termisk design — kobberpolygoner

EMC-præcompliance — planlægning