# DMM-MASTER 16SRMS

**Isoleret CAT III 4½-cifret bænkmultimeter (ESP32 / ADS1115 / AD8436)**

> ⚠️ **PROJEKTSTATUS: UNDER UDVIKLING (WORK IN PROGRESS)**
>
> Dette projekt er **ikke færdigt**. Firmware og dokumentation er langt, men følgende dele mangler stadig og er ikke publiceret endnu:
>
> - ❌ **Komplette diagrammer (KiCad/Altium)** — se [`hardware/schematics/`](hardware/schematics/README.md)
> - ❌ **PCB-layout og Gerber-filer** — se [`hardware/pcb/`](hardware/pcb/README.md)
> - ❌ **3D-printede/STEP-filer til kabinettet** — se [`hardware/enclosure/`](hardware/enclosure/README.md)
> - ❌ **Endelig, verificeret BOM med MPN'er** — se [`hardware/bom/`](hardware/bom/README.md)
> - ❌ **Kalibreringsmålinger fra fysisk prototype** — se [`tools/calibration/`](tools/calibration/README.md)
>
> Firmware er **funktionelt komplet i henhold til SRS-DMM-16SRMS-V6.0-MASSIVE**, men er endnu **ikke verificeret på fysisk hardware**.

---

## Indholdsfortegnelse

1. [Introduktion](#introduktion)
2. [Hvorfor dette projekt er anderledes](#hvorfor-dette-projekt-er-anderledes)
3. [Nøglespecifikationer](#nøglespecifikationer)
4. [Arkitektur i korte træk](#arkitektur-i-korte-træk)
5. [Hardware-overblik](#hardware-overblik)
6. [Firmware og software](#firmware-og-software)
7. [SCPI-kommunikation](#scpi-kommunikation)
8. [Sådan kommer du i gang](#sådan-kommer-du-i-gang)
9. [Projektstruktur](#projektstruktur)
10. [Hvad der stadig mangler](#hvad-der-stadig-mangler)
11. [Sikkerhed — læs dette først](#sikkerhed--læs-dette-først)
12. [Roadmap](#roadmap)
13. [Bidrag](#bidrag)
14. [Licens](#licens)

---

## Introduktion

**DMM-MASTER 16SRMS** er et open-source hardware/software co-design af et isoleret, 4½-cifret bænkmultimeter med CAT III (600 V) sikkerhedsklassifikation. Instrumentet er bygget op omkring en **ESP32-WROOM-32**, en **Texas Instruments ADS1115** 16-bit ΔΣ-ADC og en **Analog Devices AD8436** analog True RMS-konverter.

Til en estimeret udsalgspris på **ca. 825 kr. inkl. moms** (ved 1.000 enheders produktion) placerer instrumentet sig som et af markedets billigste åbne 4½-cifrede bænkmultimetre — og det eneste i sin prisklasse, hvor **hele kildekoden og hele hardware-designet er frit tilgængeligt**.

Projektet er primært målrettet:

- **R&D-elektronikudviklere** der vil integrere et DMM direkte i ATE- eller IoT-testmiljøer.
- **Universiteter og undervisere** der har brug for et instrument, hvor studerende kan læse og modificere *hele* koden (én `.ino`-fil, 100 % inline-kommenteret).
- **Avancerede hardware-magere og metrologi-entusiaster** der vil eksperimentere med egne kalibreringsalgoritmer.

> **Designfilosofi:** Dette instrument forsøger *ikke* at slå Fluke eller Keysight på rå præcision. Det er designet til at være den mest **åbne, modificerbare og veldokumenterede** platform i 4½-cifret-klassen.

---

## Hvorfor dette projekt er anderledes

| Funktion | Typisk lavpris-DMM | DMM-MASTER 16SRMS |
| --- | --- | --- |
| Firmware | Lukket, proprietær | ✅ 100 % open source (C++/Arduino) |
| Antal kildefiler | N/A | ✅ Én fil: `Multimeter_Firmware.ino` |
| Kommentar-dækning | N/A | ✅ 100 % inline-kommentarer (hver linje) |
| Kalibreringsalgoritme | Skjult | ✅ Læsbar i koden, gemmes i NVS |
| PC-interface | Proprietær app | ✅ Standard SCPI via USB-CDC |
| Datalogging | Begrænset / lukket | ✅ LittleFS + wear-leveling + SCPI-download |
| Galvanisk isolation | Ofte manglende | ✅ 8 mm barrier, >4000 V RMS |
| "Hov-fejl"-beskyttelse | Sjælden | ✅ LM393-komparator + IRAM-låst ISR |
| Dataloggingsfrekvens | Typisk ≤10 Hz | ✅ 100 Hz kontinuerligt |

---

## Nøglespecifikationer

### Metrologi

| Parameter | DCV | ACV (True RMS) |
| --- | --- | --- |
| Opløsning | 4½ cifre (60.000 counts) | 4½ cifre (60.000 counts) |
| Områder | 600 mV, 6 V, 60 V, 600 V, 1000 V | 600 mV, 6 V, 60 V, 600 V, 750 V |
| Præcision (1 år) | < ±0,025 % + 5 counts | 20 Hz – 100 kHz |
| Indgangsimpedans (lav) | > 1 GΩ (JFET-buffer) | DC-afkoblet |
| Indgangsimpedans (høj) | ≥ 10 MΩ | ≥ 10 MΩ |
| Crest-faktor | — | ≤ 3:1 ved fuldt udslag |

| Parameter | Modstand | Strøm |
| --- | --- | --- |
| Områder | 600 Ω – 60 MΩ (2W + 4W Kelvin) | 600 µA – 10 A (DC + AC) |
| Max opløsning | 0,01 Ω | Afhænger af shunt |
| Strømkilde / shunts | LT1006 + LM336-2.5 | 0,01 Ω Manganin + præcisionsshunts |

### System

- **MCU:** ESP32-WROOM-32 (dual-core, FreeRTOS)
- **ADC:** ADS1115, 16-bit ΔΣ, PGA = GAIN_ONE (±4,096 V)
- **True RMS:** AD8436 analog RMS-til-DC
- **Isolation:** >4000 V RMS, 8 mm krybeafstand (Zone A / Zone B)
- **Skærm:** 5" TFT 800×480 (SSD1963, 8-bit parallel)
- **Datalogging:** LittleFS på ESP32'ens flash, SCPI-downloadbar
- **Målehastighed:** 100 SPS kontinuerligt

---

## Arkitektur i korte træk
