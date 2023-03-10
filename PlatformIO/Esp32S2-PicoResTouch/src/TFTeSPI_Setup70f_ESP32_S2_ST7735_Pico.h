// Setup for the ESP32 S2 with ST7735 80x160 display
// See SetupX_Template.h for all options available

#define USER_SETUP_ID 70

#define ST7735_DRIVER

#define TFT_SDA_READ // Display has a bidirectional SDA pin (no MISO)

#define TFT_WIDTH  80
#define TFT_HEIGHT 160

#define ST7735_GREENTAB160x80
//#define ST7735_REDTAB160x80

//#define TFT_RGB_ORDER TFT_RGB  // Colour order Red-Green-Blue
#define TFT_RGB_ORDER TFT_BGR  // Colour order Blue-Green-Red

#define TFT_INVERSION_ON
// #define TFT_INVERSION_OFF

// Waveshare ESP32S2-Pico LCD
#define TFT_CS    9 //     10 or 34

#define TFT_MOSI 11 //     11 or 35
#define TFT_SCLK 10 //     12 or 36

#define TFT_DC   18
#define TFT_RST  21

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

// FSPI port must be used for SDA reads. Do not use #define USE_HSPI_PORT

#define SPI_FREQUENCY       27000000
#define SPI_READ_FREQUENCY  16000000
