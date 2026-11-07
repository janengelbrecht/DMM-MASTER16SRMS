
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