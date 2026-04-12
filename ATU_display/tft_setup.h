// tft_setup.h
// CYD (ESP32-2432S028) - ST7789 driver, HSPI
// Ucitava se automatski iz TFT_eSPI.h ako postoji u sketch folderu.
// Za drugi hardver zamijeni ovaj fajl sa odgovarajucim tft_setup.h
//
// Varijante:
//   CYD_SHARED_SPI  - TFT i touch dijele iste SPI pinove (ovaj hardver)
//   (bez define)    - TFT na HSPI, touch na VSPI sa razlicitim pinovima

#define USER_SETUP_INFO "CYD_ST7789"

// --- Driver ---
#define ST7789_2_DRIVER

// --- TFT Pinout (HSPI) ---
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST -1  // spojen na board RST

#define USE_HSPI_PORT

// --- Touch (dijeli HSPI pinove sa TFT, razlicit CS) ---
#define CYD_SHARED_SPI
//#define CYD_TOUCH_MIRROR_X   // X osa u ogledalu
#define CYD_TOUCH_MIRROR_Y  // Y osa u ogledalu
#define XPT2046_IRQ 36
#define XPT2046_MOSI 13  // = TFT_MOSI
#define XPT2046_MISO 12  // = TFT_MISO
#define XPT2046_CLK 14   // = TFT_SCLK
#define XPT2046_CS 33

// --- Touch kalibracija (raw ADC vrijednosti sa XPT2046) ---
#define TOUCH_X_MIN 200
#define TOUCH_X_MAX 3800
#define TOUCH_Y_MIN 240
#define TOUCH_Y_MAX 3800

// --- Backlight (PWM u sketchu, ne kroz TFT_eSPI) ---
#define BL_PIN 27

// --- Fontovi (potrebni za OTA ekran - tft.setTextFont(1)) ---
#define LOAD_GLCD
//#define LOAD_FONT2
//#define LOAD_FONT4
//#define LOAD_FONT6
//#define LOAD_FONT7
//#define LOAD_FONT8
//#define LOAD_GFXFF
//#define SMOOTH_FONT

// --- SPI brzine ---
#define SPI_FREQUENCY 55000000
#define SPI_READ_FREQUENCY 20000000

//#define TFT_INVERSION_ON
