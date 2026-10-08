//**************************************************************//
// Raspberry Pi Pico RP2040 SPI Support via EarlePhilhower core
//**************************************************************//
/*
 * RA8876_RP2040.h - Modified for Raspberry Pi Pico RP2040
 * Original Version for Teensy 3.x and T4
 * By Warren Watson and others as noted below
 * Adapted for Pico 2024
 *****************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
//**************************************************************//
/*File Name : tft.h
 *          : For Teensy 3.x and T4
 *          : By Warren Watson
 *          : 06/07/2018 - 11/31/2019
 *          : Copyright (c) 2017-2019 Warren Watson.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/***************************************************************
 *  Modified by mjs513 and KurtE and MorganS
 *  Combined libraries and added functions to be compatible with
 *  other display libraries
 *  See PJRC Forum Thread: https://forum.pjrc.com/threads/58565-RA8876LiteTeensy-For-Teensy-T36-and-T40/page5
 *
 ***************************************************************/
#ifndef _RA8876_RP2040_P
#define _RA8876_RP2040_P

#include "Arduino.h"
#include "RA8876Registers.h"
#include "RA8876_common.h"

#include "SPI.h"
// Default to conservative speed for Pico
// const ru32 SPIspeed = 47000000;
// const ru32 SPIspeed = 3000000;
const ru32 SPIspeed = 30000000;  // 30 MHz verified stable on Pico

class RA8876_RP2040 : public RA8876_common {
  public:
    RA8876_RP2040(const uint8_t CSp = 5, const uint8_t RSTp = 6, const uint8_t mosi_pin = 3, const uint8_t sclk_pin = 2, const uint8_t miso_pin = 4);

    boolean begin(uint32_t spi_clock = SPIspeed);

    /*access*/
    void lcdRegWrite(ru8 reg, bool finalize = true);
    void lcdDataWrite(ru8 data, bool finalize = true);
    ru8 lcdDataRead(bool finalize = true);
    ru16 lcdDataRead16(bool finalize = true);
    ru16 lcdDataRead16bpp(bool finalize = true);
    ru8 lcdStatusRead(bool finalize = true);
    void lcdRegDataWrite(ru8 reg, ru8 data, bool finalize = true);
    ru8 lcdRegDataRead(ru8 reg, bool finalize = true);
    void lcdDataWrite16bbp(ru16 data, bool finalize = true);

    /*BTE function*/
    void bteMpuWriteWithROPData8(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                                 ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code, const unsigned char *data);
    void bteMpuWriteWithROPData16(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                                  ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code, const unsigned short *data);
    void bteMpuWriteWithROP(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                            ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code);

    void bteMpuWriteWithChromaKeyData8(ru32 des_addr, ru16 des_image_width, ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru16 chromakey_color,
                                       const unsigned char *data);

    void bteMpuWriteWithChromaKeyData16(ru32 des_addr, ru16 des_image_width, ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru16 chromakey_color,
                                        const unsigned short *data);

    /*  Picture Functions */
    void putPicture_16bpp(ru16 x, ru16 y, ru16 width, ru16 height);
    void putPicture_16bppData8(ru16 x, ru16 y, ru16 width, ru16 height, const unsigned char *data);
    void putPicture_16bppData16(ru16 x, ru16 y, ru16 width, ru16 height, const unsigned short *data);

    // SPI Functions
    inline __attribute__((always_inline)) void startSend() {
        if (!RA8876_BUSY) {
            RA8876_BUSY = true;
            _pspi->beginTransaction(SPISettings(_SPI_CLOCK, MSBFIRST, SPI_MODE3));
        }
        digitalWrite(_cs, LOW);
    }

    inline __attribute__((always_inline)) void endSend(bool finalize) {
        digitalWrite(_cs, HIGH);
        if (finalize) {
            _pspi->endTransaction();
            RA8876_BUSY = false;
        }
    }

    void LCD_CmdWrite(unsigned char cmd);

  private:
    int _mosi;
    int _miso;
    int _sclk;
    int _cs;
    int _rst;
    int _errorCode;

    uint8_t _spi_num;         // Which SPI bus (0 or 1 on Pico)
    uint32_t _SPI_CLOCK;      // SPI clock speed
    uint32_t _SPI_CLOCK_READ; // SPI read clock speed

  protected:
    // Exposed to derived classes for bulk pixel transfers (e.g. TT_Display::drawRGB565)
    SPIClass *_pspi = nullptr;
};

#endif
