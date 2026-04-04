// graphicCursor.ino
//
// Demonstrates the RA8876 hardware graphic cursor API on the Raspberry Pi Pico.
// All four built-in cursor types (pen, arrow, hourglass, error/stop) are
// cycled through every 3 seconds.
//
// Instead of a USB mouse, the cursor moves in a smooth bouncing pattern
// (DVD screensaver style) using x/y velocity variables updated in loop().
// Cursor position and active cursor number are printed on the display.

#include "RA8876_Config_SPI.h"
#include <SPI.h>
#include <RA8876_RP2040.h>

RA8876_RP2040 tft = RA8876_RP2040(RA8876_CS, RA8876_RESET, RA8876_MOSI, RA8876_SCLK, RA8876_MISO);

// Screen bounds
static int16_t SCREEN_W = 1024;
static int16_t SCREEN_H = 600;

// Bouncing cursor state
float cursorX = 512.0f;
float cursorY = 300.0f;
float velX    =  3.7f;  // pixels per loop iteration
float velY    =  2.3f;

// Cursor cycling
int currentCursor = 1;           // 1..4
uint32_t lastCursorSwap = 0;
const uint32_t CURSOR_CYCLE_MS = 3000;

// Display info update rate
uint32_t lastInfoUpdate = 0;
const uint32_t INFO_UPDATE_MS = 100;

void selectCursor(int n) {
  switch (n) {
    case 1: tft.Select_Graphic_Cursor_1(); break; // Pen
    case 2: tft.Select_Graphic_Cursor_2(); break; // Arrow
    case 3: tft.Select_Graphic_Cursor_3(); break; // Hourglass
    case 4: tft.Select_Graphic_Cursor_4(); break; // Error/Stop
  }
}

const char* cursorName(int n) {
  switch (n) {
    case 1: return "Pen      ";
    case 2: return "Arrow    ";
    case 3: return "Hourglass";
    case 4: return "Error/Stop";
  }
  return "?";
}

void setup() {
  //I'm guessing most copies of this display are using external PWM
  //backlight control instead of the internal RA8876 PWM.
  //Connect a Raspberry Pi Pico pin to pin 14 on the display.
  //Can use analogWrite() but I suggest you increase the PWM frequency first so it doesn't sing.
#if defined(BACKLITE) // Defined in RA8876_Config.h.
  pinMode(BACKLITE, OUTPUT);
  digitalWrite(BACKLITE, HIGH);
#endif

  Serial.begin(115200);
  while (!Serial && millis() < 1000) {}

  Serial.println("RA8876 Graphic Cursor Demo (bouncing)");

#if defined(USE_SPI_47000000)
  tft.begin(47000000);
#else
  tft.begin(30000000);
#endif

  SCREEN_W = tft.width();
  SCREEN_H = tft.height();

  tft.fillScreen(DARKBLUE);
  tft.setFontSize(1, false);
  tft.setCursor(0, 0);
  tft.setTextColor(YELLOW, DARKBLUE);
  tft.println("RA8876 Graphic Cursor Demo");
  tft.setTextColor(WHITE, DARKBLUE);
  tft.println("Cursor bounces like a DVD screensaver.");
  tft.println("Cycles through all 4 cursor types every 3 seconds.");

  // Draw a white rectangle the cursor bounces inside so it is visible
  tft.drawRect(0, 0, SCREEN_W, SCREEN_H - STATUS_LINE_HEIGHT, WHITE);

  // Initialize all 4 built-in cursor images
  tft.Graphic_cursor_initial();

  // Start with cursor 1 (Pen)
  selectCursor(currentCursor);
  tft.Enable_Graphic_Cursor();
  tft.Set_Graphic_Cursor_Color_1(0xff); // White foreground
  tft.Set_Graphic_Cursor_Color_2(0x00); // Black outline
  tft.Graphic_Cursor_XY((int16_t)cursorX, (int16_t)cursorY);

  lastCursorSwap = millis();
  lastInfoUpdate = millis();
}

void loop() {
  uint32_t now = millis();

  // --- Move cursor ---
  cursorX += velX;
  cursorY += velY;

  // Bounce off edges
  if (cursorX < 1) {
    cursorX = 1;
    velX = -velX;
  }
  if (cursorX > (float)(SCREEN_W - 2)) {
    cursorX = (float)(SCREEN_W - 2);
    velX = -velX;
  }
  if (cursorY < 1) {
    cursorY = 1;
    velY = -velY;
  }
  if (cursorY > (float)(SCREEN_H - STATUS_LINE_HEIGHT - 2)) {
    cursorY = (float)(SCREEN_H - STATUS_LINE_HEIGHT - 2);
    velY = -velY;
  }

  tft.Graphic_Cursor_XY((int16_t)cursorX, (int16_t)cursorY);

  // --- Cycle cursor type every 3 seconds ---
  if (now - lastCursorSwap >= CURSOR_CYCLE_MS) {
    lastCursorSwap = now;
    currentCursor++;
    if (currentCursor > 4) currentCursor = 1;
    selectCursor(currentCursor);
  }

  // --- Update on-screen info every 100ms ---
  if (now - lastInfoUpdate >= INFO_UPDATE_MS) {
    lastInfoUpdate = now;
    tft.setFontSize(1, false);
    tft.setTextColor(YELLOW, DARKBLUE);
    tft.textxy(10, 80);
    tft.printf("Cursor X: %4d  ", (int)cursorX);
    tft.textxy(10, 96);
    tft.printf("Cursor Y: %4d  ", (int)cursorY);
    tft.textxy(10, 112);
    tft.printf("Cursor #%d: %s  ", currentCursor, cursorName(currentCursor));
  }

  delay(10);
}
