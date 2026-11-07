
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