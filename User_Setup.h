// =========================================================================
// User_Setup.h — TFT_eSPI konfiguration
// Display: 5.0" TFT LCD, 800x480, SSD1963 controller, SPI interface
// MCU: ESP32 Dev Module
// Filplacering: Arduino/libraries/TFT_eSPI/User_Setup.h
// (erstat den originale fil, eller opret en ny User_Setup.h i projektmappen
// og inkluder den via #include).
//
// -------------------------------------------------------------------------
// ARKITEKTURÆNDRING — SPI I STEDET FOR 8-BIT PARALLEL:
// Den oprindelige konfiguration brugte 8-bit parallel bus, hvilket krævede
// 12 GPIOs. Men ESP32'en har ikke 12 frie GPIOs, når relæer (12, 13, 14,
// 27), interrupt (25) og I2C (21, 22) er reserveret af firmwaren.
// Forsøg på at dele pins gav kritiske konflikter:
//   - TFT drev relæerne under skærmopdatering
//   - "Hov-fejl"-beskyttelsen (GPIO 25) blev deaktiveret
//   - I2C-bussen (GPIO 21/22) blev forstyrret
// Løsningen er at skifte til SPI-interface, som kun kræver 6 GPIOs.
// SSD1963 understøtter SPI op til 30 MHz, hvilket giver 800×480 @ ~30 Hz.
// -------------------------------------------------------------------------
// =========================================================================

// -- 1. Vælg driver --
// SSD1963 SPI driver (til 5" 800x480-skærme over SPI).
#define SSD1963_DRIVER           // Aktiver SSD1963 SPI driver (ikke 8DRIVER).

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
#define TFT_MISO -1              // SPI MISO ikke brugt af SSD1963 (write-only) → -1.

// -- 5. SPI-frekvens --
// SSD1963 understøtter op til 30 MHz SPI. 27 MHz giver stabil drift
// og ~30 Hz skærmopdatering ved 800×480.
#define SPI_FREQUENCY  27000000  // 27 MHz SPI-klokfrekvens.

// -- 6. Backlight (valgfrit) --
// Hvis backlight styres via GPIO, definer følgende; ellers udelad.
// #define TFT_BL   22           // Backlight PWM pin (men GPIO 22 er I2C SCL — brug en anden).
// #define TFT_BACKLIGHT_ON HIGH

// -- 7. Farvedybde --
#define TFT_RGB_ORDER TFT_RGB    // RGB farverækkefølge (juster til TFT_BGR hvis farverne er ombyttet).