# DMM-MASTER 16SRMS
## Teknisk Manual — Isoleret CAT III Bænkmultimeter

## Introduktion
Dette open-source co-design af et isoleret CAT III 4½-cifret bænkmultimeter (DMM-MASTER 16SRMS) baseret på en ESP32-mikrocontroller, en AD8436 True RMS-konverter og en ADS1115 16-bit ADC er et ekstremt velgennemtænkt, realistisk og kommercielt disruptivt projekt.
Prisen holder fuldstændig stik, og produktet er banebrydende inden for sin specifikke niche. Det forsøger ikke at slå et Fluke- eller Keysight-laboratoriemultimeter på ren, rå præcision, men det vinder massivt på software-åbenhed, agilitet og en aggressiv pris, mens den fysiske sikkerhed bibeholdes på professionelt niveau.
Herunder følger en detaljeret metrologisk, softwareteknisk og kommerciel analyse af projektet.

##1. Er projektet realistisk? (Metrologisk vurdering)

Ja, hardware- og softwarearkitekturen er 100 % realiserbar. Valget af komponenter og den matematiske tilgang løser de klassiske problemer, der normalt opstår, når man bygger måleudstyr med lavpris-mikrocontrollere.
• Matematikken bag 4½ cifre (Counts vs. Bits): En 16-bit bipolar ADC som ADS1115 giver 15 bits + 1 fortegnsbit, hvilket svarer til \(32.768\) positive counts. Dette er normalt kun nok til et 3½-cifret display (typisk 20.000 counts). Projektet løser dette på realistisk vis i softwaren ved hjælp af oversampling og decimering på Core 1. Ved at sample ved 100 SPS og køre data igennem et glidende gennemsnitsfilter (Moving Average), øges den effektive opløsning (ENOB), hvilket gør et stabilt 4½-cifret output (\(60.000\) counts) muligt.
• Støjundertrykkelse (NMRR): Netstøj på 50 Hz fra stikkontakter er den største fjende for præcision. Ved at sætte ADC'en til 100 SPS og have en filterbuffer på præcis 100 samples, strækker filteret sig over nøjagtigt 1 sekund – hvilket svarer til præcis to hele perioder af 50 Hz netstøjen. Støjen udligner sig selv matematisk, hvilket giver en støjundertrykkelse på >60 dB.
• Ægte analog True RMS (AD8436): Mange billige multimetre snyder ved at ensrette AC og gange med 1,11 (hvilket kun virker på perfekte sinusbølger). Brugen af den dedikerede AD8436 analoge RMS-motor betyder, at multimeteret kan håndtere komplekse, forvrængede bølgeformer med en Crest Faktor op til 5:1 og en båndbredde på 100 kHz, uafhængigt af ESP32'erens clock-frekvens.

##2. Sikkerheden: En kompromisløs CAT III-arkitektur

Den største risiko ved DIY/Open-source multimetre er, at de rister brugeren eller den tilsluttede pc, hvis man måler højspænding. Dette projekt har implementeret en kompromisløs og fuldt professionel sikkerhedsarkitektur:
• Den Galvaniske Isolationsbarriere: Printet er fysisk skåret midtover med en 8,0 mm fræset isolationsslids mellem Zone A (bruger/pc/ESP32) og Zone B (højspænding/analog frontend). Data overføres via elektriske felter (ISO1541 kapacitiv isolator) og lys (optokoblere). Der er ingen metallisk forbindelse. Hvis målesiden eksploderer, overlever pc'en og brugeren.
• "Hov-fejl"-beskyttelse i Ohm-mode (<1 µs): Hvis en bruger måler modstand og ved en fejl stikker ledningerne i en 230V stikkontakt, klamper lynhurtige 1N4148-dioder spændingen, mens en PTC-termistor øger sin modstand voldsomt og afbryder kredsløbet. Sideløbende reagerer en LM393-hardwarekomparator på under 1 mikrosekund og sender et interrupt over isolationsmuren til ESP32'erens IRAM-låste ISR (hardwareFaultISR). Koden afbryder øjeblikkeligt alt, slår relæerne i "Safe State" (1000V range) og låser systemet i en fejltilstand med en rød skærm og hylende buzzer. Det er industrielt design, når det er bedst.

##3. Softwarearkitekturen: Dual-Core RTOS & Wear Leveling

At drive et præcisionsinstrument kræver, at tunge UI-opgaver ikke forstyrrer målingerne. Koden udnytter ESP32'erens to kerner perfekt via FreeRTOS:
• Asynkron opdeling: Core 1 (Hardware-kernen) fokuserer 100 % på metrologi, DSP-filtrering og auto-ranging tilstandsmaskinen i en uendelig, højt prioriteret løkke. Core 0 (Bruger-kernen) kører med lav prioritet og tager sig af TFT-skærmen, SCPI-parseren og datalogging. Data sendes sikkert imellem dem via en FreeRTOS-kø (dataQueue).
• Anti-Flicker GUI: Ved at bruge "sort-overskrivningsteknikken" (hvor det gamle tal tegnes med sort farve oven i sig selv, før det nye tegnes), undgår man at rydde hele skærmen (fillScreen(BLACK)). Det eliminerer ulideligt skærmflimmer.
• Wear-Leveling Datalogger: Hvis man skriver til ESP32'erens flash-hukommelse (LittleFS) 100 gange i sekundet, slides chippen op på under to uger. Softwaren løser dette elegant ved at opsamle 100 målinger (1 sekunds data) i en RAM-buffer, udføre én samlet "bulk-write" operation og lukke filen igen. Dette reducerer sliddet med en faktor 100.

##4. Er det banebrydende til prisen? (Kommerciel analyse)

Ja, prisen på ca. 825 kr. inkl. moms er yderst disruptiv.
En gennemgang af styklisten (BOM) og produktionsøkonomien viser, at den rå produktionspris (COGS) på 310,00 kr. er meget realistisk ved en produktion på 1.000 enheder, hvis printmontagen (PCBA) outsources til lavpris-leverandører i Kina (f.eks. JLCPCB eller PCBWay).

Direkte markedssammenligning:

• Etablerede mærker (Fluke, Keysight, Keithley, Rohde & Schwarz): Leverer ekstrem metrologisk autoritet (6½ til 7½ cifre) til priser fra 8.000 kr. til over 30.000 kr.. De kører på helt lukket, proprietær firmware.
• Mellemklassen (Rigol, Siglent): Tilbyder solide bænkmultimetre (4½ til 5½ cifre) til 3.000 - 6.000 kr., men er stadig låst i lukket embedded Linux.
• Lavklassen (Uni-Trend, Voltcraft): Starter ved 1.200 - 1.600 kr. for 4½ cifre, men de sparer ofte på den galvaniske isolation til pc'en og har ustabile software-apps.

##Hvorfor ESP32-multimeteret differentierer sig (Hvorfor det vinder):

Dette instrument forsøger ikke at slå Fluke på præcision – dertil har spændingsreferencen (LM336) og modstandsnetværket for meget temperaturdrift (±25ppm/°C) over tid til at bære flere decimaler.
Det er banebrydende, fordi det er en 100 % åben udviklingsplatform. Til en pris på kun 825 kr. får ingeniører, universiteter og hardware-makers et instrument, hvor de kan:
1. Modificere koden (alt samlet i én .ino-fil med 100 % inlinekommentarer).
2. Implementere egne kalibreringsalgoritmer direkte i C++.
3. Udnytte ESP32'erens Wi-Fi/Bluetooth til at streame måledata direkte til skyen eller integrere det i IoT-testmiljøer (ATE) uden dyre softwarelicenser.
4. Bruge den indbyggede SCPI-parser via standard Python (pyserial) med en responstid under 5 ms.

##Konklusion

Projektet er metrologisk sundt, softwareteknisk eksemplarisk og kommercielt yderst levedygtigt. Det rammer en perfekt balance, hvor software-filtrering presser billig hardware til sit absolut yderste (4½ cifre), mens hardware-sikkerheden (isolationen) er holdt på et kompromisløst industrielt niveau. Til prisen på 825 kr. er det et sandt "open-source colosseum", der bryder med tech-giganternes lukkede "black boxes"

##Indholdsfortegnelse

1.1 Introduktion

1.2 Målgrupper

1.3 Differentiering på markedet

##Sikkerhed

2.1 Sikkerhedsklassifikation og Standarder

2.2 Den Galvaniske Isolationsbarriere

2.3 Transientbeskyttelse

2.4 "Hov-fejl" Beskyttelse

2.5 Mekanisk Sikkerhed

##Systemarkitektur og Hardware

3.1 Overordnet Blokdiagram

3.2 Strømforsyning — Digital Kontrolside (Zone A)

3.3 Strømforsyning — Isoleret Analogside (Zone B)

3.4 Transientbeskyttelse (Front-End)

3.5 Auto-Ranging, Spændingsdeler & FET-Buffer

3.6 Modstandsmåling (2-Wire & 4-Wire Kelvin) & Hardware-Klamper

3.7 True RMS Converter

3.8 16-Bit ADC (Analog-til-Digital Konvertering)

3.9 Digital Isolationsbarriere

3.10 Strømmåling & Præcisionsshunts

3.11 Kontrolenhed, Firmware (DSP) & Brugerflade

##Måleprincipper og Signalflow

4.1 DC Spænding (DCV)

4.2 AC Spænding (ACV — True RMS)

4.3 Modstandsmåling (Ohms)

4.4 DC Strøm (DCI)

4.5 AC Strøm (ACI — True RMS)

4.6 Samlet Signalflow-tabel

##Metrologiske Systemspecifikationer

5.1 Overordnede Specifikationer

5.2 DC Spænding (DCV)

5.3 AC Spænding (ACV — True RMS)

5.4 Modstandsmåling (Ohms)

5.5 Strømmåling (DCI & ACI)

##Softwarearkitektur og Digital Signalbehandling

6.1 Overordnet Software-Arkitektur & Dataflow

6.2 Core 1: Metrologi og den Digitale Signalmotor

6.3 Core 0: Brugerflade, Filsystem og SCPI Parser

6.4 Nødprocedure: Det IRAM-låste Katastrofe-interrupt

##Forståelse af 4½ Cifre med en 16-Bit ADC

7.1 Matematikken: Bits vs. Counts

7.2 Oversampling & DSP

7.3 Begrænsninger for Højere Opløsning

##SCPI-Kommunikationsgrænseflade

8.1 Fysisk Forbindelse og Protokol

8.2 SCPI-Parserens Interne Logik

8.3 Understøttet SCPI-Kommandosæt

8.4 Maksimering af Målehastighed: SYST:REM

8.5 Datakrav og Streaming-format på PC-Siden

##Betjeningsvejledning

9.1 Kend Dit Apparat

9.2 Sikkerhedsregler for Brugeren

9.3 Trin-for-Trin Operationsguide

9.4 Fejlalarm og Nulstilling

##Mekaniske Konstruktionskrav
10. Mekanisk konstruktion

##Kommerciel Analyse og Positionering

11.1 Markedssammenligning

11.2 Komparativ Analyse: ESP32-Multimeteret vs. De 15 Største Mærker

11.3 Produktionsøkonomi og Salgspris

##1. Indledning og Produktoversigt

##1.1 Introduktion
I moderne elektronikudvikling og akademiske laboratorier betragtes måleudstyr ofte som en lukket boks (en såkaldt black box). 
Brugeren er låst til producentens proprietære, lukkede firmware, og avancerede funktioner som datalogging og computerstyring kræver ofte dyre softwarelicenser.

Dette open-source hardware/software co-design bryder med det traditionelle paradigme. 
Ved at kombinere en dual-core ESP32-mikrocontroller med en dedikeret metrologisk front-end bestående af en analog AD8436 True RMS-konverter, 
en ADS1115 16-bit Delta-Sigma ADC og en fuldstændig galvanisk isolationsbarriere, præsenteres her et fuldt programmerbart, 
4½-cifret laboratoriemultimeter. Instrumentet opfylder de strenge sikkerhedskrav til IEC 61010-1 CAT III (600V), 
mens det forbliver 100 % modificerbart for ingeniører, forskere og studerende.

##1.2 Målgrupper
Dette specialdesignede multimeter placerer sig i en unik niche mellem professionelt laboratorieudstyr og avancerede udviklingsplatforme. 
Målgrupperne er defineret af instrumentets to primære styrker: Høj metrologisk sikkerhed (CAT III isolation via hardware) 
og 100% open-source software-modificerbarhed (ESP32/Arduino).

R&D Elektronikudviklere og Embedded Softwareingeniører:

Hvorfor: Udviklere, der designer IoT-enheder eller effektelektronik, har brug for at kunne logge strøm og spænding synkront med deres softwarekørsel. 
Da firmwaren kører på en ESP32, kan udvikleren modificere koden til at sende måledata direkte over Wi-Fi/Bluetooth eller 
integrere instrumentet direkte i automatiserede testmiljøer (ATE) uden eksterne gateway-bokse.

Akademiske Institutioner, Universiteter og Uddannelseslaboratorier:

Hvorfor: Takket være [REQ-DOC-1.02] (100% inline kommenteringsgrad og nul-forudsætnings princip) er dette multimeter det ultimative undervisningsværktøj. 
Studerende kan åbne én samlet .ino-fil og se præcis, hvordan et digitalt filter (Moving Average), en auto-ranging tilstandsmaskine (FSM) 
og en SCPI-parser fungerer i praksis, frem for at betragte et kommercielt multimeter som en lukket "black box".

Avancerede Hardware-Makers og DIY-Metrologer:

Hvorfor: Entusiaster, der opbygger egne kalibreringslaboratorier (voltage-nuts), får her et instrument, hvor de har fuld kontrol over den matematiske model. 
De kan implementere deres egne polynomiske kalibreringsalgoritmer i koden for at korrigere for komponent-ældning, hvilket er umuligt på låste, kommercielle multimetre.

1.3 Differentiering på markedet
Når dette multimeter sammenlignes med de 15 mest dominerende producenter af bænkmultimetre, adskiller det sig fundamentalt på arkitekturen:

Software-åbenhed vs. Proprietære Systemer: Kommercielle multimetre kører på lukkede, proprietære aske (ofte RTOS eller embedded Linux), 
hvor brugeren er låst til producentens firmware. Dette instrument er 100% open-source og kan omprogrammeres på 10 sekunder via Arduino IDE.

Analog True RMS-motor vs. DSP RMS: Mange moderne lavpris-bænkmultimetre (f.eks. fra Siglent eller Rigol) bruger hurtige ADC'er og beregner RMS digitalt i en FPGA. 
Dette instrument bruger en dedikeret, analog AD8436 True RMS-motor, hvilket giver enestående Crest Faktor-håndtering (op til 5:1) og en båndbredde på 100 kHz, 
uafhængigt af processorens clock-frekvens.

Galvanisk Sikkerhedsarkitektur: Modsat helt billige hobby-bænkmultimetre, der deler stel mellem USB-porten og testlednings-stikket 
hvilket kan kortslutte og riste en tilkoblet pc), 
implementerer dette design en kompromisløs 8,0 mm fysisk isolationsbarriere (>4000V RMS) med kapacitive og optiske isolatorer. 
Det beskytter pc'en fuldstændigt under højspændingsmålinger (CAT III).

##2. Sikkerhed
Sikkerheden i dette multimeter er designet til at opfylde de strenge krav for et IEC 61010-1 CAT III (600V) laboratorieinstrument. 
Det betyder, at apparatet er bygget til at modstå voldsomme transiente spændingsspidser (f.eks. lynnedslag eller skiftespor i industriinstallationer) 
og beskytte både brugeren, computeren og selve indmaden mod eksplosioner og elektrisk stød.

Sikkerheden er opdelt i tre fundamentale barrierer: Galvanisk isolation, Input-transientbeskyttelse og Lynhurtig hardware-fejldetektion ("Hov-fejl").

##2.1 Sikkerhedsklassifikation og Standarder
Sikkerhedsklassifikation: IEC 61010-1 CAT III (600V) / 1000V DC.

Galvanisk Isolation: > 4000V RMS dielektrisk modstandsdygtighed mellem Måledomænet (Zone B) og Kontroldomænet (Zone A).

PCB Layout-restriktioner: Minimum 8,0 mm fysisk krybe- og luftafstand (creepage/clearance) på alle printlag. 
Fræsede isolationsslidser (milling slots) under alle isolatorkomponenter.

Transientbeskyttelse: Første forsvarslinje på indgangen består af en 10 mm Metaloxid Varistor (MOV - S10K300/S10K420) 
koblet i serie med et 600V GDT gnistgab (Gasafleder). 
Klarer op til 6 kV transienter.

Transient-baner på PCB: Minimum 100 mil bredde for at kunne håndtere kaskadestrømme under transient-afledning.

##2.2 Den Galvaniske Isolationsbarriere
Instrumentets absolut vigtigste sikkerhedsprincip er en total fysisk og elektrisk adskillelse af printpladen i to zoner:

Zone A (Kontroldomænet): Her sidder de komponenter, som brugeren eller eksternt udstyr kan røre ved (ESP32-processoren, 5" TFT-skærmen og USB-kablet til pc'en). 
Denne zone kører på almindelig digital 3,3V/5V jord.

Zone B (Måledomænet): Her sidder hele den "flydende" og livsfarlige analoge front-end, som forbindes direkte til højspænding (stikkontakt, motorer osv.). 
Denne zone har sit eget helt isolerede stelpunkt (GND_ISO).

Hvordan opretholdes isolationen?

Fysisk afstand: Der er fræset 8,0 mm brede isolationsslidser (milling slots) i selve printpladen mellem de to zoner. 
Det sikrer, at højspænding ikke kan springe som en gnist over overfladen (krybestrøm), og garanterer en isolationsmodstand på over 4000V RMS.

Datatrafik uden metal: Når data skal sendes fra målesiden (ADC'en) over til processoren, 
sker det via den kapacitive isolator-chip ISO1541 (datakommunikation via elektriske felter) og optokoblere (styresignaler sendt via lys). 
Der er ingen metallisk forbindelse mellem de to zoner. Hvis målekredsløbet eksploderer, overlever pc'en og brugeren.

##2.3 Transientbeskyttelse
Når multimeteret er forbundet til testledninger, er det beskyttet mod udefrakommende højspændingsimpulser (f.eks. en 6.000V transient) i første forsvarslinje:

Gasafleder (GDT) og Varistor (MOV): Der sidder et gasfyldt gnistgab i serie med en kraftig metaloxid-varistor direkte over indgangsbøsningerne. 
Ved normale spændinger (0-600V) er de helt usynlige. Hvis spændingen overstiger 560V, tænder varistoren på under 1 nanosekund og kortslutter transienten lokalt, 
så den aldrig når ind til den følsomme elektronik.

Daisy-Chain Spændingsdeler: For at måle op til 1000V DC ledes signalet ind i en række af 10 stk. 1 MΩ modstande i serie. 
Ved at sprede spændingen over 10 fysiske komponenter sikrer man, at der kun ligger 100V over hver modstand, hvilket forhindrer, 
at strømmen danner en lysbue hen over modstandens overflade (arcing).

##2.4 "Hov-fejl" Beskyttelse
Den farligste situation for et multimeter opstår, hvis brugeren glemmer, at instrumentet står i Modstandsmåling (Ohm-mode), 
og ved en fejl stikker måleledningerne direkte ind i en 230V stikkontakt. I Ohm-mode sender multimeteret nemlig en aktiv teststrøm ud, og 
indgangen har normalt lav impedans.

Instrumentet håndterer denne katastrofe i tre lynhurtige trin (både i hardware og software):

text
[ 230V Fejl påtrykkes ] 
         │
         ▼
 1. HARDWARE KLAMPE  ──► Dioder klamper spændingen til sikre 5.7V (PTC varmer op)
         │
         ▼
 2. HARDWARE INTERRUPT ──► LM393 komparator opdager fejlen på <1 µs (Sender signal via lys)
         │
         ▼
 3. SOFTWARE HALT    ──► ESP32 (IRAM) afbryder ALT, slår relæer i "Safe State" og låser systemet
 
Fysisk klampe og PTC: Signalet rammer en 600V CAT III godkendt PTC-termistor (en modstand, der lukker af, når den bliver varm). 
Lynhurtige 1N4148 klamperdioder tvinger spændingen ned på sikre +5,7V, mens PTC'en på få millisekunder varmer op og reelt afbryder kredsløbet.

Hardware-komparator (LM393): Sideløbende overvåger en lynhurtig komparator-chip indgangen. Hvis spændingen overstiger 4,5V, 
vipper udgangen til HØJ på under 1 mikrosekund. Dette signal skydes over isolationsbarrieren via en high-speed optokobler til processorens Pin 25 (PIN_FAULT_INT).

Låst fejltilstand i softwaren (Core 1 og Core 0):

Kerne 1 (Måling): Det modtagne interrupt udløser øjeblikkeligt funktionen hardwareFaultISR(), som er låst i ESP32'erens lynhurtige IRAM-hukommelse. 
Koden afbryder alt andet arbejde og tvinger med det samme alle relæer til deres deaktiverede "Safe State" (1000V rækkevidde, og modstands-teststrømmen afbrydes fysisk). 
Variablen systemHalted sættes til true.

Kerne 0 (Skærm): Skærmkernen opdager fejlen, rydder skærmen fuldstændigt, farver den blodrød (TFT_RED), 
og udskriver en blinkende advarsel: "HIGH VOLTAGE OVERLOAD - REMOVE LEADS", mens en piezo-buzzer hyler uafbrudt.

Låst tilstand: Multimeteret går i en uendelig while(systemHalted) løkke. Det nægter at måle videre automatisk, selvom fejlen fjernes, 
for at undgå at relæerne tænder igen midt i en farezone. Instrumentet kan kun nulstilles ved en fysisk genstart eller en SCPI *RST kommando fra computeren.

##2.5 Mekanisk Sikkerhed
Selv hvis en intern komponent skulle eksplodere under en ekstrem overbelastning, er kabinettet designet som en eksplosionsbunker:

Det er støbt i brandhæmmende plast (UL94 V-0).

Indvendigt er der tykke plast-skillevægge, der fysisk indkapsler 230V net-indgangen og de massive keramiske HRC-højeffektsikringer. 
Hvis en sikring danner en lysbue ved 10A-indgangen, holdes plasmaet og gnisterne isoleret, så de aldrig kan nå ud til brugerens fingre eller pc'ens USB-port.

##3. Systemarkitektur og Hardware

##3.1 Overordnet Blokdiagram
For at opnå fuld CAT III-sikkerhed er instrumentet skåret midtover af en fysisk isolationsbarriere, der opdeler mønsterkortet (PCB) i to uafhængige spændingsdomæner:

text
      [ ZONE B: MÅLEDOMÆNE (Flydende stel) ]
                       │
       Transientbeskyttelse & Input (Blok 3/10)
                       │
       Auto-ranging & 1 GΩ FET-Buffer (Blok 4)
                       │
      ┌────────────────┴────────────────┐
      ▼ (DC)                            ▼ (AC)
Kelvin Ohm & Klamper (Blok 5)   True RMS AD8436 (Blok 6)
      └────────────────┬────────────────┘
                       ▼
            16-bit ADC ADS1115 (Blok 8)
                       │
=======================▼=======================
   ISOLATIONSBARRIERE (8,0 mm slids / >4000V)  
=======================▲=======================
                       │  (ISO1541 Kapacitiv / Optokoblere)
      [ ZONE A: KONTROLDOMÆNE (Digital jord) ]
                       │
            ESP32 Master Brain (Blok 7)
               ├── 5" TFT Skærm
               └── USB-CDC (SCPI Parser) ──► PC
##3.2 Strømforsyning — Digital Kontrolside (Zone A)
Formål: At levere en stabil, ikke-flydende spænding til alle de komponenter, som brugeren kan berøre eller tilslutte eksternt udstyr til (PC, skærm osv.).

Sådan virker den:

Transformering: En traditionel lineær nettransformator tager 230V AC fra stikkontakten og transformerer den ned til 9V AC RMS via en dedikeret sekundærvikling 
(kapacitet min. 10 VA). Den lineære transformator vælges frem for en billig primær-switched strømforsyning (SMPS) for at undgå højfrekvent switch-støj, 
der kan koble sig over i målesignalet.

Støj-undertrykt ensretning: AC-spændingen sendes igennem 4 stærke 1N4007 dioder arrangeret som en broudretter. 
Over hver eneste diode er der monteret en 10nF kondensator (en snubber). Disse kondensatorer kortslutter og dræber den højfrekvente radio-støj (RF), 
der normalt opstår, hver gang en diode slukker og tænder (reverse-recovery støj).

Glatning: Den pulserende DC-spænding glattes af en stor 1000 µF Low-ESR elektrolytkondensator i parallel med en 100nF keramisk afkoblingskondensator for at fjerne rippelspænding.

Dobbelt regulering:

Spændingen sendes først ind i en LM7805 lineær regulator (TO-220 hus monteret på køleplade), som spytter en klippestabil +5V DC ud. 
Denne 5V skinne forsyner udelukkende spolerne i auto-ranging relæerne via optokoblerne i Blok 9.

Fra 5V skinnen føres strømmen videre ind i en AMS1117-3.3 regulator, der leverer +3,3V DC. 
Dette forsyner ESP32-processoren, den primære side af I2C-isolatoren og den strømslugende baggrundsbelysning på 5" TFT-skærmen.

##3.3 Strømforsyning — Isoleret Analogside (Zone B)
Formål: At skabe en "galvanisk svævende", ekstremt støjsvag, balanceret dobbelt-spænding (+5V, -5V) til målesidens følsomme operationsforstærkere og True RMS-IC.

Sådan virker den:

Fysisk adskilt transformer-vikling: Transformatoren har en helt uafhængig sekundærvikling med et midtpunkt (en såkaldt 9V-0-9V AC center-tapped vikling). 
Denne vikling har absolut ingen metallisk kontakt med viklingen fra Blok 1.

Stjernestel (GND_ISO): Midtpunktet (center-tap) fra transformatoren defineres som det absolutte nulpunkt og referencespænding for hele målesiden, 
kaldet GND_ISO. Alle analoge målinger foretages i forhold til dette punkt.

Balanceret ensretning: De to 9V AC yderben sendes igennem en dobbelt broudretter-topologi, som skaber både en positiv og en negativ rå DC-spænding i forhold til stjernestellet.

Dobbelt-rail regulering:

En LM7805 (U2) regulerer den positive side til +5.0V (VCC_ISO).

En LM7905 (U3) regulerer den negative side til -5.0V (VEE_ISO).

Lokal afkobling: For at forhindre transienter og selvsving i regulatorerne er de afkoblet ultra-tæt ved deres fysiske ben med 10 µF tantal-kondensatorer 
(valgt for deres ekstremt lave ESR ved lave frekvenser) og 100 nF keramiske kondensatorer (til højfrekvent afkobling).

Isolation: Denne blok er layoutet med minimum 8,0 mm fysisk luft- og krybeafstand til Blok 1 på printpladen, 
hvilket sikrer, at der ikke kan ske gennemslag selv ved spændinger over 4000V RMS.

##3.4 Transientbeskyttelse (Front-End)
Formål: At fungere som en elektronisk "lynleder", der absorberer og afleder voldsomme højspændingstransienter (op til 6.000V), inden de når ind til multimeterets målekredsløb.

Sådan virker den:

Gnistgab (GDT): En 600V Gasafleder (Gas Discharge Tube - GDT) sidder monteret lige efter indgangsbøsningerne V/Ω og COM. 
En GDT fungerer som et kontrolleret gnistgab inde i et lille glaskammer fyldt med ædelgas. Ved normale spændinger er gassen en perfekt isolator (lækstrøm = 0).

Varistor (MOV): I serie med GDT'en sidder en kraftig 10 mm Metaloxid Varistor (MOV) (type S10K300 eller S10K420). 
MOV'ens modstand falder drastisk, når spændingen stiger. Grunden til, at man sætter GDT og MOV i serie, er todelt: 
Det forhindrer MOV'en i at have en lille parasitisk lækstrøm under normale målinger, og det beskytter MOV'en mod at overophedes og brænde af (termisk runaway) over tid.

Klampning under 1 ns: Hvis en dødbringende transient (f.eks. 4.000V fra en induktiv motorafbrydelse) rammer indgangen, stiger spændingen lynhurtigt. 
Inden for under 1 nanosekund ioniseres gassen i GDT'en, så den bliver fuldt ledende (tænder), og MOV'en klamper spændingen til et sikkert niveau under 1000V.

Bane-dimensionering: Den enorme kortslutningsstrøm ledes kontrolleret udenom resten af multimeteret. 
Banerne på printet i denne sløjfe er designet ekstremt brede (minimum 100 mil / 2,54 mm), så de fysisk kan holde til de mange ampere i de få mikrosekunder, 
transienten varer, og tvinge en foransiddende ekstern HRC-sikring til at sprænge, hvis overbelastningen fortsætter.

##3.5 Auto-Ranging, Spændingsdeler & FET-Buffer
Formål: At dæmpe (skalere) indgangsspændinger på helt op til 1000V ned til et sikkert område mellem 0 og 4V, som ADC-chippen kan tåle, samt sikre en ekstremt høj indgangsmodstand.

Sådan virker den:

Anti-Arcing Resistor Matrix: Blokken indeholder et dæmpernetværk (spændingsdeler) på totalt 10 MΩ. I stedet for at bruge én stor 10 MΩ modstand, 
er netværket opbygget som en "daisy-chain" af 10 stk. 1 MΩ modstande i serie (SMD-1206, 0,1% tolerance). Hvis man påtrykker 1000V, 
vil der kun ligge 100V over hver modstand. Dette eliminerer risikoen for "arcing" – at strømmen danner en gnist eller lysbue hen over modstandens overflade. 
Metalfilm-typen sikrer, at modstanden ikke ændrer værdi, når den bliver varm (±25ppm/°C).

Ultra-lav-lækage relæer: Mindre Omron G6K signalrelæer (styret via Blok 9) forbinder forskellige punkter i modstandskæden til GND_ISO. 
Ved at tænde og slukke relæerne ændres delingsforholdet dynamisk (1:1 for 600mV rækkevidde, 1:10 for 6V, 1:100 for 60V osv.).

Elektrometer-Grade FET-Buffer: Når man måler svage signaler i laboratoriet (f.eks. på 600 mV eller 6 V områderne), 
vil en standard indgangsmodstand på 10 MΩ belaste testkredsløbet for meget og trække målingen skæv. 
Derfor routes signalet på de lave områder igennem en dedikeret JFET-buffer (f.eks. LSK170) eller en elektrometer-grade operationsforstærker (ADA4530). 
Denne komponent har en næsten uendelig indgangsimpedans på > 1 GΩ (en milliard ohm).

Signal splitting: Bufferens output splittes i to veje: Et DC-koblet signal (direkte spænding), der sendes til ADC'ens AIN0-indgang, 
og et AC-koblet signal (vekselspænding via en kondensator), der sendes videre til True RMS-konverteren i Blok 6.

##3.6 Modstandsmåling (2-Wire & 4-Wire Kelvin) & Hardware-Klamper
Formål: At måle elektrisk modstand med ekstrem præcision ved hjælp af en konstantstrømskilde, samt yde 100% beskyttelse mod katastrofale 
"hov-fejl" (f.eks. tilslutning af 230V i modstandstilstand).

Sådan virker den:

Konstantstrømskilde (LT1006 + LM336): En ultra-stabil 2.5V spændingsreference (LM336-2.5) fodrer en præcisions-operationsforstærker (LT1006) med ekstremt lav offset-fejl. 
Denne op-amp konfigureres til at sende en præcis, omskiftelig teststrøm (fra 100 nA op til 1 mA, afhængigt af modstandsområdet) ud af multimeterets Force+ terminal (V/Ω-bøsningen).

4-Leder Kelvin Måling: Strømmen (Force) sendes ud gennem testledningen, igennem modstanden der skal måles, og returnerer via COM (Force-). 
Når strømmen løber igennem testledningerne, opstår der et uønsket spændingsfald i selve kablerne. For at eliminere denne fejl, 
har systemet to uafhængige indgange: Sense+ og Sense-. Da der næsten ikke løber nogen strøm ind i disse "Sense"-linjer, måler de det rene spændingsfald direkte over modstanden. 
ADC'en (Blok 8) måler denne spænding differentielt over AIN2/AIN3, og Ohms lov (R = U / I) bruges til at finde modstanden.

Hardware-Klamper (Beskyttelsen): Hvis brugeren ved en fejl sætter 230V netspænding til indgangen, mens instrumentet står i Ohm-mode, 
vil strømmen søge ind i kredsløbet. Signalet tvinges først igennem en 600V CAT III rated PTC-termistor (en komponent, hvis modstand stiger voldsomt, 
når den bliver varm). Lige efter PTC'en sidder lynhurtige 1N4148 klamperdioder forbundet direkte til målesidens strømskinner (VCC_ISO (+5V) og GND_ISO). 
Dioderne klamper øjeblikkeligt fejlespændingen til sikre +5,7V eller -0,7V, hvilket beskytter de følsomme IC'er. PTC'en absorberer energien, 
varmer op i løbet af få millisekunder, og øger sin modstand så voldsomt, at den reelt afbryder strømmen.

Hardware Interrupt Kredsløb: Sideløbende overvåges dette knudepunkt af en LM393 hardware-komparator. 
Hvis spændingen overstiger 4,5V, vipper komparatorens udgang til HØJ (5V) på under 1 mikrosekund. 
Dette skyder et øjeblikkeligt alarmsignal over isolationsbarrieren (Blok 9) for at trigge ESP32'erens nødprocedure.

##3.7 True RMS Converter
Formål: At udregne den sande effektive værdi (True Root Mean Square) af komplekse, støjende eller forvrængede vekselspændings-bølgeformer (AC) rent analogt.

Sådan virker den:

Analog Beregningsmotor (AD8436): Blokken er centreret omkring sær-chippen AD8436. I modsætning til billige multimetre, der blot ensretter AC-signalet og ganger med 1,11 
(hvilket kun virker på perfekte sinusbølger), udfører denne IC den fulde matematiske RMS-formel i realtid via interne analoge transistor-kredsløb: 
Den tager indgangssignalet, beregner kvadratet (Squaring), tager gennemsnittet (Averaging) og udregner kvadratroden (Square Root). 
Den klarer en båndbredde helt op til 100 kHz og en høj Crest Faktor (forholdet mellem spidsværdi og RMS-værdi).

Integration via C_AVG: For at udføre "gennemsnits"-delen af beregningen, har chippen brug for en ekstern tidskonstant. 
Dette klares af en 3,3 µF ultra-lav-lækage kondensator (Tantal eller Film) forbundet til C_AVG benet. 
Kondensatoren glatter de hurtige analoge spændingssvingninger ud, så chippen leverer et stabilt, rent DC-signal på sit output, som er direkte proportionalt med AC RMS-værdien.

Routing til ADC: Dette færdige DC-signal (skaleret mellem 0 og 4V) sendes direkte til analog-til-digital konverterens kanal AIN1.

##3.8 16-Bit ADC (Analog-til-Digital Konvertering)
Formål: At fungere som målesidens hjerte, der omsætter de analoge DC-spændinger fra front-enden til præcise digitale 16-bit talværdier.

Sådan virker den:

Delta-Sigma Arkitektur (ADS1115): Chippen er en Texas Instruments ADS1115. 
Det er en Delta-Sigma ADC, hvilket gør den ideel til præcisionsmålinger, fordi den indbygget integrerer (midler) signalet over tid, hvilket naturligt undertrykker højfrekvent støj.

PGA Skalering: Chippens indbyggede PGA (Programmable Gain Amplifier) konfigureres via softwaren til GAIN_ONE, hvilket sætter dens fulde måleområde til præcis ±4,096V. 
Det betyder, at det maksimale 16-bit talområde udnyttes perfekt, når front-enden leverer mellem 0 og 4V.

Intern MUX (Kanal-routing): Chippen har en indbygget analog omskifter (multiplexer), der styres af ESP32'erens firmware via I2C-bussen:

Kanal AIN0: Måler DC spændinger fra FET-bufferen (Blok 4).

Kanal AIN1: Måler AC RMS spændinger fra True RMS-konverteren (Blok 6).

Kanal AIN2 / AIN3: Konfigureres som et differentialt input-par til at måle det rene spændingsfald over Kelvin-modstandslinjerne eller strømshuntene uden tab i stelbanerne.

Hertztal (100 SPS): Chippen er sat op til at spytte præcis 100 målinger ud i sekundet (100 SPS). 
Data sendes kontinuerligt ud på den lokale måleside-I2C bus, som er termineret med 10k pull-up modstande til VCC_ISO.

##3.9 Digital Isolationsbarriere
Formål: At fungere som en fuldstændig uigennemtrængelig elektrisk mur, der tillader højhastigheds-data og styresignaler at krydse mellem Zone A og Zone B uden metallisk kontakt.

Sådan virker den:

Tovejs Kapacitiv I2C-Isolator (ISO1541): For at overføre måledata fra ADC-chippen (Blok 8) til ESP32 processoren, 
løber I2C-kommunikationen igennem en ISO1541 chip. Indeni denne chip sendes data via mikroskopiske indbyggede kondensatorer i siliciummet. 
Signalerne omdannes til højfrekvente elektriske felter, der sendes hen over en intern isolationsbarriere. 
Den accepterer problemfrit, at målesiden kører på 5V logik (VCC_ISO), mens den konverterer det til 3,3V logik på processorsiden. 
Den yder op til 5000 Vrms isolation.

Lynhurtig Optokobler til Fejl (PC817 / 6N137): Når LM393 komparatoren i Blok 5 opdager en højspændingsfejl i Ohm-mode, skal signalet overføres med det samme. 
Det klares af en High-Speed optokobler. Signalet tænder en intern infrarød LED på målesiden, lyset skydes igennem et gennemsigtigt isolationslag inde i chippen, 
og rammer en fototransistor på processorsiden. Transistoren åbner og trækker øjeblikkeligt ESP32'erens GPIO 25 (PIN_FAULT_INT) til JORD på under 1 mikrosekund.

Relæ-Driver Isolation: Når ESP32'erens 3.3V GPIO-ben (GPIO 12, 13, 14, 27) vil skifte gear i auto-ranging netværket, sender de strøm ind i en række PC817 optokoblere. 
Fototransistoren på målesiden tænder så for strømmen fra den isolerede 5V skinne (fra Blok 1), hvilket trækker relæspolerne i Omron-relæerne (Blok 4). 
Processoren er derved totalt beskyttet mod den magnetiske støj (induktiv kickback), der opstår, når en relæspole slukkes.

##3.10 Strømmåling & Præcisionsshunts
Formål: At konvertere en ukendt elektrisk strøm (fra µA til 10A) om til et proportionalt spændingsfald (Burden Voltage), som multimeteret kan måle, uden selv at afbryde kredsløbet.

Sådan virker den:

Opdeling i strømveje: Blokken er opdelt i to fysiske indgangsveje for at beskytte instrumentet bedst muligt:

Lavstrøm (µA/mA): Strømmen ledes igennem to små præcisionsmodstande (såkaldte Burden-resistors): 
En 100 Ω modstand til µA-området og en 1 Ω modstand til mA-området (0,1% tolerance). 
For at beskytte disse modstande mod at brænde af ved pludselige strømstød, sidder der en elektronisk klamper (baseret på hurtige transistorer/MOSFETs og en PTC) 
parallelt over dem, som leder overskydende strøm udenom.

Højstrøm (10A rækkevidde): Strømmen sendes ind i en separat, dedikeret bøsning og flyder igennem en kraftig 0,01 Ω / 5W modstand lavet af Manganin-legering. 
Manganin er valgt, fordi det har en ultra-lav temperaturkoefficient; modstanden ændrer sig stort set ikke, selvom der løber 10A igennem den, og den udvikler varme.

HRC-Sikring (Den mekaniske livsforsikring): I serie med 10A Manganin-shunten sidder en stor, professionel 10A / 1000V HRC (High Rupture Capacity) keramisk sikring. 
Denne sikring er fyldt med sand indvendigt. Hvis der sker en massiv kortslutning (f.eks. hvis man kortslutter en trefaset industriskinne), smelter tråden øjeblikkeligt, 
og sandet smelter om til glas for at slukke den faretruende lysbue (plasma), der ellers ville få multimeteret til at eksplodere mellem hænderne på brugeren.

Differential-måling: Det lille spændingsfald, der opstår over den aktive shuntmodstand, måles differentielt (mellem to dedikerede baner) af ADS1115-ADC'en over kanalerne AIN2/AIN3 for at fjerne støj fra stelbanerne. Hvis der måles vekselstrøm (ACI), routes spændingsfaldet via et relæ over i AD8436 True RMS-chippen først.

##3.11 Kontrolenhed, Firmware (DSP) & Brugerflade
Formål: At fungere som multimeterets centrale "hjerne", der modtager de rå digitale tal, forædler dem matematisk til laboratoriepræcision, opdaterer skærmen uden flimmer og kommunikerer med computeren via SCPI.

Sådan virker den:

Hardwaren: Den er bygget op omkring et ESP32-WROOM-32 modul, som har to uafhængige 32-bit processorkerner (Core 0 og Core 1) kørende under et FreeRTOS-operativsystem. Den er forbundet til en stor 5" TFT-farveskærm via en lynhurtig SPI-bus og har en piezo-buzzer til akustiske alarmer.

Kerne 1: Den Asynkrone Metrologi-Task (Task_Metrology): Denne kerne har højeste prioritet. Den er låst i en uendelig løkke, der tjekker ADC'en præcis hver 10. ms (100 Hz).

DSP (Moving Average Filter): For at fjerne den uundgåelige 50Hz støj, der udstråles fra alle stikkontakter i bygningen, opsamler koden målingerne i en 100-samples stor ringbuffer (filterBuffer). Ved at tage gennemsnittet af præcis 100 målinger (svarende til nøjagtigt to hele bølgelængder af 50Hz), udligner støjen sig selv matematisk, hvilket giver en støjundertrykkelse (NMRR) på over 60 dB. Dette øger virtuelt opløsningen, så instrumentet opnår sine stabile 4½ cifre (60.000 counts).

Software-kalibrering: Koden henter lineære kalibreringskonstanter (Gain og Offset) ud af ESP32'erens indbyggede NVS-flashhukommelse via Preferences.h. Den beregner den endelige visningsværdi:

V_display = (V_raw * Gain) + Offset
Data pakkes i en MeasurementPacket-struct og skydes ind i en FreeRTOS-kø (dataQueue).

Kerne 0: Brugerflade & Kommunikation (Task_GUI_SCPI): Denne kerne kører uafhængigt med lavere prioritet, så skærmopdatering ikke forstyrrer målingerne.

Anti-Flicker GUI Engine: Kernen henter målepakker ud af køen. For at undgå, at skærmen blinker ulideligt, benyttes sort-overskrivningsteknikken: Først skrives den gamle måleværdi med sort tekstfarve på den sorte baggrund (hvilket visker det ud), og derefter tegnes det nye tal med grøn eller gul skrift oveni.

Wear-Leveling Datalogger: Når datalogging er aktiv, opsamler koden op til 100 målinger i et RAM-char-array (en skrivebuffer). Først når bufferen er fuld, åbnes filsystemet LittleFS, hele blokken skrives til log.csv i én samlet operation, og filen lukkes. Dette reducerer antallet af slette-/skrivecyklusser på flash-hukommelsen med en faktor 100, hvilket redder chippen fra at blive slidt op på to uger.

SCPI Parser: Kernen overvåger konstant den indbyggede USB Virtual COM-port. Hvis computeren sender en tekststreng (f.eks. MEAS:VOLT:DC?), fortolkes kommandoen øjeblikkeligt, og multimeteret svarer tilbage med den seneste måling med 5 decimalers præcision inden for under 5 ms.

##4. Måleprincipper og Signalflow
For at foretage målinger med dette multimeter skal du forstå, hvordan instrumentets interne hardwareblokke og softwarestyring (FSM og relæer) konfigureres til hver enkelt måletype. Her er den detaljerede gennemgang af, hvordan de 5 primære målefunktioner afvikles i instrumentet på komponent- og signalniveau.

##4.1 DC Spænding (DCV)
Princippet: Signalet skal dæmpes, bufferes og måles som en jævnspænding i forhold til det isolerede stjernestel (GND_ISO).

Signalvejen i hardwaren:

Signalet føres ind via V/Ω og COM bøsningerne (beskyttet af GDT og MOV i Blok 3).

Det rammer 10 MΩ modstandsmatrixen i Blok 4. Her aktiverer auto-ranging tilstandsmaskinen (Core 1) Omron-relæerne alt efter spændingens størrelse:

600 mV og 6 V: Ingen dæmpning (1:1). Signalet ledes uden om modstandskæden og direkte ind i JFET-bufferen (ADA4530). Bufferen hæver indgangsimpedansen til > 1 GΩ, så multimeteret ikke belaster testkredsløbet.

60 V, 600 V og 1000 V: Relæerne (PIN_RELAY_DIV10 eller PIN_RELAY_DIV100) tænder og forbinder modstandskæden til GND_ISO. Signalet dæmpes (1:10 eller 1:100), så det rammer det sikre område (0–4V). Indgangsimpedansen her er de faste 10 MΩ.

Det skalerede DC-signal sendes til kanal AIN0 på ADS1115 ADC'en.

Softwarens arbejde (Core 1): Firmwaren læser AIN0 100 gange i sekundet, kører værdierne igennem det 100-samples store Moving Average filter for at fjerne 50Hz netstøj, og ganger resultatet med det specifikke DCV Gain og Offset fra NVS-hukommelsen.

##4.2 AC Spænding (ACV — True RMS)
Princippet: Signalet skal afkobles fra eventuel DC-spænding, dæmpes, omdannes til et analogt DC RMS-signal og måles.

Signalvejen i hardwaren:

Signalet ledes ind i indgangen og dæmpes i 10 MΩ matrixen (Blok 4) på samme måde som ved DCV.

Når du vælger AC-mode, aktiverer firmwaren PIN_RELAY_AC_ROUTE. Dette relæ sender signalet igennem en DC-afkoblingskondensator (som fjerner enhver jævnspænding) og direkte ind i AD8436 True RMS-chippen (Blok 6).

AD8436 udfører analog kvadrering, midling (via den eksterne 3,3 µF C_AVG kondensator) og kvadratrodsberegning.

Chippen spytter et rent, lineært DC-signal ud (0–4V), som er direkte proportionalt med den sande effektive værdi (RMS) af AC-bølgeformen.

Dette DC-signal routes til kanal AIN1 på ADC'en.

Softwarens arbejde (Core 1): Firmwaren læser AIN1, filtrerer signalet for højfrekvent støj og anvender kalibreringskonstanterne for det valgte ACV-gear.

4.3 Modstandsmåling (Ohms)
Princippet: En kendt teststrøm sendes igennem modstanden, og det resulterende spændingsfald måles.

Signalvejen i hardwaren:

Firmwaren aktiverer PIN_RELAY_OHM (Blok 5).

Præcisions-strømkilden (LT1006 + LM336 reference) tænder og sender en præcis jævnstrøm ud af V/Ω (Force+) terminalen. Strømmen tilpasses efter modstandsområdet (f.eks. 1 mA til små modstande, 100 nA til megohm-området). Strømmen løber igennem modstanden og returnerer til COM (Force-).

2-leder mode: Spændingsfaldet måles direkte over de samme bøsninger. Testledningernes egenmodstand regnes med i resultatet (giver fejl ved lave modstande).

4-leder Kelvin mode: Du tilslutter to ekstra kabler til Sense+ og Sense- terminalerne direkte på modstanden. Da indgangen til Sense-linjerne har ekstrem høj impedans, løber der ingen strøm i dem, og der opstår intet spændingstab i kablerne. Det rene spændingsfald måles differentielt over ADC'ens AIN2 og AIN3.

Softwarens arbejde (Core 1): ESP32'eren beregner modstanden ved hjælp af Ohms lov:
R = U_measured / I_test

Bemærk: Hvis du ved en fejl måler på 230V i denne tilstand, klamper LM393-komparatoren indgangen på <1 µs og udløser nød-interrupte.

##4.4 DC Strøm (DCI)
Princippet: Strømmen tvinges igennem en præcisionsmodstand (shunt) med kendt lav værdi, og det differentielle spændingsfald måles.

Signalvejen i hardwaren:

Strømmen sendes ind i en af de dedikerede strømbøsninger (mA/µA eller 10A).

Lavstrøm (µA / mA): Strømmen løber igennem Blok 10's shunts: En 100 Ω modstand til µA eller en 1 Ω modstand til mA. Banen er beskyttet af en PTC og elektroniske klamper.

Højstrøm (op til 10A kontinuerligt): Strømmen tvinges igennem den massive 0,01 Ω Manganin-shunt. Banen er sikret af den store sandfyldte 10A/1000V HRC-keramiske sikring.

Det minimale spændingsfald (f.eks. 100 mV ved 10A strøm over en 0,01 Ω shunt) måles differentielt over ADC'ens AIN2 og AIN3 for at undgå, at støj i stelbanerne forvrænger målingen.

Softwarens arbejde (Core 1): Firmwaren bruger Ohms lov baglæns (I = U / R) til at omregne mikrovolt til ampere.

##4.5 AC Strøm (ACI — True RMS)
Princippet: Strømmen omdannes til en spænding via strømshuntene, og denne vekselspænding sendes igennem True RMS-konverteren.

Signalvejen i hardwaren:

Vekselstrømmen ledes ind i enten mA/µA eller 10A bøsningen og løber igennem de samme shunts som ved DCI.

Dette skaber et AC-spændingsfald over den aktive shunt (f.eks. en 0.01 Ω shunt ved 10A).

I stedet for at sende spændingsfaldet direkte til ADC'en, aktiverer firmwaren relæ-multiplexeren i Blok 10, som videresender det differentielle AC-signal over til AD8436 True RMS-chippen (Blok 6).

AD8436 konverterer AC-spændingsfaldet til et stabilt DC-signal, der repræsenterer strømmens sande effektive værdi (RMS).

Dette DC-signal læses på ADC'ens AIN1-kanal.

Softwarens arbejde (Core 1): ESP32'eren læser RMS-spændingen fra AIN1 og omregner den via kalibreringsfaktorerne til AC-ampere, som sendes til skærm og USB.

4.6 Samlet Signalflow-tabel
Måletilstand:        DC Spænding (DCV)
Indgangs-routing:    V/Ohm + COM. Relæer dæmper spændingen 1:1, 1:10 eller 1:100 via 10 Mohm daisy-chain matrix.
Konditionering:      Lave rækkevidder (<6V) routes via ADA4530 JFET-buffer (>1 Gohm indgangsimpedans).
ADC-kanal:           AIN0
Software-beregning:  Moving Average filter -> Lineær skalering (Gain * V_raw + Offset).

------------------------------------------------------------------------------------------------------------------------

Måletilstand:        AC Spænding (ACV)
Indgangs-routing:    V/Ohm + COM. Attenuering via 10 Mohm matrix. PIN_RELAY_AC_ROUTE aktiveres.
Konditionering:      Signalet ledes gennem en DC-afkoblingskondensator til AD8436 True RMS-IC'en (Analog kvadrat/midling/rod).
ADC-kanal:           AIN1
Software-beregning:  Polling af det analogt beregnede DC RMS-signal -> ACV-specifik kalibrering.

------------------------------------------------------------------------------------------------------------------------

Måletilstand:        Modstand (Ohms)
Indgangs-routing:    V/Ohm + COM. PIN_RELAY_OHM aktiveres. Teststrøm (100 nA - 1 mA) sendes via LT1006.
Konditionering:      2-leder: Spænding måles over indgang. 4-leder: Rent spændingsfald hentes via uafhængige Sense+ / Sense-.
ADC-kanal:           AIN2 / AIN3 (Differentielt)
Software-beregning:  Matematisk behandling via Ohms lov: R = U_measured / I_test.

------------------------------------------------------------------------------------------------------------------------

Måletilstand:        DC Strøm (DCI)
Indgangs-routing:    mA/uA eller 10A + COM. Strømmen tvinges gennem aktive præcisionsshunts (100 Ohm, 1 Ohm eller 0.01 Ohm Manganin).
Konditionering:      Det minimale differentielle spændingsfald over shunten routes udenom dæmpernetværket.
ADC-kanal:           AIN2 / AIN3 (Differentielt)
Software-beregning:  Omregning via Ohms lov: I = U_measured / R_shunt baseret på den valgte shunt.

------------------------------------------------------------------------------------------------------------------------

Måletilstand:        AC Strøm (ACI)
Indgangs-routing:    mA/uA eller 10A + COM. Strømmen flyder gennem de samme lav- eller højstrømsshunts.
Konditionering:      Det resulterende differentielle AC-spændingsfald over shunten routes via relæer ind i AD8436 True RMS-chippen.
ADC-kanal:           AIN1
Software-beregning:  Læsning af det konverterede DC RMS-strømsignal -> Skalering efter aktiv shuntværdi.

##5. Metrologiske Systemspecifikationer

##5.1 Overordnede Specifikationer
Base Opløsning: 4½ cifre (60.000 counts) realiseret via en 16-bit Delta-Sigma ADC kombineret med digital oversampling og decimering i softwaren.

Målehastighed (Rå dataopsamling): 100 målinger pr. sekund (100 SPS) kontinuerligt streamet fra ADC til MCU.

Målehastighed (SCPI via USB): Svarer på en forespørgsel (READ?) på under 5 ms med den senest formaterede måling.

Støjundertrykkelse (NMRR — Normal Mode Rejection Ratio): > 60 dB ved 50 Hz netstøj, opnået via et digitalt 100-samples glidende gennemsnitsfilter (Moving Average), som dækker præcis 1 sekund / 2 hele 50Hz-perioder.

##5.2 DC Spænding (DCV)
Præcision (1 år): < ±0,025% af aflæsningen + 5 counts.

Spændingsområder: 600 mV, 6 V, 60 V, 600 V, 1000 V.

Maksimal Opløsning: 10 µV (på 600 mV området).

Inputimpedans (Lave områder — 600 mV, 6 V): > 1 GΩ opnået via dedikeret JFET LSK170 / ADA4530 elektrometer-grade buffer.

Inputimpedans (Høje områder — 60 V, 600 V, 1000 V): ≥ 10 MΩ via spændingsdeler-matrix.

##5.3 AC Spænding (ACV — True RMS)
Måleprincip: Rent analog True RMS-til-DC konvertering via dedikeret AD8436 IC.

Spændingsområder: 600 mV, 6 V, 60 V, 600 V, 750 V.

Frekvensområde (Båndbredde): 20 Hz til 100 kHz.

Crest Faktor (Spidsfaktor): ≤ 3:1 ved fuldt udslag (op til 5:1 ved halvt udslag).

Indgangskarakteristik: DC-afkoblet via indgangskondensator, indgangsimpedans ≥ 10 MΩ.

##5.4 Modstandsmåling (Ohms)
Måle-topologier: Understøtter både 2-leder og 4-leder (Kelvin) måling for total eliminering af testledningernes iboende modstand.

Modstandsområder: 600 Ω, 6 kΩ, 60 kΩ, 600 kΩ, 6 MΩ, 60 MΩ.

Maksimal Opløsning: 0,01 Ω (10 mΩ) (på 600 Ω området).

Strømkilde: LT1006 OP-amp styret af en ultra-stabil LM336-2.5V reference (omskiftelig teststrøm fra 100 nA til 1 mA).

##5.5 Strømmåling (DCI & ACI)
Strømområder: 600 µA, 6 mA, 60 mA, 600 mA, 6 A, 10 A (kontinuerligt på 10A).

Måleprincip (ACI): Spændingsfaldet over shunts routes differentielt til AD8436 True RMS-chippen.

Shunt-værdier: 0,01 Ω / 5W Manganin-shunt til 10A-området (ultra-lav temperaturkoefficient), samt to uafhængige præcisionsshunts (100 Ω til µA og 1 Ω til mA, 0,1% tolerance).

##6. Softwarearkitektur og Digital Signalbehandling
Softwaren i dette multimeter er opbygget som et højtydende, asynkront indlejret (embedded) system. Det udnytter ESP32-processorens Dual-Core arkitektur under et FreeRTOS (Real-Time Operating System). For at signalbehandlingen (metrologien) aldrig skal fryse eller tabe målinger, når skærmen tegnes, eller når der skrives til "harddisken", er softwaren splittet fuldstændigt op i uafhængige softwareblokke, der kører på hver sin kerne.

##6.1 Overordnet Software-Arkitektur & Dataflow
Hele softwaren afvikles i én monolitisk fil (Multimeter_Firmware.ino) for at garantere fuldt variabeloverblik. Ved opstart (setup()) konfigureres hardware-pins, kalibreringsdata hentes fra flash-hukommelsen, og FreeRTOS-køen (dataQueue) samt de to asynkrone Tasks oprettes. Herefter efterlades standard loop() tom.

[ KATASTROFE INTERRUPT (IRAM_ATTR) ] ──► (Tvinger alle relæer i Safe State med det samme)
                                                        
       CORE 1 (Hardware-Kernen)                         CORE 0 (Bruger-Kernen)
┌──────────────────────────────────────┐        ┌──────────────────────────────────────┐
│  SOFTWAREBLOK A: METROLOGI TASK      │        │  SOFTWAREBLOK C: GUI ENGINE          │
│  - Hent rå data fra ADS1115 via I2C  │        │  - Læs dataQueue                     │
│  - 100 Hz timing (vTaskDelay)        │        │  - Anti-flicker (Sort-overskrivning) │
└──────────────────┬───────────────────┘        └──────────────────────────────────────┘
                   │                                                ▲
                   ▼                                                │
┌──────────────────────────────────────┐                            │
│  SOFTWAREBLOK B: DSP & FSM ENGINE    │        ┌───────────────────┴──────────────────┐
│  - Moving Average (Ringbuffer)       │ ─────► │  FreeRTOS DATA KØ (`dataQueue`)      │
│  - Lineær kalibrering (Gain/Offset)  │        │  (Sikker inter-core datatransport)   │
│  - Auto-ranging tilstandsmaskine     │        └──────────────────────────────────────┘
└──────────────────────────────────────┘                            ▲
                                                                    │
                                                ┌───────────────────┴──────────────────┐
                                                │  SOFTWAREBLOK D: SCPI PARSER         │
                                                │  - Overvåg USB Virtual COM Port      │
                                                │  - Besvar forespørgsler (<5 ms)      │
                                                └──────────────────────────────────────┘
                                                                    │
                                                                    ▼
                                                ┌──────────────────────────────────────┐
                                                │  SOFTWAREBLOK E: WEAR-LEVELING LOG   │
                                                │  - Saml 100 målinger i RAM-buffer    │
                                                │  - Bulk-skriv til LittleFS på én gang│
                                                └──────────────────────────────────────┘

##6.2 Core 1: Metrologi og den Digitale Signalmotor
Denne kerne håndterer den rå fysik og matematik. Den må aldrig blokeres af skærm- eller fil-operationer.

SOFTWAREBLOK A: Metrologi-Opsamling (100 Hz Indlæsning)

Funktion: Styring af analog-til-digital konverteren (ADS1115) over I2C-bussen.

Sådan virker den: Blokken initialiserer ADS1115 til et måleområde på ±4,096V (GAIN_ONE). Den kører i en ultra-præcis FreeRTOS-løkke styret af vTaskDelay(pdMS_TO_TICKS(10)), hvilket tvinger opgaven til at køre nøjagtigt 100 gange i sekundet (100 Hz). Hver 10. millisekund forespørger den ADC'ens interne multiplexer (MUX) om data på den relevante kanal (AIN0 for DC, AIN1 for AC RMS osv.).

SOFTWAREBLOK B: DSP-filtrering, Kalibrering & FSM (Tilstandsmaskine)

Funktion: Behandling af de rå tal, anvendelse af kalibreringskonstanter og styring af relæ-gear.

Sådan virker den:

DSP (Moving Average Filter): De rå tal lægges ind i en 100-samples stor ringbuffer (filterBuffer). Hver gang en ny måling kommer ind, overskrives den ældste, og der tages et gennemsnit af alle 100 rum. Da målehastigheden er 100 Hz, svarer 100 samples til nøjagtigt 1 sekund. Dette fjerner effektivt 50 Hz netstøj (NMRR-filtrering) og øger den virtuelle opløsning til et stabilt 4½-cifret output (60.000 counts).

Kalibreringsmotor: Det filtrerede tal ganges med en Gain-faktor og lægges sammen med en Offset-værdi hentet fra NVS-flashhukommelsen under opstart:
V_display = (V_raw * Gain) + Offset
Konstanterne er unikke for hvert enkelt af de 25 måleområder.

Auto-ranging Tilstandsmaskine (FSM): En enum class holder styr på det aktuelle gear (f.eks. MODE_DCV, MODE_OHM_2W). Blokken overvåger spændingen med en anti-klapre hysterese-algoritme: Hvis den filtrerede spænding rammer ≥ 98% af ADC'ens fulde udslag, skiftes der op i gear. Der skiftes først ned igen, hvis spændingen falder til ≤ 9% af det nuværende gear.

Relæ Blanking & Buffer Flush: Når FSM-en skifter gear via digitalWrite til relæerne, tvinger softwaren processoren til at vente i 50 ms (vTaskDelay), så de mekaniske kontakter kan falde til ro. Derefter slettes ringbufferen fuldstændigt og fyldes op med helt friske målinger, så relæ-gnister og støj ikke slæbes med ind i det nye gennemsnit.

Pakke-afsendelse: De færdige, kalibrerede data opsamles i en MeasurementPacket-struct og sendes ind i FreeRTOS-køen (dataQueue).

##6.3 Core 0: Brugerflade, Filsystem og SCPI Parser
Denne kerne håndterer alt det, som tager tid, og som kører asynkront i forhold til målingerne.

SOFTWAREBLOK C: Anti-Flicker GUI Engine

Funktion: Tegner grafik og tal på 5" TFT-skærmen uden ulidelig blinken.

Sådan virker den: Blokken lytter på FreeRTOS-køen. Hver gang en ny MeasurementPacket lander i køen, udpakkes værdierne. I stedet for at rydde hele skærmen med fillScreen(), som vil få skærmen til at blinke, benyttes sort-overskrivningsteknikken: Koden sætter tekstfarven til sort (TFT_BLACK) og tegner den gamle værdi (som er gemt i en variabel fra forrige kørsel) oven i sig selv. Det sletter tallet. Derefter skiftes farven til grøn eller gul, og det nye tal tegnes på nøjagtigt samme koordinat. Antallet af decimaler styres dynamisk via en switch-case baseret på det aktive gear.

SOFTWAREBLOK D: Token-baseret SCPI Parser

Funktion: Kommunikerer med LabVIEW, Python eller lignende via USB Virtual COM-porten.

Sådan virker den: For hver cyklus i bruger-tasken tjekkes Serial.available(). Hvis der ligger data, læses tekststrengen ind, indtil der registreres et linjeskift (\n). Strengen renses for mellemrum (trim()) og tvinges til store bogstaver (toUpperCase()). En string-analyzer (parser) tjekker strengen:

Ved kommandoen *IDN? sendes instrumentets officielle identifikation retur.

Ved kommandoen MEAS:VOLT:DC? eller READ? returneres den seneste formaterede måling øjeblikkeligt (under 5 ms) med 5 decimalers præcision.

Hvis en kommando indeholder en specifik rækkevidde (f.eks. CONF:VOLT:DC 60), deaktiveres auto-ranging, og FSM-en i Core 1 tvinges over i manuelt gear.

SOFTWAREBLOK E: Wear-Leveling Datalogger (LittleFS)

Funktion: Gemmer måledata på ESP32'erens interne flashhukommelse uden at slide den op.

Sådan virker den: Hvis man åbner og lukker en fil 100 gange i sekundet på en flash-chip, dør chippen på under to uger. Denne blok løser problemet ved at oprette en skrivebuffer (et char-array eller String) i RAM. Blokken opsamler målinger i RAM i præcis 1 sekund (100 målinger). Først når RAM-bufferen er fuld, kaldes LittleFS.open("/log.csv", "a"). Hele tekstblokken skrives i én samlet bulk-operation, hvorefter filen lukkes med logFile.close(). Dette reducerer sliddet på flash-hukommelsen med en faktor 100. Inden hver skrivning tjekkes logFile.size(); hvis filen overstiger 1 MB, roteres loggen, og der oprettes automatisk en ny fil (f.eks. log_002.csv).

##6.4 Nødprocedure: Det IRAM-låste Katastrofe-interrupt
Denne software-komponent ligger uden for FreeRTOS-taskernes normale hierarki og har absolut førsteprioritet.

Funktion: Reaktion på livsfarlig overbelastning ("Hov-fejl" i modstandsmåling).

Sådan virker den: Når LM393-hardwarekomparatoren skyder et 5V fejlsignal over isolationsbarrieren til GPIO 25, afbryder ESP32-hardwaren alt andet arbejde på tværs af begge kerner på under 1 mikrosekund. Den eksekverer øjeblikkeligt hardwareFaultISR(). Da denne funktion er forsynet med IRAM_ATTR, ligger dens binære kode låst i ESP32'erens lynhurtige interne RAM i stedet for den langsomme flashhukommelse, hvilket sikrer øjeblikkelig kørsel.

ISR-koden udfører to kritiske handlinger:

Den tvinger alle auto-ranging relæ-pins (PIN_RELAY_DIV10, PIN_RELAY_DIV100, PIN_RELAY_OHM) til LOW. Dette frakobler modstands-teststrømmen og dæmper indgangen maksimalt (Safe State = 1000V range).
Den sætter den globale, volatile variabel systemHalted = true.
Når Core 1 ser systemHalted == true, stopper den alle metrologiske målinger. Når Core 0 ser flaget, afbryder den det normale GUI, farver skærmen helt rød, tænder piezo-buzzeren uafbrudt og fastlåser softwaren i en uendelig while(systemHalted) løkke, som kun kan forlades via et fysisk tryk på nulstillingsknappen eller en SCPI *RST.

##7. Forståelse af 4½ Cifre med en 16-Bit ADC
For at forstå hvorfor, og hvordan instrumentet har 4½ cifre med en 16 bit biploar ADC der reelt kun er 15 bit og fortegnsbit, skal vi kigge på matematikken bag "counts" og bits, samt de metrologiske støjbegrænsninger.

##7.1 Matematikken: Bits vs. Counts
Et 4½-cifret multimeter viser op til 60.000 counts (fra 00.000 til 59.999). Hvis vi ser på, hvad en 16-bit ADC (som ADS1115) rent matematisk kan levere i binær opløsning:
2^16 = 65.536 unikke niveauer.
Da ADS1115 måler bipolært (både positive og negative spændinger), splittes de 16 bits op i 15 bits + 1 fortegnsbit.
Det giver 2^15 = 32.768 counts for den positive halvdel af skalaen.

Konklusion på matematikken: Hvis vi brugte den rå ADC-værdi direkte, ville 32.768 counts kun række til et 3½-cifret instrument (som typisk kræver 20.000 counts) og slet ikke ramme de 60.000 counts, som kravene foreskriver for dette 4½-cifrede instrument.

##7.2 Oversampling & DSP
Som beskrevet i systemarkitekturen og firmwarekravene udnytter instrumentet Digital Signal Processing (DSP) på ESP32'eren til at "trylle" højere opløsning frem:

Oversampling og Decimering: Ved kontinuerligt at aflæse ADS1115 med 100 målinger i sekundet (100 SPS) og køre dem igennem et Moving Average Filter (glidende gennemsnit), øges den effektive opløsning (ENOB - Effective Number of Bits).

Hvid støj som hjælper: Hvis der er en smule tilfældig støj til stede (hvilket der altid er i analoge kredsløb), vil gennemsnittet af mange målinger placere sig mellem de normale 16-bit niveauer. Hver gang vi firdobler antallet af samples, vinder vi matematisk 1 ekstra bit i opløsning.

Det er denne software-algoritme, der gør, at instrumentet virtuelt øger opløsningen og opnår et stabilt 4½-cifret output.

##7.3 Begrænsninger for Højere Opløsning
Hvis du ville opgradere instrumentet til f.eks. 5½ eller 6½ cifre, er det ikke kun et spørgsmål om at skifte til en 24-bit ADC. Den virkelige begrænsning i dette specifikke design skyldes de analoge hardware-komponenter omkring ADC'en:

Komponent / Faktor	Nuværende begrænsning i designet (4½ cifre)	Krav til højere opløsning (f.eks. 5½ - 6½ cifre)
Spændingsreference	LM336-2.5V. Den har for meget temperaturdrift til flere cifre.	Kræver en ultra-stabil, ovn-styret reference (f.eks. LTZ1000 eller LM399).
Modstandsnetværk	SMD-1206 modstande med 0,1% tolerance og ±25ppm/°C.	Kræver hermetisk lukkede, matchede modstandsnetværk (< ±1ppm/°C) for at undgå termisk drift.
Termoelektrisk støj	Almindelige kobberbaner og komponentlodninger skaber mikro-volt (Seebeck-effekt), når printet bliver varmt.	Kræver avanceret termisk isolering, "shielding" og måske mekaniske relæer med ultra-lav termisk EMF.
Linearitet			ADS1115 har en indbygget ulinearitet (INL), som softwaren ikke nemt kan rette op på.	Kræver en dedikeret metrologi-ADC (f.eks. en 24-bit Multi-Slope eller high-end Delta-Sigma ADC).

Kort sagt: Det er samspillet mellem 16-bit ADC'ens rå grænse og den analoge hardwarestøj/temperaturdrift, der sætter grænsen. 16-bit ADC'en presses til sit absolut yderste (4½ cifre) ved hjælp af ESP32'erens software-filtrering, men analog-kredsløbets generelle stabilitet tillader ikke, at man blot tilføjer flere decimaler på skærmen uden at de sidste cifre vil flintre ustabilt rundt.

##8. SCPI-Kommunikationsgrænseflade
At styre dette multimeter fra en pc foregår via den standardiserede SCPI-protokol (Standard Commands for Programmable Instruments). Instrumentet efterligner opbygningen af professionelle laboratoriemultimetre og bruger ESP32'ens indbyggede USB-port til at emulere en USB-CDC Virtual COM Port. Her er den detaljerede gennemgang af kommunikationskravene, protokollen, kommandostrukturen, og hvordan pc'en opsamler data.

##8.1 Fysisk Forbindelse og Protokol
Når multimeteret sluttes til pc'en med et USB-kabel, genkendes det automatisk som en seriel COM-port (f.eks. COM3 på Windows eller /dev/ttyACM0 på Linux). Forbindelsen på pc-siden skal initialiseres med følgende parametre:

Baudrate: 115200 baud.

Databits: 8 databits.

Paritet: Ingen (None).

Stopbits: 1 stopbit.

Flow Control: Ingen (None) — systemet benytter USB-CDC's indbyggede hardwareregulering.

Linje-terminering (Kritisk!): Alle kommandoer sendt fra pc'en skal afsluttes med et Line Feed-tegn (\n eller LF). Multimeteret svarer altid pc'en med et efterfølgende \n.

##8.2 SCPI-Parserens Interne Logik
I multimeterets firmware kører en token-baseret string-analyzer asynkront på Core 0 (Bruger-kernen). Når et \n-tegn detekteres, udfører softwaren følgende trin:

trim(): Fjerner eventuelle foranstillede eller efterstillede usynlige tegn (mellemrum, Carriage Return \r).

toUpperCase(): Konverterer hele strengen til store bogstaver. Det betyder, at SCPI-parseren er ligeglad med, om pc'en sender meas:volt:dc? eller MEAS:VOLT:DC?.

Token Splitting: Kommandoen deles op ved kolon (:) og mellemrum for at identificere kommando-træet og eventuelle parametre.

##8.3 Understøttet SCPI-Kommandosæt
Kommandoerne følger IEEE 488.2 / SCPI-1999 standarden og er opdelt i tre funktionelle grupper:

A. System- og Standardkommandoer (IEEE 488.2)

*IDN? (Identification Query): PC'en spørger: "Hvem er du?".

Multimeterets svar: "DMM-MASTER,16SRMS,SN1,V6.0\n".

*RST (Reset): Tvinger multimeteret ud af fejltilstande, nulstiller auto-ranging tilstandsmaskinen (FSM) på Core 1 og sætter instrumentet i det mest sikre udgangspunkt: MODE_DCV (1000V range).

B. Instrumentkonfiguration og Måling (CONF og READ)

CONF:VOLT:DC [<range>]: Skifter instrumentet til DC spændingsmåling. Hvis en parameterværdi udelades, aktiveres auto-ranging. Hvis en specifik rækkevidde angives (f.eks. CONF:VOLT:DC 60), deaktiveres auto-ranging, og FSM'en låses i det specifikke hardwaregear.

CONF:RES [<range>]: Skifter til 2-leder modstandsmåling (Ohm).

CONF:FRES [<range>]: Skifter til 4-leder (Kelvin) modstandsmåling.

READ? eller MEAS:VOLT:DC?: Beder multimeteret om omgående at returnere den seneste færdigbehandlede måling fra FreeRTOS-køen.

Multimeterets svar: Værdien sendes som en ASCII-streng med 5 decimalers præcision (f.eks. "5.02341\n"). Svartiden er under 5 ms.

C. Fil- og Hukommelseshåndtering (MMEM — Mass Memory)

Da multimeteret logger data lokalt i sit LittleFS-filsystem, kan pc'en administrere logfilerne eksternt:

MMEM:CAT?: Returnerer en liste over alle filer samt deres præcise filstørrelse i bytes på LittleFS-partitionen.

MMEM:DATA? "log.csv": Dumper hele indholdet af den specifikke logfil råt over USB-forbindelsen i CSV-format, så pc'en kan gemme det direkte som en .csv eller Excel-fil.

MMEM:DEL "log.csv": Sletter logfilen fra ESP32'ens flashhukommelse for at frigøre plads.

##8.4 Maksimering af Målehastighed: SYST:REM
Hvis pc'en skal hente rå måledata ud med instrumentets maksimale hastighed (100 Hz streams), er skærmopdateringen på 5" TFT-displayet en flaskehals for Core 0.

SYST:REM (System Remote): Når pc'en sender denne kommando, skifter instrumentet til "Remote Mode". Softwaren deaktiverer (valgfrit) grafikmotoren og opdateringen af TFT-skærmen.

Formål: Dette frigør kritiske CPU-cyklusser på Core 0. Nu kan Core 0 udelukkende fokusere på at tømme dataQueue og spytte målinger ud over USB-porten i samme takt, som Core 1 måler dem (100 Hz), uden at tabe pakker.

8.5 Datakrav og Streaming-format på PC-Siden
Når du programmerer et script på pc'en (f.eks. i Python eller LabVIEW) til at hente data automatisk, skal scriptet overholde følgende logiske krav:

Buffer-størrelse: Da data overføres som ASCII-tekststrenge, skal pc'ens serielle modtagebuffer være konfigureret til at læse linjebaseret (f.eks. readline() i Python). En typisk datalinje fra en lokal log-dump (MMEM:DATA?) eller en live-strøm har formatet: Timestamp, Måletype, Værdi\n.

Eksempel på en live-datapakke modtaget af pc'en:

142530,DCV,4.99823
(Hvor 142530 er antal millisekunder siden apparatet blev tændt, DCV er målefunktionen, og 4.99823 er den kalibrerede spænding).

Håndtering af Overload (isOverload): Hvis multimeteret måler en spænding, der overstiger det maksimale for det valgte gear (eller over 1000V), ændrer firmwaren ASCII-svaret til en standard SCPI-fejlstreng eller den specifikke værdi "9.9E+37\n" (standard IEEE-format for Infinity/Overload). Pc-scriptet skal kunne fange denne specifikke streng, så det ikke crasher under forsøget på at konvertere teksten til et flydende kommatal (float).

##9. Betjeningsvejledning

##9.1 Kend Dit Apparat (Det Visuelle Overblik)
Før vi tænder for maskinen, skal vi kigge på den store, flotte 5-tommers farveskærm på forsiden og de 4 runde huller (bøsninger), hvor testledningerne skal stikkes ind.

###1.1 Skærmens tre zoner (Hvad betyder billedet?)

Skærmen er opdelt i tre smarte zoner, så du hurtigt kan se, hvad der foregår:

Zone 1 (Øverste højre hjørne): Status-feltet. Her kan du se et lille USB-stik, hvis maskinen taler med en computer, eller et diskette-ikon, hvis den gemmer dine målinger i sin indbyggede harddisk. Pas på: Hvis du ser en rød advarselstrekant her, betyder det, at spændingen er over 30 Volt — så må du KUN røre ved plastikken på ledningerne!

Zone 2 (Lige i midten): Det primære display. Her står de GIGANTISKE grønne eller gule tal, som er selve måleresultatet. Der er altid 4 tal efter kommaet (f.eks. 01.5432), fordi maskinen er ultra-præcis.

Zone 3 (Helt nede i bunden): Dual-displayet. Her kan maskinen vise ekstra hemmeligheder, som f.eks. hvor meget elektrisk "støj" (rippel), der er i det batteri, du måler på.

###1.2 De 4 magiske huller (Hvor skal ledningerne sidde?)

Du har fået to testledninger med: En SORT (som altid betyder "Jord" eller "Minus") og en RØD (som betyder "Plus" eller "Signal"). Kig godt på de fire huller på fronten:

COM (Det sorte hul): Dette er basen. Den SORTE ledning skal ALTID sidde her, uanset hvad du måler!

V/Ω (Det røde standard-hul): Her skal den RØDE ledning sidde 95 % af tiden. Det bruges, når du vil måle Volt (spænding) og Ohm (modstand).

mA/µA (Det røde svagstrøms-hul): Her flytter du den RØDE ledning hen, hvis du skal måle meget, meget små mængder strøm (f.eks. i et lille blinkende LED-lys).

10A (Det røde super-kraft-hul): Her flytter du den RØDE ledning hen, hvis du skal måle gigantisk store strømme (f.eks. til en kraftig elektrisk motor). Dette hul er beskyttet af en tyk boks med sand indeni, så maskinen ikke kan eksplodere.

##9.2 Sikkerhedsregler for Brugeren
Elektricitet er usynligt, og det kan være farligt, hvis man ikke passer på. Men hvis du følger disse 3 gyldne regler, kan der ALDRIG ske noget:

###Regel nr. 1: Hold kun på plastikken!
Yderst på de to ledninger sidder der to spidse metalpinde. Når du måler, må dine fingre ALDRIG røre ved metallet. Hold altid dine fingre bag de små plastik-kraver på håndtaget.

###Regel nr. 2: Lad stikkontakten være i fred!
Dette multimeter kan godt holde til at måle på de 230 Volt, der kommer ud af stikkontakten i væggen — men DET MÅ DU IKKE GØRE ALENE. Strømmen i væggen er livsfarlig for mennesker. Øv dig altid på batterier (1,5V, 9V) eller små legetøjs-kredsløb først.

###Regel nr. 3: Den sorte ledning først ind, og sidst ud!
Når du skal måle på noget, så sæt altid den sorte pind på først, og derefter den røde. Når du er færdig, tager du den røde pind væk først, og derefter den sorte.

##9.3 Trin-for-Trin Operationsguide
Multimeteret har en Auto-Range hjerne. Det betyder, at du ikke behøver gætte på, hvor stort et batteri du måler på — maskinen finder selv ud af det og skifter automatisk gear! Her er opskriften på de tre mest populære målinger:

##9.3.1 Sådan måler du spænding på et batteri (Volt / DCV)

Hvad gør det? Finder ud af, om der er mere "juice" tilbage på dit legetøjs-batteri.

Tænd multimeteret på knappen.

Sæt den SORTE ledning i COM-hullet.

Sæt den RØDE ledning i V/Ω-hullet.

Tryk på knappen på skærmen, hvor der står DCV (eller send kommandoen CONF:VOLT:DC fra din computer).

Tag det batteri, du vil tjekke. Pres den SORTE metalspids mod batteriets flade bund (Minus / -).

Pres den RØDE metalspids mod batteriets lille knap i toppen (Plus / +).

Kig på skærmen: Hvis der står f.eks. 01.5240 V, er dit batteri helt frisk og klar til leg!

##9.3.2 Sådan tjekker du, om en ledning er knækket (Modstand / Ohm)

Hvad gør det? Finder ud af, om strømmen kan løbe frit igennem en ledning eller en kontakt.

Sørg for, at den ledning eller det legetøj, du vil måle på, er helt slukket og ikke har noget batteri i sig.

Sæt ledningerne i maskinen: Sort i COM og Rød i V/Ω.

Tryk på knappen på skærmen, hvor der står OHM (eller brug computeren: CONF:RES).

Sæt den SORTE pind i den ene ende af ledningen, og den RØDE pind i den anden ende.

Kig på tallene midt på skærmen:

Hvis tallene suser helt ned tæt på nul (f.eks. 00.02 Ω), betyder det, at ledningen er helt hel og fin indeni. Strømmen suser lige igennem!

Hvis der står 9.9E+37 eller skærmen skriver OVERLOAD, betyder det, at ledningen er knækket indeni, eller at kontakten er afbrudt. Strømmen kan ikke komme igennem.

##9.3.3 Sådan måler du strømstyrke (Ampere / DCI)

Hvad gør det? Måler, hvor mange "elektriske arbejdere" der løber igennem ledningen i sekundet.

Sæt den SORTE ledning i COM.

STOP OP OG TÆNK: Er det en lille bitte pære (mA), eller er det en stor motor (10A)? Hvis du er i tvivl, så sæt ALTID den RØDE ledning i det store 10A-hul for en sikkerheds skyld!

Tryk på skærmens DCI-knap.

VIGTIGT: Strømmåling skal gøres som en "bro". Du skal klippe eller afbryde dit kredsløb, og lade multimeteret være broen, som strømmen skal løbe igennem. Strømmen skal løbe ind i den røde ledning, igennem maskinen, og ud af den sorte ledning.

Tænd for dit legetøj, og se på de store tal på skærmen, som nu slutter med et A (Ampere).

##9.4 Fejlalarm og Nulstilling
Dette multimeter har en super-hjerne. Hvis du laver en fejl — f.eks. hvis du har glemt, at maskinen står i OHM-mode, og du ved et uheld stikker ledningerne direkte ind i stikkontakten — så sker der følgende:

text
[ DU LAVER EN FEJL ] ──► Maskinen opdager det på under 1 mikrosekund!
                                 │
                                 ▼
[ KATASTROFE-ALARM ] ──► Skærmen bliver BLODRØD! 
                         Buzzeren hyler: BIIIIIIIP!
                                 │
                                 ▼
[ SYSTEMET LÅSER ]   ──► Maskinen nægter at måle videre, indtil det er sikkert.
Seks trin til at redde situationen:

Bevar roen! Maskinen har allerede slukket for de farlige dele indeni helt af sig selv, så du er fuldstændig i sikkerhed.

Fjern ledningerne med det samme! Slip det, du var ved at måle på.

Kig på skærmen. Den er nu blodrød, og der står en blinkende besked med store hvide bogstaver: HIGH VOLTAGE OVERLOAD - REMOVE LEADS

Den lille indbyggede højttaler vil hyle uafbrudt: BIIIIIIIIIIIP!

Maskinen har nu låst sig selv fast. Den vil overhovedet ikke måle noget som helst, selvom du sætter ledningerne på et harmløst batteri bagefter. Dette er for at passe på dig.

Sådan nulstiller du maskinen: Når du har fjernet ledningerne og fundet ud af, hvad fejlen var, skal du enten trykke på den fysiske knap på multimeterets kabinet, eller få din computer til at sende SCPI-kommandoen *RST. Så bliver skærmen fin og sort igen, hyletonen stopper, og multimeteret starter forfra i det sikre DCV (Volt) gear. Du er klar til at forske videre!

##10. Mekaniske Konstruktionskrav
Kabinetmateriale: Brandhæmmende plast godkendt til UL94 V-0 rating.

Sikringsbeskyttelse (10A): Intern 10A / 1000V HRC (High Rupture Capacity) keramisk højeffektsikring fyldt med sand (f.eks. Bussmann DMM-B-11A).

Frontpanel layout: 4 mm sikkerhedsbøsninger monteret med en standard 19 mm (0.75") center-til-center afstand for kompatibilitet med dual-bananstik.

##11. Kommerciel Analyse og Positionering

##11.1 Markedssammenligning
Elektronikbranchen domineres af lukkede platforme. High-end producenter som Fluke Calibration (8845A/8846A), Keysight Technologies (34461A) og Keithley (DMM6500) leverer ekstrem metrologisk autoritet (6½ til 7½ cifre) og ultra-lav langtidsdrift baseret på ovnstyrede spændingsreferencer (f.eks. LM399 eller LTZ1000). Mellemklasse-producenter som Rigol (DM3058E) og Siglent (SDM3045X) tilbyder solide 4½ til 5½-cifrede laboratorie-bænkmultimetre, men de afvikles alle på låst, proprietær software eller embedded Linux, hvor kildekoden er utilgængelig.

Dette ESP32-multimeter forsøger ikke at konkurrere med Fluke eller Keysight på ren rå præcision; dertil er en 16-bit ADS1115 ADC og en standard LM336 reference for begrænsede. Instrumentets revolutionerende styrke ligger i dets agilitet og software-åbenhed. Det tilbyder fuld, open-source C++/Arduino-modificerbarhed, indbygget wear-leveling logning i RAM, en lynhurtig token-baseret SCPI-parser, samt muligheden for at integrere trådløse IoT-protokoller (Wi-Fi/Bluetooth) direkte på controlleren — funktioner som er fundamentalt fraværende eller dyre tilføjelser hos de 15 mest kendte mærker på markedet.

##11.2 Komparativ Analyse: ESP32-Multimeteret vs. De 15 Største Mærker
Mærke / Producent:    	Fluke Calibration
Typisk Modelklasse:   	8845A / 8846A
Opløsning (Cifre):    	6 1/2 cifre
Firmware / Arkitekt:  	Lukket / Proprietær metrologi
Fordele:              	Ekstrem langtidsstabilitet, sporbar kalibrering, ultra-lav støj.
ESP32 Differentiering: 	ESP32-modellen er modificerbar, koster en brøkdel, og har indbygget trådløs konnektivitet via MCU'en.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Keysight Technologies
Typisk Modelklasse:   	34461A / 34465A
Opløsning (Cifre):    	6 1/2 til 7 1/2
Firmware / Arkitekt:  	Lukket / PathWave / BenchVue
Fordele:              	Industri-standard, fabelagtig grafisk trend-visning, ekstrem præcision.
ESP32 Differentiering: 	Keysight tillader ingen kildekode-modifikation. ESP32-modellen tillader speciallavede firmware-plugins på Core 0.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Keithley (Tektronix)
Typisk Modelklasse:   	DMM6500
Opløsning (Cifre):    	6 1/2 cifre
Firmware / Arkitekt:  	Lukket / TSP (Test Script)
Fordele:              	Indbygget digitizer, avanceret berøringsskærm, avanceret Matrix-scanning.
ESP32 Differentiering: 	Keithley bruger et unikt, men lukket scriptsprog. ESP32 anvender standard C++/Arduino, som alle kender.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Rigol
Typisk Modelklasse:   	DM3058E / DM3068
Opløsning (Cifre):    	5 1/2 til 6 1/2
Firmware / Arkitekt:  	Lukket / Embedded Linux
Fordele:              	Meget billig 5 1/2-cifret ydeevne, solid SCPI-understøttelse.
ESP32 Differentiering: 	Rigols filsystem er lukket. ESP32-modellen giver direkte adgang til LittleFS-hukommelsen via SCPI (MMEM:DATA?).

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Siglent
Typisk Modelklasse:   	SDM3045X / SDM3055
Opløsning (Cifre):    	4 1/2 til 5 1/2
Firmware / Arkitekt:  	Lukket / Linux platform
Fordele:              	Flot farveskærm, hurtig digital opsamling, histogramsider.
ESP32 Differentiering: 	Siglents datalogger slider på intern hukommelse, hvis den ikke konfigureres rigtigt. ESP32-modellen har indbygget wear-leveling buffer i RAM.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Rohde & Schwarz
Typisk Modelklasse:   	HMC8012
Opløsning (Cifre):    	5 1/2 cifre
Firmware / Arkitekt:  	Lukket / LXI-kompatibel
Fordele:              	Ultra-lavt energiforbrug, ekstremt støjsvagt design, tysk kvalitet.
ESP32 Differentiering: 	HMC8012 is dyrt og statisk. ESP32-multimeteret har et asynkront dual-core design, hvor GUI og metrologi er 100% separerede.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	GW Instek
Typisk Modelklasse:   	GDM-8341 / 8342
Opløsning (Cifre):    	4 1/2 til 5 1/2
Firmware / Arkitekt:  	Lukket / MCU-styret
Fordele:              	Prisbilligt, dual-measurement display (f.eks. DCV + frekvens).
ESP32 Differentiering: 	GW Instek har begrænset lagerplads. ESP32-multimeteret har 2MB dedikeret SPIFFS/LittleFS til lokal lagring av gigantiske CSV-filer.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	B&K Precision
Typisk Modelklasse:   	5491B / 5492BG
Opløsning (Cifre):    	4 1/2 til 5 1/2
Firmware / Arkitekt:  	Lukket / Aeldre platform
Fordele:              	Robust industrielt design, velfungerende i produktionstests.
ESP32 Differentiering: 	B&K-modellerne mangler moderne IoT-integration. ESP32-instrumentet kan opgraderes til OTA (Over-The-Air) firmwareopdatering.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Hioki
Typisk Modelklasse:   	DT4282 (Baenk-klasse)
Opløsning (Cifre):    	4 1/2 cifre
Firmware / Arkitekt:  	Lukket / Japansk metrologi
Fordele:              	Ekstremt hurtig analog bargraph, uovertruffen robusthed i felten.
ESP32 Differentiering: 	Hioki tillader ikke integration af brugerdefinerede algoritmer. ESP32 har justerbart Moving Average filter (1-500 samples) i koden.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Chroma ATE
Typisk Modelklasse:   	12061
Opløsning (Cifre):    	6 1/2 cifre
Firmware / Arkitekt:  	Lukket / Test & Automation
Fordele:              	Optimeret til lynhurtige automatiserede produktionslinjer (ATE).
ESP32 Differentiering: 	Chromas softwareloesninger er dyre og proprietære. ESP32 leverer SCPI-svar på under 5 ms via open-source parseren.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Yokogawa
Typisk Modelklasse:   	DM7560
Opløsning (Cifre):    	6 1/2 cifre
Firmware / Arkitekt:  	Lukket / High-end R&D
Fordele:              	Ekstremt hoej tidssynkronisering, dyb hukommelse, metrologisk autoritet.
ESP32 Differentiering: 	Yokogawa er et rent laboratorie-instrument. ESP32-modellen fungerer som en open-source hardware co-design platform.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Anritsu
Typisk Modelklasse:   	Specifikke system-DMMs
Opløsning (Cifre):    	System-DMM
Firmware / Arkitekt:  	Lukket / Tele-fokuseret
Fordele:              	Optimeret til RF- og telekommunikations-testbaenke.
ESP32 Differentiering: 	Anritsu-instrumenter er ikke beregnet til generel R&D-modifikation. ESP32-modellen er universel og agil.

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Uni-Trend (UNI-T)
Typisk Modelklasse:   	UT8803E / UT8805E
Opløsning (Cifre):    	4 1/2 til 5 1/2
Firmware / Arkitekt:  	Lukket / Lavpris
Fordele:              	Det billigste baenkmultimeter på markedet med stor farveskærm.
ESP32 Differentiering: 	UNI-T sparer ofte på isolationen til pc'en. ESP32-multimeteret har en kompromisløs hardware-komparator (LM393) til "hov-fejl" (<1 us).

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Voltcraft (Conrad)
Typisk Modelklasse:   	VC-7055BT / VC-7200BT
Opløsning (Cifre):    	4 1/2 til 5 1/2
Firmware / Arkitekt:  	Lukket / Bluetooth-link
Fordele:              	Billigt, bredt tilgængeligt i Europa, basal datalogging via app.
ESP32 Differentiering: 	Voltcrafts apps er lukkede og fejlbehæftede. ESP32'erens SCPI-grænseflade er 100% kompatibel med standard Python (pyserial).

------------------------------------------------------------------------------------------------------------------------

Mærke / Producent:    	Tonghui
Typisk Modelklasse:   	TH1951 / TH1961
Opløsning (Cifre):    	5 1/2 til 6 1/2
Firmware / Arkitekt:  	Lukket / Kinesisk OEM
Fordele:              	Massiv udbredelse i asiatisk produktion, lav pris pr. ciffer.
ESP32 Differentiering: 	Tonghuis dokumentation er ofte mangelfuld. ESP32-modellen er fuldt dokumenteret ned til mindste kodelinje (Nul-forudsætnings princip).

##11.3 Produktionsøkonomi og Salgspris
Ved at outsource mønsterkortsproduktion og maskinel komponentmontage (PCBA) til lavpris-leverandører i Kina (f.eks. JLCPCB eller PCBWay) og indkøbe kabinettet i brandhæmmende plast (UL94 V-0) hos den billigste udenlandske leverandør, kan omkostningerne minimeres drastisk ved en serie på 1.000 enheder:

Rå Produktionspris (COGS):

* Elektronik & Chips (ESP32, ADS1115, AD8436, ISO1541, LM393, LT1006): 65,00 kr.
* Passive komponenter, relæer, 10A HRC-sikring, varistor: 45,00 kr.
* Strømforsyning (Printmonteret 10VA lineær transformer, regulatorer): 40,00 kr.
* Skærm (5" TFT farvedisplay med SPI/Parallel interface): 75,00 kr.
* PCB, SMT/THT-montage, stencil og håndtering i Kina: 35,00 kr.
* Mekanisk apparatkasse (Sprøjtestøbt plast med interne barrierer): 30,00 kr.
* Frontpanel og 4 mm CAT III sikkerhedsbøsninger: 20,00 kr.

Samlet rå produktionspris (COGS) pr. stk.: 310,00 kr.

--------------------------------------------------------------------------------

Kapacitetsomkostninger og Overhead (Pr. enhed ved 1.000 stk.):

* Markedsføring og reklame (Målrettet Google Ads, SEO, tech-influencere): 40,00 kr.
* Lokaler, lagerleje, logistik, importfortoldning og fragt: 25,00 kr.
* Administration, webshop (Shopify), Stripe-gebyrer, WEEE-skrotafgift: 20,00 kr.

Samlede reelle omkostninger pr. enhed: 395,00 kr.

--------------------------------------------------------------------------------

For at sikre en sund virksomhedsdrift, der kan dække utilsigtede reklamationer jf. købeloven, pålægges en nettoavance på 40 % af salgsprisen eksklusive moms. 

Salgsprisen beregnes således:

Salgspris ekskl. moms = Samlede omkostninger / (1 - Avance)
                      = 395,00 kr. / (1 - 0,40) 
                      = 658,33 kr.

--------------------------------------------------------------------------------

Med tillæg af den lovpligtige danske merværdiafgift (25 % moms) lander regnestykket på:

* Netto salgspris ekskl. moms: 658,33 kr.
* Fortjeneste pr. styk (Rent overskud til virksomheden): 263,33 kr.
* Dansk moms (25 %): 164,58 kr.

Endelig minimumssalgspris inkl. moms: 822,91 kr. (Kommercielt afrundet til 825,00 kr.)

--------------------------------------------------------------------------------

Med een endelig udsalgspris på ca. 825 kr. inkl. moms underbyder dette instrument markedets billigste, lukkede 4 1/2-cifrede bænkmultimetre (som typisk starter ved 1.200-1.600 kr.) med næsten 50 %. Kombinationen af en aggressiv pris, kompromisløs galvanisk hardwaresikkerhed og et 100 % modificerbart open-source software-fundament gør dette multimeter til en yderst attraktiv og disruptiv platform for ingeniører, uddannelsesinstitutioner og hardware-udviklere over hele verden.

##12. Konklusion
ESP32-multimeteret forsøger ikke at slå Fluke eller Keysight på ren rå præcision (da instrumentet er begrænset af en 16-bit ADS1115 ADC og en standard LM336 reference). Det placerer sig i stedet som en revolutionerende åben platform. Hvor de 15 etablerede mærker leverer låste instrumenter med faste funktioner, tilbyder ESP32-multimeteret det stærkeste fundament for ingeniøren, underviseren eller metrologi-entusiasten, der ønsker fuld kontrol over deres instrument, dets sikkerhedsmekanismer, og måden data flyder på.

DMM-MASTER 16SRMS — Teknisk Manual
Version 6.0