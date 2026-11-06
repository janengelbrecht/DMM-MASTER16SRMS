# SCPI-kommandoreference — DMM-MASTER 16SRMS

Dette dokument beskriver det fulde SCPI-kommandosæt, som DMM-MASTER
understøtter. Protokollen følger IEEE 488.2 / SCPI-1999.

## Fysisk forbindelse

- USB-CDC Virtual COM Port
- 115200 baud, 8N1, ingen flow control
- Alle kommandoer afsluttes med `\n` (LF)
- Alle svar afsluttes med `\n`

## A. System- og standardkommandoer (IEEE 488.2)

### `*IDN?`

Identification Query. Returnerer instrumentets identifikation.

**Svar:** `DMM-MASTER,16SRMS,SN1,V6.0\n`

### `*RST`

Reset. Frigiver fejllås, nulstiller FSM, sætter instrumentet i sikker
DCV-mode (1000 V range).

## B. Instrumentkonfiguration

### `CONF:VOLT:DC [<range>]`

Skift til DC spændingsmåling.

- **Uden `<range>`:** Auto-ranging aktiveres
- **Med `<range>`:** Manuelt gear (0–4), auto-ranging deaktiveres

**Eksempel:** `CONF:VOLT:DC 2` (60 V range, manuelt)

### `CONF:RES [<range>]`

Skift til 2-leder modstandsmåling.

### `CONF:FRES [<range>]`

Skift til 4-leder (Kelvin) modstandsmåling.

## C. Måling

### `READ?`

Returnerer seneste færdige måling som ASCII med 5 decimaler.

**Svar:** `5.02341\n`

**Svartid:** < 5 ms

### `MEAS:VOLT:DC?`

Alias for `READ?` (for kompatibilitet med eksisterende scripts).

## D. Fil- og hukommelseshåndtering (MMEM)

### `MMEM:CAT?`

List alle filer på LittleFS.

**Svar:** Én linje per fil: `<filnavn>,<størrelse_i_bytes>`

### `MMEM:DATA? "<filnavn>"`

Dump hele filindholdet over USB.

**Eksempel:** `MMEM:DATA? "log.csv"`

### `MMEM:DEL "<filnavn>"`

Slet en fil.

**Eksempel:** `MMEM:DEL "log.csv"`

## E. Remote mode

### `SYST:REM`

Sæt instrumentet i Remote mode. GUI-opdateringer deaktiveres for at frigive
CPU-cyklusser til maksimal SCPI-hastighed.

### `SYST:LOC`

Forlad Remote mode. GUI-opdateringer genoptages.

## F. Kalibrering

### `CAL:ZERO`

Zero-punkt-kalibrering. Gemmer nuværende raw-værdi som offset for det
aktive gear.

**Forudsætning:** Indgangen skal være kortsluttet.

### `CAL:VAL <reference>`

Span-punkt-kalibrering. Beregner ny gain ud fra en kendt reference.

**Eksempel:** `CAL:VAL 5.0`

**Forudsætning:** En kendt reference på 5,0000 V skal være påtrykt indgangen.

## Fejlhåndtering

### Overload

Ved overload returnerer `READ?` værdien `9.9E+37` (IEEE 488.2-standard
for Infinity/Overload).

### Ugyldig kommando

Ukendte kommandoer ignoreres lydløst. Der returneres intet svar.