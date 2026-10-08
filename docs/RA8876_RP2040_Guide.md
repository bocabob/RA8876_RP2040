# RA8876_RP2040 — Usage Guide

A guide to the functions and methods of **RA8876_RP2040**, a port of the Teensy RA8876 library (Warren Watson, mjs513, KurtE, MorganS) to the Raspberry Pi Pico using the EarlePhilhower arduino-pico core. It drives the RAiO **RA8876** controller over SPI and exposes the chip's hardware features: the 2D drawing engine, Block Transfer Engine (BTE), multiple SDRAM pages, picture-in-picture, graphic cursor, ROM fonts and PWM.

> Choosing between this library and `TFT_eSPI_RA8876`? See [Library_Comparison.md](Library_Comparison.md).

---

## Contents

1. [Concepts](#1-concepts)
2. [Configuration and wiring](#2-configuration-and-wiring)
3. [Initialization](#3-initialization)
4. [Display control and orientation](#4-display-control-and-orientation)
5. [Colors](#5-colors)
6. [Drawing primitives (hardware accelerated)](#6-drawing-primitives-hardware-accelerated)
7. [Text and fonts](#7-text-and-fonts)
8. [Text screen, margins, scrolling and status line](#8-text-screen-margins-scrolling-and-status-line)
9. [Images and pixel blocks](#9-images-and-pixel-blocks)
10. [Origin and clipping](#10-origin-and-clipping)
11. [SDRAM pages, canvas and double buffering](#11-sdram-pages-canvas-and-double-buffering)
12. [Block Transfer Engine (BTE)](#12-block-transfer-engine-bte)
13. [Picture-in-picture (PIP)](#13-picture-in-picture-pip)
14. [Graphic cursor and text cursor](#14-graphic-cursor-and-text-cursor)
15. [Backlight and PWM](#15-backlight-and-pwm)
16. [Touch](#16-touch)
17. [Low-level register access and diagnostics](#17-low-level-register-access-and-diagnostics)
18. [Notes and limitations](#18-notes-and-limitations)

---

## 1. Concepts

| Item | Value |
|------|-------|
| Header | `#include <RA8876_RP2040.h>` |
| Class | `RA8876_RP2040` (derives from `RA8876_common`, which derives from `Print`) |
| Config file | `RA8876_Config_SPI.h` in the sketch folder |
| MCU | RP2040 only (`architectures=rp2040`), SPI1 |
| Types | `ru8`, `ru16`, `ru32` = `uint8_t`, `uint16_t`, `uint32_t` |

**How drawing works.** Most primitives are *commands* to the RA8876: the library writes a few registers (coordinates, color) and the controller's 2D engine draws into its own SDRAM. A full-screen fill takes a handful of SPI bytes regardless of size. Image data still travels over SPI, but can then be copied, blended and composited inside the controller with the BTE.

**Display memory.** The RA8876 has its own SDRAM, organised by this library as ten 1024×600×16-bit pages (`PAGE1_START_ADDR`…`PAGE10_START_ADDR`, also `SCREEN_1`…`SCREEN_9`) plus pattern RAM. One page is shown; any page can be drawn to.

---

## 2. Configuration and wiring

Copy `RA8876_Config_SPI.h` from any example into your sketch folder and edit it:

```cpp
#define USE_SPI

#define RA8876_CS     9   // GPIO9  (software CS)
#define RA8876_RESET 14   // GPIO14
#define RA8876_MOSI  11   // GPIO11 SPI1 TX
#define RA8876_SCLK  10   // GPIO10 SPI1 SCK
#define RA8876_MISO   8   // GPIO8  SPI1 RX

// #define USE_SPI_47000000   // examples use this to call begin(47000000)
// #define BACKLITE 5         // GPIO for external backlight control
// #define USE_FT5206_TOUCH   // FT5206 capacitive touch (not the GT9271)
```

| Display signal | Pico GPIO |
|----------------|-----------|
| CS | GP9 |
| MISO | GP8 |
| MOSI | GP11 |
| SCK | GP10 |
| RESET | GP14 |
| CTP SDA / SCL (GT9271) | GP12 / GP13 (I²C0) |

The pins must be valid **SPI1** pins; the library always uses `SPI1`.

Board settings: Raspberry Pi Pico / Pico W, 125 MHz (200 MHz also works), `-O2`.

---

## 3. Initialization

```cpp
#include "RA8876_Config_SPI.h"
#include <SPI.h>
#include <RA8876_RP2040.h>

RA8876_RP2040 tft(RA8876_CS, RA8876_RESET, RA8876_MOSI, RA8876_SCLK, RA8876_MISO);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 1000) {}
  if (!tft.begin(30000000)) {          // 30 MHz SPI
    Serial.println("RA8876 not found");
    while (true) {}
  }
  tft.graphicMode(true);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE, BLACK);
  tft.setCursor(10, 10);
  tft.println("Hello Pico");
}
```

| Method | Description |
|--------|-------------|
| `RA8876_RP2040(uint8_t cs = 5, uint8_t rst = 6, uint8_t mosi = 3, uint8_t sclk = 2, uint8_t miso = 4)` | Constructor. Always pass your pins — the defaults are not the documented wiring. Pass `rst = 255` if reset is not wired. |
| `boolean begin(uint32_t spi_clock = 30000000)` | Configures SPI1 pins, pulses reset, waits for IC ready, checks chip ID (0x76/0x77), runs PLL/SDRAM/panel init. Returns `false` on failure. Prints diagnostics to `Serial`. |
| `boolean ra8876Initialize()`, `ra8876PllInitial()`, `ra8876SdramInitial()` | Init stages (called by `begin`). |
| `void RA8876_SW_Reset()` | Software reset. |

---

## 4. Display control and orientation

| Method | Description |
|--------|-------------|
| `void displayOn(boolean on)` | Panel output on/off. |
| `void backlight(boolean on)` | Internal PWM backlight output on/off (see §15). |
| `void setRotation(uint8_t r)` / `uint8_t getRotation()` | 0–3. Rotations 1 and 3 are **true portrait** (600×1024): `width()`/`height()` swap. |
| `void textRotate(boolean on)` | Rotate ROM-font text 90°. |
| `int16_t width()`, `int16_t height()` | Size in current rotation. |
| `void graphicMode(boolean on)` / `void textMode(boolean on)` | Switch the controller between graphic and ROM-text modes (the library mostly switches automatically). |
| `void Color_Bar_ON()` / `Color_Bar_OFF()` | Built-in test pattern. |

---

## 5. Colors

RGB565 everywhere. Constants from `RA8876Registers.h`:

- Short names: `BLACK`, `WHITE`, `RED`, `LIGHTRED`, `CRIMSON`, `GREEN`, `PALEGREEN`, `DARKGREEN`, `BLUE`, `LIGHTBLUE`, `SKYBLUE`, `DARKBLUE`, `YELLOW`, `LIGHTYELLOW`, `DARKYELLOW`, `CYAN`, `LIGHTCYAN`, `DARKCYAN`, `MAGENTA`, `VIOLET`, `BLUEVIOLET`, `ORCHID`.
- `COLOR65K_*` names (e.g. `COLOR65K_RED`) and greys `COLOR65K_GRAYSCALE1`…`COLOR65K_GRAYSCALE30`.

| Method | Description |
|--------|-------------|
| `static uint16_t color565(uint8_t r, uint8_t g, uint8_t b)` | 8-bit R,G,B → RGB565. |
| `static void color565toRGB(uint16_t c, uint8_t &r, uint8_t &g, uint8_t &b)` | RGB565 → 8-bit components. |
| `static void color565toRGB14(...)` / `static uint16_t RGB14tocolor565(...)` | Fixed-point helpers for gradients. |
| `void foreGroundColor16bpp(ru16 c)` / `void backGroundColor16bpp(ru16 c)` | Set the 2D engine's colors directly. |

---

## 6. Drawing primitives (hardware accelerated)

Two naming styles exist. The **Adafruit-style** names take `x, y, w, h`; the **RA8876-style** names take corner coordinates.

### Adafruit-style

| Method | Notes |
|--------|-------|
| `drawPixel(ru16 x, ru16 y, ru16 color)` | One pixel. |
| `drawFastHLine(x, y, w, color)` / `drawFastVLine(x, y, h, color)` | |
| `drawRect(x, y, w, h, color)` / `fillRect(x, y, w, h, color)` | |
| `drawRoundRect(x, y, w, h, xr, yr, color)` / `fillRoundRect(...)` | Separate X/Y corner radii. |
| `fillCircle(x0, y0, r, color)` | |
| `fillEllipse(xc, yc, longAxis, shortAxis, color)` | |
| `fillTriangle(x0,y0, x1,y1, x2,y2, color)` | |
| `fillRectHGradient(x, y, w, h, c1, c2)` / `fillRectVGradient(...)` | |
| `fillScreen(uint16_t color)` | Fills the current margin area (whole screen by default; see §8). |

### RA8876-style

| Method | Notes |
|--------|-------|
| `drawLine(x0, y0, x1, y1, color)` | |
| `drawSquare(x0, y0, x1, y1, color)` / `drawSquareFill(...)` | Rectangle by corners. |
| `drawCircleSquare(x0, y0, x1, y1, xr, yr, color)` / `drawCircleSquareFill(...)` | Rounded rectangle by corners. |
| `drawCircle(x0, y0, r, color)` / `drawCircleFill(...)` | |
| `drawEllipse(x0, y0, xr, yr, color)` / `drawEllipseFill(...)` | |
| `drawTriangle(x0,y0, x1,y1, x2,y2, color)` / `drawTriangleFill(...)` | |

```cpp
tft.fillRectVGradient(0, 0, tft.width(), 80, DARKBLUE, BLACK);
tft.drawRoundRect(20, 100, 300, 120, 12, 12, WHITE);
tft.drawCircleFill(600, 300, 60, YELLOW);
tft.check2dBusy();   // wait for the engine before reading back or changing pages
```

Pixel read-back works: `uint16_t readPixel(x, y)`, `ru16 getPixel(x, y)`, `readRect(x, y, w, h, uint16_t *buf)`.

---

## 7. Text and fonts

The class derives from `Print`: `print()`, `println()`, `printf()` and `write()` draw at the text cursor.

### 7.1 Font sources

| Source | How to select | Notes |
|--------|---------------|-------|
| RA8876 internal ROM font | `setFontDef()` then `setFontSize(0..2)` | 8×16, 12×24, 16×32. Fastest — the controller renders glyphs. |
| User-defined 8×16 font in CGRAM | `fontLoadMEM((char*)font8x16)` then `setFontSource(1)` | `setFontSize(0..2)` scales ×1/×2/×3. `setFontSource(0)` returns to ROM. |
| ILI9341_t3 anti-aliased fonts | `#include "font_Arial.h"` then `setFont(Arial_14)` | Many families/sizes included in `src/` (Arial, ComicSansMS, DroidSans, Liberation*, OpenSans, TimesNewRoman, Awesome icon fonts, …). |
| Adafruit GFX fonts | `#include <Fonts/FreeSans12pt7b.h>` then `setFont(&FreeSans12pt7b)` | Requires the Adafruit_GFX library installed. |

### 7.2 Text methods

| Method | Description |
|--------|-------------|
| `setCursor(int16_t x, int16_t y, bool autocenter = false)` | Pixel position. Pass `CENTER` (9998) for x or y to centre on screen. |
| `getCursor(int16_t &x, int16_t &y)`, `getCursorX()`, `getCursorY()` | |
| `setTextCursor(x, y)` / `textxy(x, y)` | Cursor in ROM-text terms (pixels / character cells). |
| `getTextX()`, `getTextY()` | |
| `setTextColor(uint16_t fg)` | Transparent background (custom fonts). |
| `setTextColor(uint16_t fg, uint16_t bg)` | Opaque background. |
| `textColor(fg, bg)` | ROM-text colors. |
| `setBackGroundColor(uint16_t c)` | |
| `boolean setFontSize(uint8_t scale, boolean runflag = false)` | ROM / CGRAM font size 0–2. |
| `setFontDef()` | Return to the internal font. |
| `setFont(const ILI9341_t3_font_t &f)` / `setFont(const GFXfont *f = NULL)` / `setFontAdafruit()` | Custom fonts. |
| `setTextSize(uint8_t s)` / `setTextSize(sx, sy)` | Scaling for GFX/glcd fonts. |
| `uint8_t getFontWidth()`, `uint8_t getFontHeight()` | Current cell size. |
| `getTextBounds(str, x, y, &x1, &y1, &w, &h)` | Bounding box (char*, String or buffer). |
| `int16_t strPixelLen(const char *str)` | Width of a string in the current font. |
| `putString(x0, y0, const char *str)` | Draw a ROM-font string at a position. |
| `drawChar(x, y, c, color, bg, size)` | One glyph. |
| `size_t rawPrint(uint8_t c)` / `tftRawWrite(uint8_t c)` | Write a character without control-code processing. |
| `uint16_t getTextFGC()`, `getTextBGC()` | Current text colors. |

```cpp
#include "font_Arial.h"
tft.setFont(Arial_24);
tft.setTextColor(WHITE, BLACK);
tft.setCursor(CENTER, 40);
tft.print("Centered title");
```

---

## 8. Text screen, margins, scrolling and status line

The library keeps a terminal-like text area with margins, a bottom **status line** (`STATUS_LINE_HEIGHT` = 24 px) and ANSI-style clear functions.

| Method | Description |
|--------|-------------|
| `setMargins(xl, yt, xr, yb)` | Text/scroll area in **pixels**. |
| `setTMargins(xl, yt, xr, yb)` | Margins in **character cells**, measured inward from each edge. `setTMargins(0,0,0,1)` reserves the bottom line. |
| `activeWindowXY(x0, y0)`, `activeWindowWH(w, h)` | Controller active window (drawing limits). |
| `setPromptSize(uint16_t chars)` | Width of a prompt for terminal apps. |
| `scrollUp()`, `scrollDown()`, `scroll()` | Scroll the text area one line. |
| `clearActiveScreen()`, `clreol()`, `clreos()`, `clrbol()`, `clrbos()`, `clrlin()` | Clear screen / to end of line / to end of screen / to beginning of line / to beginning of screen / whole line. |
| `printStatusLine(uint16_t x0, fg, bg, const char *text)` | Write text in the status line (`x0` in characters for ROM fonts). |
| `writeStatusLine(x0, fg, bg, str)` | Same, `x0` in pixels. |
| `fillStatusLine(color)` / `clearStatusLine(color)` | Clear the status line. |

```cpp
tft.fontLoadMEM((char *)font8x16);
tft.setFontSize(1, false);
tft.setTMargins(0, 0, 0, 1);       // keep scrolling off the status bar
tft.fillStatusLine(DARKBLUE);
tft.printStatusLine(0, WHITE, DARKBLUE, "Ready");
```

---

## 9. Images and pixel blocks

| Method | Description |
|--------|-------------|
| `writeRect(x, y, w, h, const uint16_t *pcolors)` | RGB565 image. |
| `readRect(x, y, w, h, uint16_t *pcolors)` | Read back a block. |
| `writeRotatedRect(x, y, w, h, const uint16_t *pcolors)` | Image already rotated to the current orientation (faster in portrait). |
| `uint16_t* rotateImageRect(w, h, const uint16_t *pcolors, int16_t rotation = -1)` | Pre-rotate an image into a newly allocated buffer (`nullptr` if out of memory). The returned pointer is 32-byte aligned *inside* the allocation, so it must **not** be passed to `free()` — rotate once and keep it. |
| `writeRect8BPP / 4BPP / 2BPP / 1BPP(x, y, w, h, const uint8_t *pixels, const uint16_t *palette)` | Paletted bitmaps (min width 1/2/4/8 px). |
| `writeRectNBPP(x, y, w, h, bits, pixels, palette)` | Generic N-bpp. |
| `putPicture(x, y, w, h, const unsigned char *data)` | Byte-array picture. |
| `putPicture_16bppData8 / _16bppData16(...)` | Legacy — prefer the BTE writes in §12. |

For maximum speed, upload an image once to an off-screen page with `bteMpuWriteWithROPData16` and copy it with `bteMemoryCopy` whenever it is needed.

---

## 10. Origin and clipping

| Method | Description |
|--------|-------------|
| `setOrigin(int16_t x = 0, int16_t y = 0)` / `getOrigin(&x, &y)` | Offset all drawing. |
| `setClipRect(x, y, w, h)` | Clip to a rectangle (relative to origin). |
| `setClipRect()` | Remove clipping. |

---

## 11. SDRAM pages, canvas and double buffering

### Pages

| Constant | Address |
|----------|---------|
| `PAGE1_START_ADDR` / `SCREEN_1` | 0 (shown at boot) |
| `PAGE2_START_ADDR` / `SCREEN_2` | 1 228 800 |
| … | +1 228 800 per page |
| `PAGE10_START_ADDR` | used for CGRAM |
| `PATTERN1/2/3_RAM_START_ADDR` | 16×16 pattern storage |

| Method | Description |
|--------|-------------|
| `selectScreen(uint32_t pageAddr)` | Make a page both displayed and drawn-to; saves/restores per-page text state. |
| `currentPage` (member) | Address of the page being drawn to — pass to BTE calls. |
| `saveTFTParams(tftSave_t*)` / `restoreTFTParams(tftSave_t*)` | Save/restore text state manually. |
| `boxPut(pageAddr, x0, y0, x1, y1, dx, dy)` | Copy a region of the current page to another page. |
| `boxGet(pageAddr, x0, y0, x1, y1, dx, dy)` | Copy a region of another page to the current page. |

### Displayed vs. drawn-to memory

| Method | Description |
|--------|-------------|
| `setDisplayRegion(uint32_t addr, uint16_t width)` | Which SDRAM region the panel shows. |
| `setDisplayOffset(x, y)` | Scroll/pan the shown window inside that region. |
| `setCanvasRegion(uint32_t addr, uint16_t width = 0)` | Where drawing goes. |
| `setCanvasWindow(x, y, w, h)` | Drawing window inside the canvas. |
| `displayImageStartAddress()`, `displayImageWidth()`, `displayWindowStartXY()`, `canvasImageStartAddress()`, `canvasImageWidth()` | Raw register equivalents. |

### Double buffering

```cpp
tft.useCanvas(true);     // draw to page 2, show page 1
drawFrame();             // compose off-screen
tft.updateScreen();      // BTE copy page 2 -> page 1 (in hardware)
// tft.useCanvas(false); // draw directly to page 1 again
```

---

## 12. Block Transfer Engine (BTE)

The BTE moves rectangular blocks between SDRAM locations (or from the MCU into SDRAM) with raster operations, chroma-key transparency or alpha blending. Every call takes `(address, image_width, x, y)` for each surface; use `tft.currentPage` / `SCREEN_n` for addresses and `tft.width()` for image width.

### SDRAM → SDRAM

| Method | Description |
|--------|-------------|
| `bteMemoryCopy(s0_addr, s0_w, s0_x, s0_y, des_addr, des_w, des_x, des_y, w, h)` | Plain copy. |
| `bteMemoryCopyWithROP(s0..., s1..., des..., w, h, rop)` | Combine two sources with a ROP. |
| `bteMemoryCopyWithChromaKey(s0..., des..., w, h, key_color)` | Copy, skipping `key_color` pixels (sprites over backgrounds). |
| `bteMemoryCopyWindowAlpha(s0..., s1..., des..., w, h, uint8_t alpha)` | Blend two sources (alpha 0–32). |
| `btePatternFill(p8x8or16x16, s0..., des..., w, h)` | Tile an 8×8 or 16×16 pattern. |
| `btePatternFillWithChromaKey(..., key_color)` | Pattern fill with transparency. |

### MCU → SDRAM

| Method | Description |
|--------|-------------|
| `bteMpuWriteWithROPData16(s1..., des..., w, h, rop, const uint16_t *data)` | Upload RGB565. Use `rop = RA8876_BTE_ROP_CODE_12` (12 = copy source). |
| `bteMpuWriteWithROPData8(..., const uint8_t *data)` | Same, byte array. |
| `bteMpuWriteWithChromaKeyData16 / Data8(des..., w, h, key_color, data)` | Upload with transparency. |
| `bteMpuWriteColorExpansionData(des..., w, h, fg, bg, const uint8_t *data)` | Expand a 1-bpp bitmap to two colors in hardware. |
| `bteMpuWriteColorExpansionWithChromaKeyData(...)` | 1-bpp expansion with transparent background. |
| `bteMpuWriteWithROP / WithChromaKey / ColorExpansion(...)` (no `Data`) | Start the operation; you then stream the data yourself. |

Common ROP codes (`rop_code` 0–15, constants `RA8876_BTE_ROP_CODE_0`…`_15`): 12 (`0x0C`) source copy, 6 (`0x06`) XOR, 0 black, 15 (`0x0F`) white.

```cpp
// Upload a sprite once to page 2, then stamp it with transparency on the visible page
tft.bteMpuWriteWithROPData16(SCREEN_2, tft.width(), 0, 0,
                             SCREEN_2, tft.width(), 0, 0, 64, 64,
                             RA8876_BTE_ROP_CODE_12, sprite64);
tft.bteMemoryCopyWithChromaKey(SCREEN_2, tft.width(), 0, 0,
                               tft.currentPage, tft.width(), x, y, 64, 64, MAGENTA);
```

Register-level BTE setters (`bte_Source0_MemoryStartAddr`, `bte_WindowSize`, `bte_WindowAlpha`, …) are public for custom sequences.

---

## 13. Picture-in-picture (PIP)

Two hardware overlay windows (PIP1 above PIP2) show part of any SDRAM page over the main display, without redrawing.

```cpp
tft.selectScreen(SCREEN_4);   // draw PIP content on page 4
tft.fillScreen(DARKBLUE);
tft.selectScreen(SCREEN_1);   // back to the main page
tft.PIP(1, 1, SCREEN_4, 0, 0, tft.width(), 100, 100, 320, 240);   // enable PIP1
```

| Method | Description |
|--------|-------------|
| `PIP(onOff, pipNo, srcAddr, XP, YP, imageWidth, X_Dis, Y_Dis, W, H)` | One-call setup. `onOff`: 0 off, 1 on, 2 unchanged. `pipNo`: 1 or 2. `XP`, `YP`, `W`, `H` must be multiples of 4. |
| `Enable_PIP1()` / `Disable_PIP1()` / `Enable_PIP2()` / `Disable_PIP2()` | |
| `Select_PIP1_Parameter()` / `Select_PIP2_Parameter()` | Choose which PIP the next register calls affect. |
| `PIP_Display_Start_XY()`, `PIP_Image_Start_Address()`, `PIP_Image_Width()`, `PIP_Window_Image_Start_XY()`, `PIP_Window_Width_Height()` | Register-level control. |
| `Select_PIP1/2_Window_8bpp / 16bpp / 24bpp()` | PIP color depth. |

---

## 14. Graphic cursor and text cursor

### Graphic (mouse) cursor — hardware sprite, 4 slots of 32×32, 2 colors

| Method | Description |
|--------|-------------|
| `Graphic_cursor_initial()` | Initialise with built-in shapes. |
| `Upload_Graphic_Cursor(uint8_t n, uint8_t *data)` | Load a custom 32×32 2-bpp shape into slot 1–4. |
| `Select_Graphic_Cursor_1()` … `_4()` | Choose a slot. |
| `Set_Graphic_Cursor_Color_1(uint8_t)` / `_Color_2(uint8_t)` | 8-bit (RGB332) colors. |
| `Enable_Graphic_Cursor()` / `Disable_Graphic_Cursor()` | |
| `Graphic_Cursor_XY(x, y)` | Move. |
| `gCursorSet(enable, type, color1, color2)`, `gcursorxy(x, y)`, `GetGCursorX()`, `GetGCursorY()` | Convenience wrappers. |

### Text cursor (ROM-font mode)

`Enable_Text_Cursor()`, `Disable_Text_Cursor()`, `Enable_Text_Cursor_Blinking()`, `Disable_Text_Cursor_Blinking()`, `Blinking_Time_Frames(uint8_t frames)`, `Text_Cursor_H_V(w, h)`, `cursorInit()`, `setCursorMode()`, `setCursorType()`, `setCursorBlink()`.

---

## 15. Backlight and PWM

Many modules drive the backlight from an external pin: define `BACKLITE` and use `digitalWrite`/`analogWrite`. To use the RA8876's own PWM outputs:

| Method | Description |
|--------|-------------|
| `backlight(boolean on)` | Enable the default internal PWM backlight. |
| `pwm_Prescaler(uint8_t)` | Core clock prescaler. |
| `pwm_ClockMuxReg(pwm1_div, pwm0_div, xpwm1_ctrl, xpwm0_ctrl)` | Clock dividers and pin function. |
| `pwm_Configuration(pwm1_inv, pwm1_reload, pwm1_start, pwm0_dead, pwm0_inv, pwm0_reload, pwm0_start)` | Start/stop and polarity. |
| `pwm0_ClocksPerPeriod(uint16_t)`, `pwm0_Duty(uint16_t)` | PWM0 period/duty. |
| `pwm1_ClocksPerPeriod(uint16_t)`, `pwm1_Duty(uint16_t)` | PWM1 period/duty (usual backlight channel). |

---

## 16. Touch

- **GT9271** (ER-TFTM101-1 capacitive panel, I²C 0x5D): not handled by this library. Use [bb_captouch](https://github.com/bitbank2/bb_captouch); see `examples/touch_GT9271`.
- **FT5206**: define `USE_FT5206_TOUCH` to enable `touched()`, `getTouches()`, `getTScoordinates()`, `getGesture()`, `useCapINT()`, `enableCapISR()`, `setWireObject()`, `setTouchLimit()`.

---

## 17. Low-level register access and diagnostics

| Method | Description |
|--------|-------------|
| `lcdRegWrite(ru8 reg, bool finalize = true)` | Select a register. |
| `lcdDataWrite(ru8 data, bool finalize = true)` | Write data. |
| `lcdRegDataWrite(ru8 reg, ru8 data, bool finalize = true)` | Register + data. |
| `ru8 lcdRegDataRead(ru8 reg, bool finalize = true)` | Read a register. |
| `ru8 lcdDataRead()`, `ru16 lcdDataRead16()`, `ru8 lcdStatusRead()` | Raw reads. |
| `lcdDataWrite16bbp(ru16 data, bool finalize = true)` | One RGB565 pixel to the memory port. |
| `vmemReadData/16(addr)`, `vmemWriteData/16(addr, v)` | Direct SDRAM access. |
| `Memory_Select_SDRAM()`, `Memory_Select_CGRAM()`, `Memory_Select_Graphic_Cursor_RAM()`, `Memory_XY_Mode()`, `Memory_Linear_Mode()` | Memory port targets. |
| `boolean checkIcReady()`, `boolean checkSdramReady()` | Status. |
| `check2dBusy()` | Wait for the 2D engine/BTE to finish. |
| `checkWriteFifoNotFull()`, `checkWriteFifoEmpty()`, `checkReadFifo*()` | FIFO waits. |
| `ru8 powerSavingStatus()` | |
| `setSerialFlash4BytesMode()`, `dma_24bitAddressBlockMode()`, `dma_32bitAddressBlockMode()` | Load images from serial flash attached to the RA8876. |
| `lcdHorizontalWidthVerticalHeight()`, `lcdHsync*()`, `lcdVsync*()`, … | Panel timing. |

Set `finalize = false` to keep the SPI transaction open across several calls.

---

## 18. Notes and limitations

- **RP2040 only, SPI1 only.** The pins passed to the constructor must be SPI1-capable.
- **SPI clock.** 30 MHz is the verified default; 47 MHz is possible with very short wires but documented as unstable.
- **Serial diagnostics.** `begin()` prints several lines to `Serial`.
- **Wait for the engine.** Hardware operations run asynchronously; call `check2dBusy()` before reading pixels, switching pages, or relying on a finished BTE result (most library functions already do this).
- **Status line.** `printStatusLine` writes in the bottom 24 px. Reserve it with `setTMargins(0,0,0,1)` if you also scroll text.
- **`fillScreen` respects margins** — after `setMargins`/`setTMargins` it fills only the text area.
- **No MCU-side sprite class.** Use spare SDRAM pages and the BTE instead.
