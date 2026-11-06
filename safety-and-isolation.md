# Sikkerhed og isolation — DMM-MASTER 16SRMS

> ⚠️ **Dette dokument beskriver designintentionen. Den fysiske prototype er
> ikke færdig, ikke testet og ikke certificeret. Følg sikkerhedsreglerne
> nedenfor ubetinget.**

## Sikkerhedsklassifikation (designmål)

- **IEC 61010-1 CAT III (600 V) / 1000 V DC**
- **Galvanisk isolation:** > 4000 V RMS
- **Krybeafstand:** ≥ 8 mm mellem Zone A og Zone B

## Zone A / Zone B

Instrumentet er opdelt i to zoner med en fysisk isolationsbarriere:

### Zone A (Kontroldomæne)

- ESP32-WROOM-32
- 5" TFT-skærm
- USB-CDC til PC
- Digital 5 V / 3,3 V forsyning

Denne zone er **ikke flydende** og kan berøres af brugeren.

### Zone B (Måledomæne)

- Analog front-end (AFE)
- AD8436 True RMS
- ADS1115 ADC
- Strømshunts
- Isoleret ±5 V forsyning

Denne zone er **galvanisk flydende** og er forbundet til det, der måles på.
Den kan være på livsfarlig spænding.

## Isolationens opretholdelse

1. **Fysisk afstand:** 8 mm krybeafstand på alle PCB-lag
2. **Isolationsslidser:** Fræses under optokoblere og ISO1541
3. **Datatrafik:** Kun via kapacitive isolatorer (ISO1541) og optokoblere
4. **Ingen metallisk kontakt** mellem zonerne

## "Hov-fejl"-beskyttelse

Hvis brugeren ved en fejl påtrykker 230 V i Ohm-mode:

1. **PTC-termistor** begrænser strømmen
2. **1N4148-klamper** klamper spændingen til +5,7 V / −0,7 V
3. **LM393-komparator** vipper til HIGH på <1 µs
4. **Optokobler** sender signal over isolationsbarrieren
5. **ESP32 IRAM-ISR** tvinger relæer i safe state
6. **GUI** viser rød skærm + buzzer
7. **Systemet låser** i `while(systemHalted)` — kun `*RST` kan frigive

## Brugsregler for prototype

1. **Forsøg ikke at måle på 230 V** med en hjemmebygget prototype
2. Brug kun **godkendte CAT III-testledninger** og -bøsninger
3. Overhold **8 mm krybeafstande** — verificeret i Gerber
4. **"Hov-fejl"-beskyttelsen er kun effektiv, hvis LM393 er korrekt implementeret**
5. Brug **altid sikkerhedsbriller** ved første strømpåtrykning
6. **Test aldrig alene** ved højspændingsmålinger

## Mekanisk sikkerhed

- Kabinet i **UL94 V-0 brandhæmmende plast**
- Interne plastbarrierer omkring 230 V-netindgang og HRC-sikringer
- 10 A / 1000 V **HRC-keramisk sikring** (sandfyldt) ved 10 A-indgangen

## Certificering

For at kunne kalde instrumentet **CAT III-certificeret** skal det testes hos
et akkrediteret testhus. Dette er **ikke sket** for dette projekt endnu.

Se `docs/HW-SW-SPEC-V6.0-MASTER.md` afsnit 2.0 og 6.0 for de fulde krav.