// =========================================================================
// User_Setup.h — TFT_eSPI konfiguration
// Display: 5.0" TFT LCD, 800x480, RA8875 controller, SPI interface
// MCU: ESP32 Dev Module
// Filplacering: Arduino/libraries/TFT_eSPI/User_Setup.h
// (erstat den originale fil, eller opret en ny User_Setup.h i projektmappen
// og inkluder den via #include).
//
// -------------------------------------------------------------------------
// RETTELSE (buglist F + analyse): Den oprindelige fil definerede
// SSD1963_DRIVER med SPI, men TFT_eSPI har INGEN SSD1963 SPI-driver.
// Biblioteket understøtter kun SSD1963 i 8-bit/16-bit parallel-tilstand
// (SSD1963_8DRIVER / SSD1963_DRIVER med TFT_PARALLEL_*_BIT). SSD1963's
// datablad specificerer heller ikke en standardiseret SPI-grænseflade til
// pixel-data (kun en hjælpe-SPI til register-adgang på nogle moduler).
//
// KONSEKVENS: Den valgte display-controller SKAL være en, som TFT_eSPI
// faktisk understøtter i SPI-mode ved 800×480. Den eneste reelle mulighed
// i TFT_eSPI er RA8875. Det betyder, at det fysiske display-modul skal
// være et RA8875-baseret 5" 800×480 SPI-modul (f.eks. Buydisplay
// ER-TFTM050-3 med RA8875), IKKE et SSD1963-modul.
//
// firmware_README.md og HW-DOC skal opdateres tilsvarende, så de ikke
// længere refererer til SSD1963 eller 8-bit parallel.
//
// ARKITEKTUR — HVORFOR SPI OG IKKE 8-BIT PARALLEL:
// ESP32'en har ikke 12 frie GPIOs til 8-bit parallel bus, når relæer
// (GPIO 12, 13, 14, 27), interrupt (GPIO 25) og I2C (GPIO 21, 22) er
// reserveret af firmwaren. SPI kræver kun 6 GPIOs og er derfor det eneste
// farbare valg. RA8875 understøtter SPI op til ~20 MHz stabilt.
//
// RETTELSE (buglist F): SPI-frekvensen er sat til 20 MHz (ikke 27 MHz),
// hvilket matcher firmware-headerens "~20 MHz stabil drift" og RA8875's
// anbefalede maksimum for lange PCB-baner.
// -------------------------------------------------------------------------
// =========================================================================

// -- 0. Unik setup-ID (anbefalet af TFT_eSPI for at undgå konflikt) --
#define USER_SETUP_ID 1001       // Unik ID, så TFT_eSPI kan skelne denne setup fra bibliotekets default.

// -- 1. Vælg driver --
// RETTELSE (buglist F): RA8875 er den eneste 800×480 SPI-controller, som
// TFT_eSPI faktisk understøtter. SSD1963_DRIVER er fjernet, fordi den ikke
// findes i TFT_eSPI's SPI-driver-sæt.
#define RA8875_DRIVER            // Aktiver RA8875 SPI driver (800×480, understøttet af TFT_eSPI).

// -- 2. Skærmopløsning --
#define TFT_WIDTH  800           // Fysisk bredde 800 pixels.
#define TFT_HEIGHT 480           // Fysisk højde 480 pixels.

// -- 3. SPI-tilstand --
// TFT_eSPI bruger standard SPI-interfacet (MOSI, SCLK, CS, DC).
// Ingen TFT_PARALLEL_*_BIT-makro skal defineres her.

// -- 4. Kontrolpins (SPI) --
// Alle pins er valgt fra sikre GPIOs, der ikke er strapping-pins,
// ikke er flash-pins, og ikke bruges af firmwaren.
#define TFT_CS   33              // Chip select — sikker output, ingen strapping-krav.
#define TFT_DC   32              // Data/Command (RS) — sikker output, ingen strapping-krav.
#define TFT_RST  23              // Reset — sikker output; GPIO 23 er fri.
#define TFT_MOSI 4               // SPI MOSI (Master Out Slave In) — sikker output.
#define TFT_SCLK 5               // SPI SCLK (Serial Clock) — sikker output.
// RETTELSE (buglist F): TFT_MISO er UDELADT helt i stedet for at sætte -1.
// RA8875 er write-only for pixel-data, og TFT_eSPI's håndtering af -1 er
// ikke konsistent på tværs af versioner. Ved at udelade makroen bruger
// TFT_eSPI sin default (ingen MISO).

// -- 5. SPI-frekvens --
// RETTELSE (buglist F): 20 MHz (ikke 27 MHz) matcher firmware-headerens
// "~20 MHz stabil drift" og RA8875's anbefalede maksimum for lange baner.
// 20 MHz giver ~25 Hz skærmopdatering ved 800×480, hvilket er tilstrækkeligt
// for anti-flicker GUI'en.
#define SPI_FREQUENCY  20000000  // 20 MHz SPI-klokfrekvens.

// -- 6. Backlight (valgfrit) --
// Hvis backlight styres via GPIO, definer følgende; ellers udelad.
// BEMÆRK: GPIO 22 er I2C SCL og må IKKE bruges til backlight. Vælg en fri
// GPIO (f.eks. GPIO 15 eller GPIO 2), hvis backlight skal styres.
// #define TFT_BL   15           // Backlight PWM pin — vælg en fri GPIO, ikke 21/22.
// #define TFT_BACKLIGHT_ON HIGH // Polaritet for backlight-aktivering.

// -- 7. Farvedybde --
// RETTELSE (buglist F): TFT_RGB_ORDER er pakket i #ifndef for at undgå
// warning/fejl, hvis TFT_eSPI's version bruger en anden makro eller allerede
// har defineret den. Juster til TFT_BGR, hvis farverne er ombyttet på det
// fysiske modul.
#ifndef TFT_RGB_ORDER
#define TFT_RGB_ORDER TFT_RGB    // RGB farverækkefølge (juster til TFT_BGR hvis farverne er ombyttet).
#endif