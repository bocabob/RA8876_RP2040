/* RA8876_Config_SPI.h
 A file to place user defines and configs.
*/

#ifndef RA8876_CONFIG_SPI_H
#define RA8876_CONFIG_SPI_H

// SPI hardware settings
#define USE_SPI // Needed for writeRect() in RA8876_GFX

// Raspberry Pi Pico SPI1 pins
#define RA8876_CS 9       // GPIO9
#define RA8876_RESET 14   // GPIO14
#define RA8876_MOSI 11    // GPIO11 (SPI1 TX)
#define RA8876_SCLK 10    // GPIO10 (SPI1 SCK)
#define RA8876_MISO 8     // GPIO8  (SPI1 RX)

//#define RA8876_CS 30
//#define RA8876_RESET 28

//Uncomment to to use 47MHz SPI clock. Default is 30MHz.
// #define USE_SPI_47000000

// Uncomment to use BL pin else wired to 3.3V
//#define BACKLITE 5 //External backlight control connected to this Arduino pin
//#define BACKLITE 29 // Kurt's DB5 shield

// Uncomment to use FT5206 touch.
// #define USE_FT5206_TOUCH

#endif // RA8876_CONFIG_H
