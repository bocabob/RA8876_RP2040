# RA8876_RP2040

Arduino library for the RAiO RA8876 display controller, ported to the **Raspberry Pi Pico (RP2040)** using the [EarlePhilhower arduino-pico](https://github.com/earlephilhower/arduino-pico) board support package.

Includes the `treedee` example — a 3D rotating wire-frame cube adapted from sumotoy's RA8875 library.

---

## Documentation

- [Usage guide](docs/RA8876_RP2040_Guide.md) — functions and methods, configuration, hardware features
- [RA8876_RP2040 vs TFT_eSPI_RA8876](docs/Library_Comparison.md) — which library to choose

---

## Hardware

| Component | Notes |
|-----------|-------|
| Raspberry Pi Pico / Pico W | RP2040, 125 MHz |
| RA8876-based display | RAiO RA8876 controller, SPI mode |
| SPI bus | SPI1 (secondary bus) |

### Wiring

Pin assignments are defined in `RA8876_Config_SPI.h` and can be changed there.

| Display signal | Pico GPIO | Notes |
|----------------|-----------|-------|
| GND  (pin 1,2) | GND       | |
| VCC  (pin 3,4) | VBUS/5V   | or 3V3 per module |
| CS   (pin 5)   | GP 9      | SPI1 — software CS |
| MISO (pin 6)   | GP 8      | SPI1 RX |
| MOSI (pin 7)   | GP 11     | SPI1 TX |
| SCK  (pin 8)   | GP 10     | SPI1 SCK |
| RES  (pin 11)  | GP 14     | Reset; also wire NO button to GND |
| (no DC pin)    | —         | not used for RA8876 |
| CTPSDA (I2C)   | GP 12     | GT9271 SDA (I2C0) |
| CTPSCL (I2C)   | GP 13     | GT9271 SCL (I2C0) |
| CTPRST         | —         | not software-controlled; NO button to GND |
| CTPINT         | —         | not connected; polling requires no INT |

#### Touch screen

The ER-TFTM101-1 uses a **GT9271** capacitive touch controller on I2C (address `0x5D`). This is not compatible with the XPT2046 SPI touch driver used by TFT_eSPI. Use the [bb_captouch](https://github.com/bitbank2/bb_captouch) library instead. Set `TOUCH_CS -1` in any TFT_eSPI config to disable SPI touch.

See `bb_captouch/examples/touch_demo/touch_demo.ino` for usage.

---

## Installation

1. Install [EarlePhilhower RP2040 board support](https://github.com/earlephilhower/arduino-pico) in Arduino IDE.
2. Select **Tools → Board → Raspberry Pi Pico** (or Pico W).
3. Copy this folder into your Arduino sketchbook.
4. Open `treedee.ino` and upload.

### Board settings

| Setting | Value |
|---------|-------|
| Flash Size | 2MB |
| CPU Speed | 125 MHz (200 MHz also works) |
| Optimize | -O2 |

---

## Configuration (`RA8876_Config_SPI.h`)

```cpp
#define RA8876_CS      9    // Chip Select GPIO
#define RA8876_RESET   14   // Reset GPIO
#define RA8876_MOSI    11   // SPI1 TX
#define RA8876_SCLK    10   // SPI1 SCK
#define RA8876_MISO    8    // SPI1 RX

// #define USE_SPI_47000000   // Use 47 MHz SPI (short wires only; 30 MHz recommended)
// #define BACKLITE  5        // GPIO pin for external backlight PWM control
// #define USE_FT5206_TOUCH   // Enable FT5206 capacitive touch (requires Wire)
```

---

## Quick Start

```cpp
#include "RA8876_Config_SPI.h"
#include <SPI.h>
#include "src/RA8876_RP2040.h"

RA8876_RP2040 tft(RA8876_CS, RA8876_RESET, RA8876_MOSI, RA8876_SCLK, RA8876_MISO);

void setup() {
    Serial.begin(115200);
    tft.begin(30000000);          // 30 MHz SPI clock (verified stable)
    tft.fillScreen(0x0000);       // black
    tft.setCursor(0, 0);
    tft.setFontSize(1, false);
    tft.printStatusLine(0, 0xFFFF, 0x0000, "Hello Pico!");
}
```

---

## API Reference

### Construction & Initialization

```cpp
RA8876_RP2040(uint8_t cs, uint8_t rst, uint8_t mosi, uint8_t sclk, uint8_t miso)
```
Create a driver instance. Pass GPIO pin numbers for all five SPI signals.

```cpp
boolean begin(uint32_t spi_clock = 30000000)
```
Initialize SPI1, reset the display, verify the chip ID, and run the full RA8876 startup sequence. Returns `true` on success. Prints diagnostic messages to `Serial`.

---

### Display Control

```cpp
void fillScreen(uint16_t color)
```
Fill the entire display with a 16-bit RGB565 color.

```cpp
void displayOn(boolean on)
```
Turn the display output on or off.

```cpp
void backlight(boolean on)
```
Enable or disable the RA8876 internal backlight PWM output. For displays with external backlight control, use `analogWrite()` on the `BACKLITE` pin instead.

```cpp
int16_t width()
int16_t height()
```
Return display dimensions in pixels.

---

### Drawing

All coordinates are in pixels. Colors are 16-bit RGB565.

```cpp
void drawPixel(uint16_t x, uint16_t y, uint16_t color)
uint16_t readPixel(int16_t x, int16_t y)
uint16_t getPixel(uint16_t x, uint16_t y)
```

```cpp
void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
```

```cpp
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
void drawSquare(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
void drawSquareFill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
```

```cpp
void drawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t xr, uint16_t yr, uint16_t color)
void fillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t xr, uint16_t yr, uint16_t color)
void drawCircleSquare(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t xr, uint16_t yr, uint16_t color)
void drawCircleSquareFill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t xr, uint16_t yr, uint16_t color)
```

```cpp
void drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
void drawCircleFill(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
```

```cpp
void drawEllipse(uint16_t x0, uint16_t y0, uint16_t xr, uint16_t yr, uint16_t color)
void drawEllipseFill(uint16_t x0, uint16_t y0, uint16_t xr, uint16_t yr, uint16_t color)
void fillEllipse(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color)
```

```cpp
void drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
void drawTriangleFill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
```

#### Gradient fills

```cpp
void fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2)
void fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2)
```

---

### Text & Fonts

```cpp
void setCursor(uint16_t x, uint16_t y)   // (inherited via Print)
void setTextColor(uint16_t color)
void setTextColor(uint16_t fgColor, uint16_t bgColor)
boolean setFontSize(uint8_t scale, boolean runflag = false)
uint8_t getFontWidth()
uint8_t getFontHeight()
int16_t getTextX()
int16_t getTextY()
```

```cpp
void textColor(uint16_t foreground_color, uint16_t background_color)
void setTextCursor(uint16_t x, uint16_t y)
void textxy(uint16_t x, uint16_t y)
void putString(uint16_t x0, uint16_t y0, const char *str)
```

```cpp
// Status bar (bottom row reserved area)
void printStatusLine(uint16_t x0, uint16_t fgColor, uint16_t bgColor, const char *text)
void fillStatusLine(uint16_t color)
void clearStatusLine(uint16_t color)
```

```cpp
// Text cursor display
void Enable_Text_Cursor()
void Disable_Text_Cursor()
void Enable_Text_Cursor_Blinking()
void Disable_Text_Cursor_Blinking()
void Blinking_Time_Frames(uint8_t frames)
void Text_Cursor_H_V(uint16_t w, uint16_t h)
```

The library inherits from `Print`, so `tft.print()` / `tft.println()` work normally.

---

### Window & Margins

```cpp
void setMargins(uint16_t xl, uint16_t yt, uint16_t xr, uint16_t yb)
void setTMargins(uint16_t xl, uint16_t yt, uint16_t xr, uint16_t yb)
void activeWindowXY(uint16_t x0, uint16_t y0)
void activeWindowWH(uint16_t width, uint16_t height)
```

```cpp
void setOrigin(int16_t x = 0, int16_t y = 0)
void getOrigin(int16_t *x, int16_t *y)
void setClipRect(int16_t x1, int16_t y1, int16_t w, int16_t h)
void setClipRect()   // reset to full display
```

---

### Rotation

```cpp
void setRotation(uint8_t rotation)   // 0–3
uint8_t getRotation()
void textRotate(boolean on)
```

---

### Image / Pixel Block Transfer

```cpp
void writeRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors)
void readRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pcolors)
void writeRotatedRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors)
uint16_t *rotateImageRect(int16_t w, int16_t h, const uint16_t *pcolors, int16_t rotation = -1)
```

#### Paletted bitmaps

```cpp
void writeRect1BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t *palette)
void writeRect2BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t *palette)
void writeRect4BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t *palette)
void writeRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t *palette)
```

---

### BTE (Block Transfer Engine)

The RA8876 has a hardware BTE unit for fast SDRAM-to-SDRAM and CPU-to-SDRAM transfers with optional ROP or chroma-key compositing. Prefer these over `putPicture_*` functions.

```cpp
// Copy within SDRAM
void bteMemoryCopy(uint32_t s0_addr, uint16_t s0_image_width, uint16_t s0_x, uint16_t s0_y,
                   uint32_t des_addr, uint16_t des_image_width, uint16_t des_x, uint16_t des_y,
                   uint16_t copy_width, uint16_t copy_height)

void bteMemoryCopyWithROP(uint32_t s0_addr, uint16_t s0_image_width, uint16_t s0_x, uint16_t s0_y,
                          uint32_t s1_addr, uint16_t s1_image_width, uint16_t s1_x, uint16_t s1_y,
                          uint32_t des_addr, uint16_t des_image_width, uint16_t des_x, uint16_t des_y,
                          uint16_t copy_width, uint16_t copy_height, uint8_t rop_code)

void bteMemoryCopyWithChromaKey(uint32_t s0_addr, uint16_t s0_image_width, uint16_t s0_x, uint16_t s0_y,
                                uint32_t des_addr, uint16_t des_image_width, uint16_t des_x, uint16_t des_y,
                                uint16_t copy_width, uint16_t copy_height, uint16_t chromakey_color)

void bteMemoryCopyWindowAlpha(uint32_t s0_addr, uint16_t s0_image_width, uint16_t s0_x, uint16_t s0_y,
                              uint32_t s1_addr, uint16_t s1_image_width, uint16_t s1_x, uint16_t s1_y,
                              uint32_t des_addr, uint16_t des_image_width, uint16_t des_x, uint16_t des_y,
                              uint16_t copy_width, uint16_t copy_height, uint8_t alpha)
```

```cpp
// CPU → SDRAM write with ROP
void bteMpuWriteWithROPData8(uint32_t s1_addr, uint16_t s1_image_width, uint16_t s1_x, uint16_t s1_y,
                             uint32_t des_addr, uint16_t des_image_width, uint16_t des_x, uint16_t des_y,
                             uint16_t width, uint16_t height, uint8_t rop_code, const uint8_t *data)

void bteMpuWriteWithROPData16(uint32_t s1_addr, uint16_t s1_image_width, uint16_t s1_x, uint16_t s1_y,
                              uint32_t des_addr, uint16_t des_image_width, uint16_t des_x, uint16_t des_y,
                              uint16_t width, uint16_t height, uint8_t rop_code, const uint16_t *data)
```

```cpp
// CPU → SDRAM write with chroma key
void bteMpuWriteWithChromaKeyData8(uint32_t des_addr, uint16_t des_image_width, uint16_t des_x, uint16_t des_y,
                                   uint16_t width, uint16_t height, uint16_t chromakey_color, const uint8_t *data)

void bteMpuWriteWithChromaKeyData16(uint32_t des_addr, uint16_t des_image_width, uint16_t des_x, uint16_t des_y,
                                    uint16_t width, uint16_t height, uint16_t chromakey_color, const uint16_t *data)
```

```cpp
// Pattern fill
void btePatternFill(uint8_t p8x8or16x16, uint32_t s0_addr, uint16_t s0_image_width, uint16_t s0_x, uint16_t s0_y,
                    uint32_t des_addr, uint16_t des_image_width, uint16_t des_x, uint16_t des_y,
                    uint16_t width, uint16_t height)

void btePatternFillWithChromaKey(uint8_t p8x8or16x16, uint32_t s0_addr, uint16_t s0_image_width, uint16_t s0_x, uint16_t s0_y,
                                 uint32_t des_addr, uint16_t des_image_width, uint16_t des_x, uint16_t des_y,
                                 uint16_t width, uint16_t height, uint16_t chromakey_color)
```

Common `rop_code` values:

| Code | Operation |
|------|-----------|
| 0x0C | Source copy (overwrite) |
| 0x06 | XOR |
| 0x00 | Black |
| 0x0F | White |

---

### Canvas / Display Regions

```cpp
bool setCanvasRegion(uint32_t address, uint16_t width = 0)
bool setCanvasWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
bool setDisplayRegion(uint32_t address, uint16_t width)
bool setDisplayOffset(uint16_t x, uint16_t y)
void useCanvas(boolean on)
void updateScreen()
```

---

### PWM (Backlight)

```cpp
void pwm_Prescaler(uint8_t prescaler)
void pwm_ClockMuxReg(uint8_t pwm1_clk_div, uint8_t pwm0_clk_div, uint8_t xpwm1_ctrl, uint8_t xpwm0_ctrl)
void pwm_Configuration(uint8_t pwm1_inverter, uint8_t pwm1_auto_reload, uint8_t pwm1_start,
                        uint8_t pwm0_dead_zone, uint8_t pwm0_inverter, uint8_t pwm0_auto_reload, uint8_t pwm0_start)
void pwm0_ClocksPerPeriod(uint16_t clocks_per_period)
void pwm0_Duty(uint16_t duty)
void pwm1_ClocksPerPeriod(uint16_t clocks_per_period)
void pwm1_Duty(uint16_t duty)
```

---

### Graphic Cursor

```cpp
void Enable_Graphic_Cursor()
void Disable_Graphic_Cursor()
void Select_Graphic_Cursor_1()   // through _4()
void Upload_Graphic_Cursor(uint8_t cursorNum, uint8_t *data)
void Graphic_Cursor_XY(int16_t x, int16_t y)
void Set_Graphic_Cursor_Color_1(uint8_t color)
void Set_Graphic_Cursor_Color_2(uint8_t color)
void Graphic_cursor_initial()
void gCursorSet(boolean enable, uint8_t type, uint8_t color1, uint8_t color2)
void gcursorxy(uint16_t x, uint16_t y)
uint16_t GetGCursorX()
uint16_t GetGCursorY()
```

---

### PIP (Picture-In-Picture)

```cpp
void PIP(uint8_t On_Off, uint8_t Select_PIP, uint32_t PAddr,
         uint16_t XP, uint16_t YP, uint32_t ImageWidth,
         uint16_t X_Dis, uint16_t Y_Dis, uint16_t X_W, uint16_t Y_H)
void Enable_PIP1()
void Disable_PIP1()
void Enable_PIP2()
void Disable_PIP2()
void Select_PIP1_Parameter()
void Select_PIP2_Parameter()
void PIP_Display_Start_XY(uint16_t x, uint16_t y)
void PIP_Image_Start_Address(uint32_t addr)
void PIP_Image_Width(uint16_t w)
void PIP_Window_Image_Start_XY(uint16_t x, uint16_t y)
void PIP_Window_Width_Height(uint16_t w, uint16_t h)
```

---

### Color Utilities

```cpp
static uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
static void color565toRGB(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b)
static void color565toRGB14(uint16_t color, int16_t &r, int16_t &g, int16_t &b)
static uint16_t RGB14tocolor565(int16_t r, int16_t g, int16_t b)
```

---

### Low-Level SPI / Register Access

These are used internally but are available if you need direct register control.

```cpp
void lcdRegWrite(uint8_t reg, bool finalize = true)
void lcdDataWrite(uint8_t data, bool finalize = true)
uint8_t lcdDataRead(bool finalize = true)
uint8_t lcdStatusRead(bool finalize = true)
void lcdRegDataWrite(uint8_t reg, uint8_t data, bool finalize = true)
uint8_t lcdRegDataRead(uint8_t reg, bool finalize = true)
void lcdDataWrite16bbp(uint16_t data, bool finalize = true)
```

---

### Diagnostics

```cpp
boolean checkIcReady()      // poll status register until IC is ready
boolean checkSdramReady()   // confirm SDRAM initialization
void checkWriteFifoNotFull()
void checkWriteFifoEmpty()
void check2dBusy()          // wait for 2D engine to finish
uint8_t powerSavingStatus()
void Color_Bar_ON()         // display built-in color bar test pattern
void Color_Bar_OFF()
```

---

## Performance

| SPI Clock | Result |
|-----------|--------|
| 1 MHz     | Safe fallback |
| 30 MHz    | Verified stable (default) |
| 47 MHz    | Unstable — not recommended |

The RA8876 hardware accelerates all drawing operations (lines, rectangles, circles, BTE) so CPU time per frame is low even at modest SPI speeds.

---

## Troubleshooting

**`begin()` returns false / IC ready check fails**
- Verify MOSI/MISO/SCK/CS/RST wiring matches `RA8876_Config_SPI.h`.
- Confirm 3V3 and GND are connected.
- Lower the SPI clock: `tft.begin(1000000)`.

**Display shows garbage or artifacts**
- Lower SPI clock.
- Use shorter wires (under 15 cm preferred).

**Compile error: `has no member named 'setTX'`**
- You are calling pin-setup methods through a `SPIClass*` pointer. Call them on the concrete `SPI1` object directly (already fixed in this library).

**Serial Monitor blank**
- Check baud rate is 115200.
- `begin()` waits up to 1 second for Serial before continuing.

---

## Credits

- Original RA8876 library by Warren Watson, mjs513, KurtE, MorganS for Teensy
- `treedee` sketch adapted from sumotoy's RA8875 library
- RP2040 port by the project contributors
