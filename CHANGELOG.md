
---

## `CHANGELOG.md`

```markdown
# Changelog

Alle væsentlige ændringer i dette projekt dokumenteres her.
Formatet følger [Keep a Changelog](https://keepachangelog.com/da/1.1.0/)
og projektet følger [Semantic Versioning](https://semver.org/lang/da/).

## [Unreleased]

### Tilføjet
- `tools/python/dmm_master.py`: SCPI-klient til streaming og logging
- `.github/workflows/arduino-build.yml`: CI-build af firmware via arduino-cli

### Ændret
- `README.md`: Statusafsnit tilføjet for at understrege, at hardware-filer endnu ikke er publiceret

## [6.0.0] – 2026-11-05

### Tilføjet
- `firmware/Multimeter_Firmware.ino`: Komplet, monolitisk firmware, 100 % inline-kommenteret
- `firmware/User_Setup.h`: TFT_eSPI-konfiguration til 5" 800×480 SSD1963 (8-bit parallel)
- `docs/SRS-DMM-16SRMS-V6.0-MASSIVE.md`: Software Requirements Specification
- `docs/HW-SW-SPEC-V6.0-MASTER.md`: Global Engineering Specification Control Document
- `docs/technical-manual.md`: Teknisk manual med betjeningsvejledning

### Kendte mangler
- Komplette KiCad-skemaer er endnu ikke publiceret
- PCB-layout og Gerber-filer er endnu ikke publiceret
- 3D-kabinet (.STEP/.STL) er endnu ikke publiceret
- Fysisk prototype og kalibreringsdata er endnu ikke verificeret

## [0.1.0] – 2026-10-01

### Tilføjet
- Første udkast til SRS og arkitekturdiagrammer (internt review)