
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

## Designkrav (uddrag)

Fulde krav findes i [`docs/HW-SW-SPEC-V6.0-MASTER.md`](../docs/HW-SW-SPEC-V6.0-MASTER.md).

- **Isolation:** ≥ 8 mm krybeafstand mellem Zone A og Zone B, >4000 V RMS
- **PCB:** Anbefalet 4-lags, 1 oz indre / 2 oz ydre kobber, FR4 ≥1,6 mm
- **Isolationsslidser:** Skal fræses under optokoblere og ISO1541
- **Guard-rings:** Omkring højimpedansbaner i Blok 4
- **Termisk design:** Manganin-shunt og regulatorer skal have thermiske vias
- **Kabinet:** UL94 V-0 brandhæmmende plast med interne barrierer

## Hvordan du kan hjælpe

Se [`CONTRIBUTING.md`](../CONTRIBUTING.md) — især afsnittene om hardware-design
og 3D-kabinet.

Prioriteret rækkefølge:

1. **KiCad-skemaer** for alle 10 blokke
2. **PCB-layout** med isolationsslidser og guard-rings
3. **BOM** med verificerede MPN'er og tolerancer
4. **Kabinet** med CAT III-barrierer og 19 mm bøsningsafstand