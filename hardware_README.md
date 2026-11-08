
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

Komplet dimensionerings- og komponentvalgsproces for DMM-MASTER 16SRMS
Baseret på den eksisterende dokumentation og de identificerede mangler gives her en systematisk proces for hvert blok, der mangler dimensionering. Processen er opdelt i beslutningspunkter, beregninger, komponentvalg og verifikationskrav.

Blok 3 — Transientbeskyttelse (MOV/GDT)
Problem
S10K300 (300 V AC RMS) er utilstrækkelig til 600 V AC RMS kontinuerlig måling. Åbent punkt #5 i HW-DOC.

Beslutningsproces
Trin 1: Fastlæg MCOV-krav
For CAT III 600 V AC RMS skal MOV'ens Maximum Continuous Operating Voltage (MCOV) være ≥ 600 V AC RMS. Bourns anbefaler MCOV ≥ 1,25 × nominel spænding for UL/IEC 62368-1 compliance . Ved 600 V AC RMS kræves MCOV ≥ 750 V AC.

Trin 2: Vælg MOV-type

S10K420: 420 V AC RMS MCOV — stadig utilstrækkelig til 600 V AC kontinuerlig.

S14K510 eller S20K550: 510–550 V AC RMS MCOV — tilstrækkelig med margin.

Alternativt: Differentier beskyttelsen, så MOV kun er aktiv i DC-mode, og AC-mode bruger en anden kreds. Dette kræver ekstra relæer og komplicerer designet.

Anbefaling: Vælg S20K550 (550 V AC RMS MCOV, 20 mm) for at dække både 600 V AC og 1000 V DC. Ved 1000 V DC er MCOV-kravet lavere (DC-MCOV er typisk ~1,4× AC-MCOV for MOV'er), så 550 V AC RMS dækker 1000 V DC.

Trin 3: Vælg GDT
GDT'ens sparkover-spænding skal være højere end MOV'ens clamp-spænding ved maksimal transient, men lav nok til at-beskytte MOV'en. Med S20K550 er clamp-spændingen ved 100 A typisk ~1,5 kV. Vælg GDT med sparkover 600–800 V DC (f.eks. Littelfuse GTCX36-601M eller GTCX36-801M).

Trin 4: Verifikation

Beregn samlet restspænding ved 6 kV/3 kA transient (8/20 µs): MOV clamp + GDT arc voltage.

Verificer at restspænding < ADC's absolutte maksimum (4,096 V efter dæmpning).

Blok 4 — LSK170 bias-netværk
Problem
LSK170's bias-netværk er ikke dimensioneret. Dokumentet siger kun "skal bias'es korrekt".

Beslutningsproces
Trin 1: Fastlæg DC-operationspunkt
LSK170 er en N-kanal JFET med:

V
G
S
S
=
−
40
V 
GSS
​
 =−40 V max (gate-source breakdown) 

I
D
S
S
I 
DSS
​
  varierer efter grade (A/B/C/D)

Typisk transkonduktans 
g
m
=
22
g 
m
​
 =22 mS 

Indgangsimpedans 20 GΩ typ 

Trin 2: Vælg source-modstand 
R
S
R 
S
​
 
For at sætte et bias-strøm 
I
D
I 
D
​
 :

R
S
=
V
G
S
I
D
R 
S
​
 = 
I 
D
​
 
V 
GS
​
 
​
 
hvor 
V
G
S
V 
GS
​
  er den ønskede gate-source spænding (negativ for N-JFET).

Vælg 
I
D
=
1
I 
D
​
 =1 mA og 
V
G
S
=
−
0
,
5
V 
GS
​
 =−0,5 V (typisk for LSK170):

R
S
=
0
,
5
0
,
001
=
500
 
Ω
R 
S
​
 = 
0,001
0,5
​
 =500Ω
Trin 3: Vælg drain-modstand 
R
D
R 
D
​
 
For at sætte drain-spændingen til midten af forsyningsspændingen (f.eks. 2,5 V ved 5 V forsyning):

R
D
=
V
D
D
−
V
D
I
D
=
5
−
2
,
5
0
,
001
=
2
,
5
 
k
Ω
R 
D
​
 = 
I 
D
​
 
V 
DD
​
 −V 
D
​
 
​
 = 
0,001
5−2,5
​
 =2,5kΩ
Trin 4: Gate-bias
For høj indgangsimpedans skal gate være DC-koblet til signalet uden DC-sti til jord. En gate-modstand 
R
G
=
10
R 
G
​
 =10 MΩ til jord (eller til offset-justering) sikrer defineret DC-potentiale uden at belaste signalet væsentligt (10 MΩ || 20 GΩ ≈ 6,7 MΩ, stadig >1 GΩ kravet? Nej — 6,7 MΩ er under 1 GΩ).

Kritisk: For at opfylde >1 GΩ indgangsimpedans må 
R
G
R 
G
​
  være meget stor eller helt udelades. Løsning: Brug DC-servo eller offset-justering via højimpedans-netværk, eller ac-kobl signalet og brug JFET'ens gate som floating DC-punkt (gate-lækstrøm vil langsomt drive potentialet). Den mest robuste løsning er en guard-ring på PCB og en gate-modstand på ≥100 MΩ til en lav-impedans offset-kilde.

Trin 5: Komponentvalg

R
S
R 
S
​
 : 500 Ω, 0,1 %, 25 ppm/°C, 0603

R
D
R 
D
​
 : 2,5 kΩ, 0,1 %, 25 ppm/°C, 0603

R
G
R 
G
​
 : 100 MΩ, 1 %, 0805 (eller udelad hvis DC-servo bruges)

Afkobling: 100 nF X7R ved drain og source

Trin 6: Verifikation

Simuler i LTspice: AC-analyse for båndbredde, støj-analyse for input-referred noise.

Mål gate-lækstrøm ved 25 °C og 50 °C (skal være <1 nA for at undgå offset-drift).

Blok 5 — LT1006 konstantstrømskilde
Problem
Strømkilden er nævnt, men ikke dimensioneret. Kræver range-skift 100 nA–1 mA.

Beslutningsproces
Trin 1: Fastlæg topologi
Standard howland-strømkilde eller Jim Williams current source (LT1006 + instrumentforstærker) . For enkelhed og præcision anbefales howland-strømkilde med LT1006:

I
o
u
t
=
V
r
e
f
R
s
e
n
s
e
I 
out
​
 = 
R 
sense
​
 
V 
ref
​
 
​
 
Trin 2: Dimensioner for hvert range

Range	
I
o
u
t
I 
out
​
 	
R
s
e
n
s
e
R 
sense
​
 	
V
r
e
f
V 
ref
​
  (LM336)	Effekt i DUT ved 600 Ω
600 Ω	1 mA	2,5 kΩ	2,5 V	0,6 mW
6 kΩ	100 µA	25 kΩ	2,5 V	60 µW
60 kΩ	10 µA	250 kΩ	2,5 V	6 µW
600 kΩ	1 µA	2,5 MΩ	2,5 V	0,6 µW
6 MΩ	100 nA	25 MΩ	2,5 V	0,06 µW
60 MΩ	10 nA	250 MΩ	2,5 V	0,006 µW
Problem: 250 MΩ modstand er upraktisk (støj, lækstrøm, PCB-layout). Alternativ: Brug lavere 
V
r
e
f
V 
ref
​
  for høje ranges, eller switch-kondensator-teknik.

Praktisk løsning: Brug 
V
r
e
f
=
2
,
5
V 
ref
​
 =2,5 V for alle ranges, men skift 
R
s
e
n
s
e
R 
sense
​
  via reed-relæer (lav lækstrøm) eller analog switch med lav lækstrøm (f.eks. ADG1408, men lækstrøm ~100 pA kan være for høj for 60 MΩ).

Anbefaling: Brug reed-relæer (f.eks. Coto 9007) til range-skift af 
R
s
e
n
s
e
R 
sense
​
 , da de har lækstrøm <1 pA og kontaktmodstand <100 mΩ.

Trin 3: Komponentvalg

R
s
e
n
s
e
R 
sense
​
  (1 mA): 2,5 kΩ, 0,1 %, 25 ppm/°C, 0805

R
s
e
n
s
e
R 
sense
​
  (100 µA): 25 kΩ, 0,1 %, 25 ppm/°C, 0805

R
s
e
n
s
e
R 
sense
​
  (10 µA): 250 kΩ, 0,1 %, 25 ppm/°C, 0805

R
s
e
n
s
e
R 
sense
​
  (1 µA): 2,5 MΩ, 0,1 %, 25 ppm/°C, 1206

R
s
e
n
s
e
R 
sense
​
  (100 nA): 25 MΩ, 1 %, 25 ppm/°C, 1206 (svær at skaffe i 0,1 %)

R
s
e
n
s
e
R 
sense
​
  (10 nA): 250 MΩ — upraktisk. Alternativ: Brug T-netværk af 10 MΩ + 10 MΩ + 2,5 MΩ med switching.

Trin 4: Verifikation

Mål output-strøm ved hvert range med præcisionsmultimeter i serie.

Mål lækstrøm i "off" state (<1 nA for alle ranges).

Blok 5 — LM393 reference-deler (R18–R21)
Problem
Reference-deler til 4,5 V detektion er ikke dimensioneret.

Beslutningsproces
Trin 1: Fastlæg reference
LM393's ikke-inverterende indgang skal have en reference på 4,5 V (i henhold til HW-DOC §4.3). Brug en spændingsdeler fra VCC_ISO (5 V) eller en LM336-2,5 reference.

Trin 2: Beregn modstandsdeler
Med VCC_ISO = 5 V og ønsket 
V
r
e
f
=
4
,
5
V 
ref
​
 =4,5 V:

R
t
o
p
R
b
o
t
t
o
m
=
5
−
4
,
5
4
,
5
=
0
,
5
4
,
5
=
1
9
R 
bottom
​
 
R 
top
​
 
​
 = 
4,5
5−4,5
​
 = 
4,5
0,5
​
 = 
9
1
​
 
Vælg 
R
b
o
t
t
o
m
=
90
R 
bottom
​
 =90 kΩ og 
R
t
o
p
=
10
R 
top
​
 =10 kΩ (E96-serien: 90,9 kΩ og 10 kΩ giver 
V
r
e
f
=
4
,
50
V 
ref
​
 =4,50 V).

Trin 3: Hysterese
For at undgå chattering ved langsomt stigende spænding tilføjes positiv feedback (hysterese). Med LM393's open-collector output og en pull-up 
R
P
U
=
10
R 
PU
​
 =10 kΩ:

V
h
y
s
=
V
C
C
×
R
b
o
t
t
o
m
R
t
o
p
+
R
b
o
t
t
o
m
×
R
P
U
R
P
U
+
R
t
o
p
∣
∣
R
b
o
t
t
o
m
V 
hys
​
 =V 
CC
​
 × 
R 
top
​
 +R 
bottom
​
 
R 
bottom
​
 
​
 × 
R 
PU
​
 +R 
top
​
 ∣∣R 
bottom
​
 
R 
PU
​
 
​
 
Med 
R
t
o
p
=
10
R 
top
​
 =10 kΩ, 
R
b
o
t
t
o
m
=
90
,
9
R 
bottom
​
 =90,9 kΩ, 
R
P
U
=
10
R 
PU
​
 =10 kΩ:

R
t
o
p
∣
∣
R
b
o
t
t
o
m
=
9
,
0
 
k
Ω
R 
top
​
 ∣∣R 
bottom
​
 =9,0kΩ
V
h
y
s
≈
5
×
0
,
9
×
10
10
+
9
≈
2
,
4
 
V
V 
hys
​
 ≈5×0,9× 
10+9
10
​
 ≈2,4V
Det er for meget hysterese (reference falder til ~2,1 V efter trigger). Reducer 
R
P
U
R 
PU
​
  til 1 kΩ:

V
h
y
s
≈
5
×
0
,
9
×
1
1
+
9
≈
0
,
45
 
V
V 
hys
​
 ≈5×0,9× 
1+9
1
​
 ≈0,45V
Det giver en trigger ved 4,5 V og release ved ~4,05 V — acceptabelt.

Trin 4: Komponentvalg

R
t
o
p
R 
top
​
 : 10 kΩ, 1 %, 0603

R
b
o
t
t
o
m
R 
bottom
​
 : 90,9 kΩ, 1 %, 0603

R
P
U
R 
PU
​
 : 1 kΩ, 1 %, 0603

Afkobling: 100 nF X7R ved LM393's VCC

Trin 5: Verifikation

Mål trigger-spænding med præcisionsspændingskilde: skal være 4,5 V ±0,1 V.

Mål release-spænding: skal være 4,05 V ±0,1 V.

Verificer reaktionstid ved 230 V step (skal være <10 µs inkl. optokobler).

Blok 6 — AD8436 gain-modstande
Problem
R22/R23 er "efter datablad" — ikke dimensioneret.

Beslutningsproces
Trin 1: Fastlæg input/output-spændingsområde

ACV-indgang: 0–750 V RMS (efter dæmpning: 0–7,5 V RMS ved ADC-indgang? Nej — efter 1:100 dæmpning ved 750 V er signalet 7,5 V RMS, men AD8436's maksimale input er 3 V RMS . Så ved 750 V RMS skal dæmpningen være større end 1:100, eller AD8436 skal have ekstern dæmpning).

Kritisk: AD8436's maksimale input er 3 V RMS (8,5 V p-p) . Ved 750 V RMS kræves dæmpning på mindst 750/3 = 250:1. Den eksisterende 1:100 dæmpning er utilstrækkelig for ACV på høje ranges.

Løsning: Enten:

Tilføj ekstra dæmpning (1:3) før AD8436 for ACV-mode (kræver ekstra relæ/modstand).

Brug AD8436's interne FET-buffer med ekstern modstandsfordeler (f.eks. 1 MΩ + 2 MΩ = 1:3).

Begræns ACV til 250 V RMS (ikke acceptabelt iht. spec).

Anbefaling: Tilføj en ekstern 1:3 dæmpning (1 MΩ serie + 500 kΩ til jord) før AD8436's IBUFIN+ i ACV-mode. Dette kræver et ekstra relæ (K5) og to præcisionsmodstande.

Trin 2: Dimensioner gain
AD8436's output er 0–4 V DC for 0–3 V RMS input (typisk gain ~1,33). Med ekstern 1:3 dæmpning bliver input til AD8436 0–1 V RMS for 0–750 V RMS indgang. Ønsket output: 0–4 V DC.

AD8436's transferfunktion: 
V
o
u
t
=
V
i
n
,
R
M
S
×
G
V 
out
​
 =V 
in,RMS
​
 ×G, hvor 
G
G sættes af eksterne modstande. Fra databladet: 
G
=
1
+
R
f
/
R
g
G=1+R 
f
​
 /R 
g
​
  eller tilsvarende konfiguration.

Ved 
V
i
n
,
R
M
S
=
1
V 
in,RMS
​
 =1 V (svarende til 750 V RMS indgang efter 1:250 dæmpning): ønsket 
V
o
u
t
=
4
V 
out
​
 =4 V → 
G
=
4
G=4.

Trin 3: Vælg modstande
Ifølge AD8436-databladet er gain konfigureret via modstande tilsluttet til SUM-pinden. Typisk 
R
f
=
16
R 
f
​
 =16 kΩ (intern) og ekstern 
R
g
R 
g
​
 :

G
=
16
k
R
g
(forenklet model)
G= 
R 
g
​
 
16k
​
 (forenklet model)
For 
G
=
4
G=4: 
R
g
=
4
R 
g
​
 =4 kΩ.

Komponentvalg:

R
g
R 
g
​
 : 4,02 kΩ, 0,1 %, 25 ppm/°C, 0603

R
f
R 
f
​
 : intern (16 kΩ), ingen ekstern nødvendig

Ekstern dæmpning: 1 MΩ + 499 kΩ (0,1 %, 25 ppm/°C) for 1:3

Trin 4: Verifikation

Påtryk kendt AC RMS (f.eks. 1 V RMS, 50 Hz) og mål DC-output.

Verificer linearitet over 0,1–3 V RMS.

Verificer båndbredde ved 100 kHz (skal være −3 dB punkt >100 kHz).

Blok 9 — Optokobler LED-modstande
Problem
R3–R7 er "dimensioneres efter VCC_ISO" — ikke beregnet.

Beslutningsproces
Trin 1: 6N137 (fejl-interrupt)
6N137's LED kræver 
I
F
=
5
I 
F
​
 =5 mA minimum, maksimum 20 mA . Ved VCC_ISO = 5 V og 
V
F
≈
1
,
4
V 
F
​
 ≈1,4 V:

R
3
=
5
−
1
,
4
0
,
010
=
360
 
Ω
R 
3
​
 = 
0,010
5−1,4
​
 =360Ω
Vælg 330 Ω (E24) for 
I
F
≈
11
I 
F
​
 ≈11 mA.

Trin 2: PC817 (relædrivere)
PC817's CTR er 50–300 %. For at drive et Omron G6K relæ (coil 5 V, ~40 mA) via PC817's fototransistor:

I
C
=
C
T
R
×
I
F
I 
C
​
 =CTR×I 
F
​
 
Med 
C
T
R
=
50
CTR=50 % og ønsket 
I
C
=
40
I 
C
​
 =40 mA: 
I
F
=
80
I 
F
​
 =80 mA — for høj (PC817 max 
I
F
=
50
I 
F
​
 =50 mA).

Løsning: Brug Darlington-konfiguration med PC817 + ekstern transistor (f.eks. BC547), eller brug lavere coil-strøm relæ (Omron G6K-2P-Y har coil-strøm ~20 mA ved 5 V). Med 
I
C
=
20
I 
C
​
 =20 mA og CTR = 50 %: 
I
F
=
40
I 
F
​
 =40 mA — stadig for høj for kontinuerlig drift.

Anbefaling: Brug PC817 som driver for en MOSFET (f.eks. 2N7002) eller brug PC817 med lavere CTR-krav og en pull-up modstand på output, der driver relæet via en ekstern transistor.

Bedste løsning: Brug PC817 til at drive en BC547 (common-emitter), som driver relæspolen. PC817's 
I
F
=
10
I 
F
​
 =10 mA (R4 = 360 Ω), 
I
C
≈
5
I 
C
​
 ≈5 mA, som driver BC547's base (
h
F
E
=
100
h 
FE
​
 =100) → 
I
c
o
i
l
=
500
I 
coil
​
 =500 mA (rigeligt).

Trin 3: Komponentvalg

R3 (6N137): 330 Ω, 1 %, 0603

R4–R7 (PC817): 360 Ω, 1 %, 0603

BC547: 4 stk., TO-92

Pull-up på 6N137 output: 4,7 kΩ til VCC (ESP32-siden)

Trin 4: Verifikation

Mål 
I
F
I 
F
​
  i hver optokobler ved nominel spænding.

Mål relæets træk- og slip-tid (<5 ms for G6K).

Verificer 6N137's propagation delay (<100 ns).

Blok 10 — Klampertransistorer (Q2–Q5) og PTC2
Problem
"Efter dimensionering" — ikke specificeret.

Beslutningsproces
Trin 1: Fastlæg beskyttelsesbehov
Strømindgangen (µA/mA) skal beskyttes mod:

Overstrøm (f.eks. 230 V påtryk i strøm-mode)

Overspænding på ADC-indgang (<4,096 V)

Trin 2: Vælg klamper-topologi
Brug BJT-baseret aktiv klamper (som nævnt i HW-DOC §4.6): en PNP-transistor med base styret af en reference, der klamper spændingen ved ~4,5 V.

Kredsløb: For hver indgang (µA og mA):

PTC2: 100 mA holdestrøm, 600 V CAT III rated (f.eks. Bourns MF-R100)

Klamper: 2× 1N4148 til VCC_ISO/GND_ISO (grov beskyttelse)

Aktiv klamper: BC556 (PNP) med base til 4,5 V reference (fra LM393's reference-deler), emitter til indgang, collector til GND. Når indgangsspænding >4,5 V + 
V
B
E
V 
BE
​
 , leder BC556 og klamper spændingen.

Trin 3: Komponentvalg

PTC2: Bourns MF-R100 (1 A hold, 2 A trip) — ikke 100 mA som specificeret. 100 mA PTC er for lav for 10 A-range? Nej, PTC2 er kun for µA/mA-indgangen (max 600 mA). Vælg MF-R050 (500 mA hold) eller MF-R075 (750 mA hold) for at undgå unødig udkobling ved 600 mA.

Klampertransistorer: BC556 (PNP, 65 V, 100 mA) — 2 stk. (µA og mA)

1N4148: 4 stk. (2 per indgang)

Trin 4: Verifikation

Påtryk 230 V AC i mA-mode: mål restspænding på ADC-indgang (<4,1 V).

Mål PTC'ens reaktionstid (<1 s ved 2× holdestrøm).

Verificer at klamperen ikke påvirker normal måling (lækstrøm <1 nA ved 4 V).

Blok 1/2 — Transformator-MPN
Problem
"Myrra 44083" er nævnt, men ikke verificeret for viklingsadskillelse ≥8 mm.

Beslutningsproces
Trin 1: Fastlæg krav

Primær: 230 V AC

Sekundær 1: 9 V AC, min. 10 VA (digital)

Sekundær 2: 9V-0-9V AC, min. 10 VA (analog)

Viklingsadskillelse: ≥8 mm (for at opfylde CAT III-krav)

Isolationsspænding: ≥4 kV

Trin 2: Søg efter transformatorer med dokumenteret viklingsadskillelse
Myrra 44000-serien har 4 kV isolationsspænding , men viklingsadskillelsen er ikke specificeret i databladet. Dette er et problem for CAT III-certificering.

Anbefaling: Kontakt Myrra eller en anden producent (f.eks. Block, Hahn) for at få dokumenteret viklingsadskillelse. Alternativt: Vælg en medicinsk-grade transformator (f.eks. Block TIM-serien), som typisk har specificeret dobbelt isolation og viklingsadskillelse >8 mm.

Trin 3: Verifikation

Mål viklingsadskillelse på den fysiske komponent (skydelære).

Udfør hipot-test (4 kV RMS, 1 min) mellem primær og sekundær.

Blok 7 — TFT-MPN (RA8875)
Problem
"Buydisplay ER-TFTM050-3" er nævnt, men ikke verificeret.

Beslutningsproces
Trin 1: Verificer kompatibilitet
ER-TFTM050-3 er et 5" 800×480 TFT med RA8875-controller . Kritisk: Modulet kræver 3,3 V forsyning — 5 V vil ødelægge det . Jumper J8 skal være closed (3,3 V).

Trin 2: Verificer SPI-konfiguration
Modulet har pull-up modstande (R1/R2/R3) på SPI-linjerne, som kan forstyrre andre SPI-enheder . Løsning: Fjern R1/R2/R3 og C1/C2, eller brug en 74HC125 buffer mellem ESP32 og display.

Trin 3: Verificer strømforbrug
ER-TFTM050-3's baggrundsbelysning kræver typisk ~200 mA ved 3,3 V. ESP32's AMS1117-3.3 (800 mA) kan håndtere dette, men samlet last (ESP32 ~150 mA + TFT ~200 mA + ISO1541 ~5 mA + PC817 ~20 mA) = ~375 mA — inden for 800 mA, men med kun ~50 % margin.

Anbefaling: Vælg ER-TFTM050-3 med 3,3 V forsyning, fjern R1/R2/R3, og verificer samlet strømforbrug <600 mA.

Alternativ: Buydisplay ER-TFTM050-3 med RA8875 og SPI-grænseflade — dette er det eneste dokumenterede modul, der matcher firmwarens User_Setup.h.

Samlet komponentvalgsliste (nye/ændrede)
Blok	Komponent	Værdi/Type	MPN (forslag)	Antal	Kritisk parameter
3	MOV	550 V AC RMS, 20 mm	S20K550	1	MCOV ≥ 600 V AC
3	GDT	600–800 V DC	GTCX36-601M	1	Sparkover > MOV clamp
4	
R
S
R 
S
​
  (LSK170)	500 Ω, 0,1 %	—	1	25 ppm/°C
4	
R
D
R 
D
​
  (LSK170)	2,5 kΩ, 0,1 %	—	1	25 ppm/°C
4	
R
G
R 
G
​
  (LSK170)	100 MΩ, 1 %	—	1	Højimpedans
5	
R
s
e
n
s
e
R 
sense
​
  (1 mA)	2,5 kΩ, 0,1 %	—	1	25 ppm/°C
5	
R
s
e
n
s
e
R 
sense
​
  (100 µA)	25 kΩ, 0,1 %	—	1	25 ppm/°C
5	
R
s
e
n
s
e
R 
sense
​
  (10 µA)	250 kΩ, 0,1 %	—	1	25 ppm/°C
5	
R
s
e
n
s
e
R 
sense
​
  (1 µA)	2,5 MΩ, 0,1 %	—	1	25 ppm/°C
5	Reed-relæ (range)	5 V, lav lækstrøm	Coto 9007-05-01	3	Lækstrøm <1 pA
5	
R
t
o
p
R 
top
​
  (LM393)	10 kΩ, 1 %	—	1	—
5	
R
b
o
t
t
o
m
R 
bottom
​
  (LM393)	90,9 kΩ, 1 %	—	1	—
5	
R
P
U
R 
PU
​
  (LM393)	1 kΩ, 1 %	—	1	—
6	
R
g
R 
g
​
  (AD8436)	4,02 kΩ, 0,1 %	—	1	25 ppm/°C
6	Ekstern dæmpning (ACV)	1 MΩ + 499 kΩ, 0,1 %	—	2	25 ppm/°C
6	Relæ K5 (ACV dæmpning)	Omron G6K-2P-Y	G6K-2P-Y DC5	1	—
9	
R
3
R 
3
​
  (6N137)	330 Ω, 1 %	—	1	—
9	
R
4
R 
4
​
 –
R
7
R 
7
​
  (PC817)	360 Ω, 1 %	—	4	—
9	Driver-transistor	BC547	BC547B	4	
h
F
E
h 
FE
​
  ≥100
10	PTC2	500 mA hold, 600 V	MF-R050	2	CAT III
10	Klampertransistor	BC556	BC556B	2	65 V, 100 mA
1/2	Transformator	9 V + 9V-0-9V, 20 VA	Block TIM 20/230	1	Viklingsadskillelse ≥8 mm
7	TFT	5" 800×480 RA8875	ER-TFTM050-3	1	3,3 V kun
Verifikationsrækkefølge (anbefalet)
Blok 1/2: Mål transformator-isolation (hipot 4 kV), verificer ±5 V og 3,3 V.

Blok 3: Test MOV/GDT med transient-generator (6 kV, 8/20 µs).

Blok 4: Mål JFET-bias (DC-punkter), AC-sweep for båndbredde.

Blok 5: Mål hver strøm-range med præcisionsmultimeter.

Blok 5: Mål LM393 trigger/release-spænding.

Blok 6: Mål AD8436 output vs. input (linearitet, båndbredde).

Blok 9: Mål optokobler 
I
F
I 
F
​
  og relæ-timing.

Blok 10: Test klamper ved 230 V påtryk (mål restspænding).

Blok 7: Test TFT ved 20 MHz SPI, verificer ingen pixel-fejl.

Fuld systemtest: Kalibrer iht. CAL-PROC-V1.0, verificer alle ranges.

Kritiske beslutninger, der kræver ledelsesgodkendelse
MOV-valg: S20K550 (550 V AC) er større og dyrere end S10K300, men nødvendig for 600 V AC.

ACV-dæmpning: Ekstra 1:3 dæmpning kræver ekstra relæ og modstande — øger BOM og kompleksitet.

Strøm-range (10 nA): 250 MΩ modstand er upraktisk; alternativ (T-netværk) kræver flere komponenter og kalibrering.

Transformator: Medicinsk-grade transformator er dyrere, men nødvendig for dokumenteret viklingsadskillelse.

TFT-modul: ER-TFTM050-3 kræver 3,3 V og fjernelse af pull-ups — verificer før PCB-layout.