/*
  touch_GT9271.ino

  Capacitive touch demo for RA8876 display with GT9271 touch controller.
  Designed for ER-TFTM101-1 or similar panels using the GT9271 over I2C.

  Hardware connections (Raspberry Pi Pico):
    Display SPI:   see RA8876_Config_SPI.h
    Touch I2C SDA: GP12
    Touch I2C SCL: GP13
    Touch INT:     (not used here; polled via bb_captouch)

  Uses the bb_captouch library (BBCapTouch) to read GT9271 touch data.

  Behavior:
    - On touch:    draws a filled circle at the touch coordinates;
                   moves the graphic cursor to the touch position.
    - No touch:    animates the custom spinning cursor through 8 orientations
                   (cycles every 100 ms).
*/

#include "RA8876_Config_SPI.h"
#include <SPI.h>
#include <RA8876_RP2040.h>
#include <Wire.h>
#include "my_bb_captouch.h"

// GT9271 I2C pins on the Raspberry Pi Pico
#define GT9271_SDA 12
#define GT9271_SCL 13

// GT9271 I2C address (can be 0x5D or 0x14 depending on INT pin state at boot)
#define GT9271_ADDR 0x5D

RA8876_RP2040 tft = RA8876_RP2040(RA8876_CS, RA8876_RESET, RA8876_MOSI, RA8876_SCLK, RA8876_MISO);
BBCapTouch bbct;

// my_bb_captouch.h declares this as extern; we must define it here
TwoWire* myWire = &Wire;

int currentCursor = 4;  // start with custom cursor in slot 4
int cursorOffsetX = 15;
int cursorOffsetY = 15;

// Custom cursor: spinning circle, 8-frame animation cycle
// Each frame is produced by rotateCursor(rotation) applied to this base shape.
PROGMEM unsigned char customCursor[256] = {
  0b10101010, 0b10101010, 0b10101010, 0b10010101, 0b01011010, 0b10101010, 0b10101010, 0b10101010,
  0b10101010, 0b10101010, 0b10010101, 0b01000000, 0b00011010, 0b10101010, 0b10101010, 0b10101010,
  0b10101010, 0b10101010, 0b01010000, 0b00000000, 0b00011010, 0b10101010, 0b10101010, 0b10101010,
  0b10101010, 0b10100101, 0b01000000, 0b00000000, 0b00011010, 0b10101010, 0b10101010, 0b10101010,
  0b10101010, 0b10010100, 0b00000000, 0b00000101, 0b01011010, 0b10101010, 0b10100110, 0b10101010,
  0b10101010, 0b01010000, 0b00000001, 0b01011010, 0b10101010, 0b10101010, 0b10010101, 0b10101010,
  0b10101001, 0b01000000, 0b00010101, 0b10101010, 0b10101010, 0b10101010, 0b01000001, 0b01101010,
  0b10101001, 0b00000000, 0b01011010, 0b10101010, 0b10101010, 0b10101001, 0b00000000, 0b01101010,
  0b10100101, 0b00000001, 0b01101010, 0b10101010, 0b10101010, 0b10101001, 0b01000000, 0b01011010,
  0b10010100, 0b00000110, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b01010000, 0b00010110,
  0b10010000, 0b00000110, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10010000, 0b00000110,
  0b10010000, 0b00010110, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10010100, 0b00000110,
  0b10010000, 0b00011010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10100100, 0b00000110,
  0b01000000, 0b01011010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10100101, 0b00000001,
  0b01000000, 0b01101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101001, 0b00000001,
  0b01000000, 0b01101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101001, 0b00000001,
  0b01000000, 0b01101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101001, 0b00000001,
  0b01000000, 0b01101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101001, 0b00000001,
  0b01000000, 0b01011010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10100101, 0b00000001,
  0b10010000, 0b00011010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10100100, 0b00000110,
  0b10010000, 0b00010110, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10010100, 0b00000110,
  0b10010000, 0b00000110, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10010000, 0b00000110,
  0b10010100, 0b00000110, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b01010000, 0b00010110,
  0b10100101, 0b00000001, 0b01101010, 0b10101010, 0b10101010, 0b10101001, 0b01000000, 0b01011010,
  0b10101001, 0b00000000, 0b01011010, 0b10101010, 0b10101010, 0b10100101, 0b00000000, 0b01101010,
  0b10101001, 0b01000000, 0b00010101, 0b10101010, 0b10101010, 0b01010100, 0b00000001, 0b01101010,
  0b10101010, 0b01010000, 0b00000001, 0b01011010, 0b10100101, 0b01000000, 0b00000101, 0b10101010,
  0b10101010, 0b10010100, 0b00000000, 0b00000101, 0b01010100, 0b00000000, 0b00010110, 0b10101010,
  0b10101010, 0b10100101, 0b01000000, 0b00000000, 0b00000000, 0b00000001, 0b01011010, 0b10101010,
  0b10101010, 0b10101010, 0b01010000, 0b00000000, 0b00000000, 0b00000101, 0b10101010, 0b10101010,
  0b10101010, 0b10101010, 0b10010101, 0b01000000, 0b00000001, 0b01010110, 0b10101010, 0b10101010,
  0b10101010, 0b10101010, 0b10101010, 0b10010101, 0b01010110, 0b10101010, 0b10101010, 0b10101010,
};

// -----------------------------------------------------------------------
// Cursor rotation helpers (from original touch_FT5316_RA8876 example)
// -----------------------------------------------------------------------

unsigned char reverseByte(unsigned char b) {
  return (b & 0b00000011) << 6 | (b & 0b00001100) << 2 | (b & 0b00110000) >> 2 | (b & 0b11000000) >> 6;
}

unsigned char rotateByte(unsigned char input[], int in, int column) {
  switch (column) {
    case 3:
      return (input[in] & 0b00000011) | (input[in - 8] & 0b00000011) << 2 | (input[in - 16] & 0b00000011) << 4 | (input[in - 24] & 0b00000011) << 6;
    case 2:
      return (input[in] & 0b00001100) >> 2 | (input[in - 8] & 0b00001100) | (input[in - 16] & 0b00001100) << 2 | (input[in - 24] & 0b00001100) << 4;
    case 1:
      return (input[in] & 0b00110000) >> 4 | (input[in - 8] & 0b00110000) >> 2 | (input[in - 16] & 0b00110000) | (input[in - 24] & 0b00110000) << 2;
    case 0:
      return (input[in] & 0b11000000) >> 6 | (input[in - 8] & 0b11000000) >> 4 | (input[in - 16] & 0b11000000) >> 2 | (input[in - 24] & 0b11000000);
  }
  return 0;
}

void rotateCursor(int rotation) {
  unsigned char data[256];
  int outx = 0;
  int outy = 0;
  int out = 0;
  int inx, iny, in;
  while (out < 256) {
    unsigned char tmp = 0;
    switch (rotation) {
      case 0:
        tmp = customCursor[out];
        break;
      case 1:
        inx = 7 - outy / 4;
        iny = 31 - outx * 4;
        in = iny * 8 + inx;
        tmp = reverseByte(rotateByte(customCursor, in, 3 - outy % 4));
        break;
      case 2:
        inx = outy / 4;
        iny = 31 - outx * 4;
        in = iny * 8 + inx;
        tmp = reverseByte(rotateByte(customCursor, in, outy % 4));
        break;
      case 3:
        inx = outx;
        iny = 31 - outy;
        in = iny * 8 + inx;
        tmp = customCursor[in];
        break;
      case 4:
        inx = 7 - outx;
        iny = 31 - outy;
        in = iny * 8 + inx;
        tmp = reverseByte(customCursor[in]);
        break;
      case 5:
        inx = outy / 4;
        iny = outx * 4 + 3;
        in = iny * 8 + inx;
        tmp = rotateByte(customCursor, in, outy % 4);
        break;
      case 6:
        inx = 7 - outy / 4;
        iny = outx * 4 + 3;
        in = iny * 8 + inx;
        tmp = rotateByte(customCursor, in, 3 - outy % 4);
        break;
      case 7:
        inx = 7 - outx;
        iny = outy;
        in = iny * 8 + inx;
        tmp = reverseByte(customCursor[in]);
        break;
      default:
        tmp = customCursor[out];
    }
    data[out] = tmp;
    outx++;
    if (outx >= 8) {
      outx = 0;
      outy++;
    }
    out++;
  }
  tft.Upload_Graphic_Cursor(4, data);
}

// -----------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  long unsigned debug_start = millis();
  while (!Serial && ((millis() - debug_start) <= 500)) ;
  Serial.println("RA8876 GT9271 touch test starting!");
  Serial.print("Compiled ");
  Serial.print(__DATE__);
  Serial.print(" at ");
  Serial.println(__TIME__);

  //I'm guessing most copies of this display are using external PWM
  //backlight control instead of the internal RA8876 PWM.
  //Connect a Raspberry Pi Pico pin to pin 14 on the display.
  //Can use analogWrite() but I suggest you increase the PWM frequency first so it doesn't sing.
#if defined(BACKLITE)
  pinMode(BACKLITE, OUTPUT);
  digitalWrite(BACKLITE, HIGH);
  tft.backlight(true);
#endif

#if defined(USE_SPI_47000000)
  tft.begin(47000000);
#else
  tft.begin(30000000);
#endif

  // Initialize GT9271 touch controller via bb_captouch.
  // init() configures Wire (setSDA/setSCL/begin) internally.
  // RST and INT are not wired on this board, so -1 is passed for both;
  // the library will skip the hardware reset sequence and detect the
  // chip's I2C address from its power-on state.
  int rc = bbct.init(GT9271_SDA, GT9271_SCL, -1, -1, 400000, &Wire);
  if (rc != CT_SUCCESS) {
    Serial.print("bb_captouch init failed, rc=");
    Serial.println(rc);
  } else {
    Serial.println("bb_captouch init OK");
    bbct.setOrientation(180, tft.width(), tft.height());
  }

  // Display setup instructions
  tft.fillScreen(BLACK);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setFontSize(1, false);
  tft.setCursor(0, 300);
  tft.println("GT9271 Touch Demo");
  tft.println("Touch screen to draw circles");
  tft.println("and move the cursor.");
  tft.println("Cursor spins when idle.");

  // Upload and enable the custom spinning cursor
  tft.Graphic_cursor_initial();
  tft.Upload_Graphic_Cursor(4, customCursor);
  tft.Select_Graphic_Cursor_4();
  currentCursor = 4;

  tft.Enable_Graphic_Cursor();
  tft.Set_Graphic_Cursor_Color_1(0xff); // White foreground (8-bit RRRGGBB)
  tft.Set_Graphic_Cursor_Color_2(0x00); // Black outline
  tft.Graphic_Cursor_XY(tft.width() / 2 - cursorOffsetX, tft.height() / 2 - cursorOffsetY);

  tft.fillRect(0, 0, 215, 215, ORCHID);
  tft.fillRect(400, 0, 172, 172, BLUEVIOLET);
}

// -----------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------

void loop() {
  TOUCHINFO ti;
  bool touched = (bbct.getSamples(&ti) > 0) && (ti.count > 0);

  if (touched) {
    // Use first touch point
    int tx = ti.x[0];
    int ty = ti.y[0];

    Serial.printf("Touch: (%d, %d)\n", tx, ty);

    // Draw a filled circle at touch point
    tft.fillCircle(tx, ty, 10, CYAN);

    // Move graphic cursor to touch position
    tft.Graphic_Cursor_XY(tx - cursorOffsetX, ty - cursorOffsetY);

  } else {
    // No touch: animate the spinning cursor through 8 orientations
    static unsigned long lastRotate = 0;
    static int rotation = 0;
    if (millis() - lastRotate > 100) {
      lastRotate = millis();
      rotateCursor(rotation);
      rotation++;
      if (rotation >= 8) rotation = 0;
    }
  }
}
