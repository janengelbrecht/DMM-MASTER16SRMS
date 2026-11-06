// =========================================================================
// User_Setup.h — TFT_eSPI konfiguration
// Display: 5.0" TFT LCD, 800x480, SSD1963 controller, 8-bit parallel
// MCU: ESP32 Dev Module
// Filplacering: Arduino/libraries/TFT_eSPI/User_Setup.h 
// (erstat den originale fil, eller opret en ny User_Setup.h i projektmappen 
// og inkluder den via #include).
//
// Et User_Setup.h-eksempel til TFT_eSPI, der matcher det konkrete display 
// i dette projekt: 5-tommer, 800×480 opløsning, SSD1963-controller, 
// 8-bit parallel bus. SSD1963 er en almindelig controller til 5-tommer 
// 800×480-skærme, men bemærk: TFT_eSPI understøtter paralleltilstand 
// primært på ESP32, og pinnene skal vælges efter dit faktiske 
// ledningsføring. 
// Dette eksempel antager brug af ESP32 Dev Module 
// med 8-bit parallel interface.
//
// Dette eksempel er baseret på følgende typiske hardware:
// Display: 5,0 tommer TFT LCD, opløsning 800×480
// Controller: SSD1963 (understøtter 8-bit/16-bit parallel bus)
// Grænseflade: 8-bit parallel (DB0–DB7) 
// + kontrolsignaler (CS, DC/RS, WR, RD, RST)
// MCU: ESP32 Dev Module
// Hvis dit display faktisk bruger ILI9488 eller ST7262, 
// skal du udskifte driver-makroen nedenfor med #define ILI9488_DRIVER 
// eller den tilsvarende driver. SSD1963 er valgt her, 
// fordi det er en almindelig controller til 
// 5-tommer 800×480-skærme på markedet
// =========================================================================

// -- 1. Vælg driver --
// SSD1963 8-bit parallel driver (til 5" 800x480-skærme)
#define SSD1963_8DRIVER          // Aktiver SSD1963 8-bit parallel driver

// -- 2. Skærmopløsning --
#define TFT_WIDTH  800           // Fysisk bredde 800 pixels
#define TFT_HEIGHT 480           // Fysisk højde 480 pixels

// -- 3. Aktiver 8-bit parallel tilstand --
// TFT_eSPI understøtter 8-bit parallel bus på ESP32
#define TFT_PARALLEL_8_BIT

// -- 4. Kontrolpins (skal tilpasses din faktiske ledningsføring) --
#define TFT_CS   33              // Chip select
#define TFT_DC   15              // Data/Command (RS)
#define TFT_RST  32              // Reset
#define TFT_WR    4              // Write strobe
#define TFT_RD    2              // Read strobe (kan udelades hvis ikke nødvendig)

// -- 5. Databus DB0–DB7 (8-bit parallel) --
// Disse pins skal være i GPIO 0–31 intervallet, så en enkelt register-skrivning kan sætte hele porten
#define TFT_D0   12              // DB0
#define TFT_D1   13              // DB1
#define TFT_D2   26              // DB2
#define TFT_D3   25              // DB3
#define TFT_D4   17              // DB4
#define TFT_D5   16              // DB5
#define TFT_D6   27              // DB6
#define TFT_D7   14              // DB7

// -- 6. Backlight (valgfrit) --
// Hvis backlight styres via GPIO, definer følgende; ellers udelad
// #define TFT_BL   21           // Backlight PWM pin
// #define TFT_BACKLIGHT_ON HIGH

// -- 7. SPI-frekvens (bruges ikke i paralleltilstand, men skal være defineret) --
#define SPI_FREQUENCY  27000000  // Standardværdi, påvirker ikke paralleltilstand

// -- 8. Farvedybde --
#define TFT_RGB_ORDER TFT_RGB    // RGB farverækkefølge (juster til TFT_BGR hvis farverne er ombyttet)