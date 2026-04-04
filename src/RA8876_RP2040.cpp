//**************************************************************//
// Raspberry Pi Pico RP2040 SPI Support
// Original code for Teensy, adapted for Pico with EarlePhilhower core
//**************************************************************//

#include "RA8876_RP2040.h"
#include "Arduino.h"
#include "SPI.h"

//**************************************************************//
// RA8876_RP2040()
//**************************************************************//
// Create RA8876 driver instance
RA8876_RP2040::RA8876_RP2040(const uint8_t CSp, const uint8_t RSTp, const uint8_t mosi_pin, const uint8_t sclk_pin, const uint8_t miso_pin) {
    _mosi = mosi_pin;
    _miso = miso_pin;
    _sclk = sclk_pin;
    _cs = CSp;
    _rst = RSTp;
    RA8876_GFX(8);  // Assumes 8 bit 
}

//**************************************************************//
// Ra8876_begin()
//**************************************************************//
boolean RA8876_RP2040::begin(uint32_t spi_clock) {
    // Initialize for Raspberry Pi Pico with EarlePhilhower core
    // Using SPI1 (secondary SPI bus)
    // SPI1 pins: MOSI=GP11, MISO=GP8, SCK=GP10
    _SPI_CLOCK = spi_clock;
    _pspi = &SPI1;
    _spi_num = 1;

    Serial.printf("Initializing SPI1 at %lu Hz\n", spi_clock);
    Serial.printf("MOSI: GPIO%d, MISO: GPIO%d, SCK: GPIO%d\n", _mosi, _miso, _sclk);
    Serial.printf("CS: GPIO%d, RST: GPIO%d\n", _cs, _rst);
    
    // Configure SPI1 pins directly on the concrete type before assigning the pointer.
    // setTX/setRX/setSCK are on SPIClassRP2040, not the base SPIClass, so they
    // cannot be called through _pspi.  CS is managed in software via startSend()/endSend().
    SPI1.setTX(_mosi);
    SPI1.setRX(_miso);
    SPI1.setSCK(_sclk);
    _pspi = &SPI1;

    Serial.println("SPI pins configured");

    // Start SPI
    _pspi->begin();
    Serial.println("SPI1.begin() called successfully");
    delay(10);

    // Initialize CS pin (software control)
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
    Serial.println("CS pin initialized (HIGH)");

    // toggle RST low to reset
    if (_rst < 255) {
        pinMode(_rst, OUTPUT);
        Serial.println("Starting reset sequence...");
        digitalWrite(_rst, HIGH);
        delay(10);
        digitalWrite(_rst, LOW);
        Serial.println("RST pulled LOW");
        delay(50);
        digitalWrite(_rst, HIGH);
        Serial.println("RST released (HIGH)");
        delay(250);  // Increased delay for stability
        Serial.println("Reset sequence complete");
    }

    // Test SPI communication with debug
    Serial.println("Attempting to read status register...");
    ru8 status = lcdStatusRead();
    Serial.printf("First status read: 0x%02X\n", status);
    
    Serial.println("Checking IC ready...");
    if (!checkIcReady()) {
        Serial.println("ERROR: IC ready check failed!");
        Serial.println("Possible issues:");
        Serial.println("  1. SPI wiring incorrect (MOSI/MISO/SCK/CS)");
        Serial.println("  2. Display not powered (check 3V3 and GND)");
        Serial.println("  3. Reset pin not connected properly");
        Serial.println("  4. SPI clock too fast - try lower speed");
        return false;
    }
    Serial.println("IC ready confirmed");
    
    // read ID code must disable pll, 01h bit7 set 0
    lcdRegDataWrite(0x01, 0x08);
    delay(1);
    ru8 id1 = lcdRegDataRead(0xff);
    Serial.printf("Display ID: 0x%02X (should be 0x76 or 0x77)\n", id1);
    
    if ((id1 != 0x76) && (id1 != 0x77)) {
        Serial.printf("ERROR: Invalid display ID: 0x%02X\n", id1);
        return false;
    }

    // Initialize RA8876 to default settings
    Serial.println("Initializing RA8876 display...");
    if (!ra8876Initialize()) {
        Serial.println("ERROR: RA8876 initialization failed!");
        return false;
    }

    Serial.println("Display initialization complete!");
    // return success
    return true;
}
//**************************************************************//
// Write to a RA8876 register
//**************************************************************//
void RA8876_RP2040::lcdRegWrite(ru8 reg, bool finalize) {
    ru16 _data = (RA8876_SPI_CMDWRITE16 | reg);

    startSend();
    _pspi->transfer16(_data);
    endSend(finalize);
}

void RA8876_RP2040::LCD_CmdWrite(unsigned char cmd) {
    startSend();
    _pspi->transfer16(0x00);
    _pspi->transfer(cmd);
    endSend(true);
}

//**************************************************************//
// Write RA8876 Data
//**************************************************************//
void RA8876_RP2040::lcdDataWrite(ru8 data, bool finalize) {
    ru16 _data = (RA8876_SPI_DATAWRITE16 | data);
    startSend();
    _pspi->transfer16(_data);
    endSend(finalize);
}

//**************************************************************//
// Read RA8876 Data
//**************************************************************//
ru8 RA8876_RP2040::lcdDataRead(bool finalize) {
    ru16 _data = (RA8876_SPI_DATAREAD16 | 0x00);

    startSend();
    ru8 data = _pspi->transfer16(_data);
    endSend(finalize);
    return data;
}

//**************************************************************//
// Read RA8876 Data 16-bit
//**************************************************************//
ru16 RA8876_RP2040::lcdDataRead16(bool finalize) {
    ru16 _data = (RA8876_SPI_DATAREAD16 | 0x00);

    startSend();
    ru16 data = _pspi->transfer16(_data);
    endSend(finalize);
    return data;
}

//**************************************************************//
// Read RA8876 status register
//**************************************************************//
ru8 RA8876_RP2040::lcdStatusRead(bool finalize) {
    startSend();
    ru8 data = _pspi->transfer16(RA8876_SPI_STATUSREAD16);
    endSend(finalize);
    return data;
}

//**************************************************************//
// Write Data to a RA8876 register
//**************************************************************//
void RA8876_RP2040::lcdRegDataWrite(ru8 reg, ru8 data, bool finalize) {
    // write the register we wish to write to, then send the data
    // don't need to release _CS between the two transfers
    // ru16 _reg = (RA8876_SPI_CMDWRITE16 | reg);
    // ru16 _data = (RA8876_SPI_DATAWRITE16 | data);
    uint8_t buf[4] = {RA8876_SPI_CMDWRITE, reg, RA8876_SPI_DATAWRITE, data};
    startSend();
    //_pspi->transfer16(_reg);
    //_pspi->transfer16(_data);
    _pspi->transfer(buf, nullptr, 4);
    endSend(finalize);
}

//**************************************************************//
// Read a RA8876 register Data
//**************************************************************//
ru8 RA8876_RP2040::lcdRegDataRead(ru8 reg, bool finalize) {
    lcdRegWrite(reg, finalize);
    return lcdDataRead();
}

//**************************************************************//
// support SPI interface to write 16bpp data after Regwrite 04h
//**************************************************************//
void RA8876_RP2040::lcdDataWrite16bbp(ru16 data, bool finalize) {
    startSend();
    _pspi->transfer(RA8876_SPI_DATAWRITE);
    _pspi->transfer16(data);
    endSend(finalize);
}

//**************************************************************//
// Send data from the microcontroller to the RA8876
// Does a Raster OPeration to combine with an image already in memory
// For a simple overwrite operation, use ROP 12
//**************************************************************//
void RA8876_RP2040::bteMpuWriteWithROPData8(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                                        ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code, const unsigned char *data) {
    bteMpuWriteWithROP(s1_addr, s1_image_width, s1_x, s1_y, des_addr, des_image_width, des_x, des_y, width, height, rop_code);

    startSend();
    _pspi->transfer(RA8876_SPI_DATAWRITE);

    // If you try _pspi->transfer(data, length) then this tries to write received data into the data buffer
    // but if we were given a PROGMEM (unwriteable) data pointer then _pspi->transfer will lock up totally.
    // So we explicitly tell it we don't care about any return data.
    _pspi->transfer(data, NULL, width * height * 2);
    endSend(true);
}
//**************************************************************//
// For 16-bit byte-reversed data.
// Note this is 4-5 milliseconds slower than the 8-bit version above
// as the bulk byte-reversing SPI transfer operation is not available
// on all Teensys.
//**************************************************************//
void RA8876_RP2040::bteMpuWriteWithROPData16(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                                         ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code, const unsigned short *data) {
    ru16 i, j;
    bteMpuWriteWithROP(s1_addr, s1_image_width, s1_x, s1_y, des_addr, des_image_width, des_x, des_y, width, height, rop_code);

    startSend();
    _pspi->transfer(RA8876_SPI_DATAWRITE);

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            _pspi->transfer16(*data);
            data++;
        }
    }

    endSend(true);
}

//==========================================================================================

//**************************************************************//
/* Write 16bpp(RGB565) picture data for user operation          */
/* Not recommended for future use - use BTE instead             */
//**************************************************************//
void RA8876_RP2040::putPicture_16bpp(ru16 x, ru16 y, ru16 width, ru16 height) {
    graphicMode(true);
    activeWindowXY(x, y);
    activeWindowWH(width, height);
    setPixelCursor(x, y);
    ramAccessPrepare();
    // Now your program has to send the image data pixels
}

//*******************************************************************//
/* write 16bpp(RGB565) picture data in byte format from data pointer */
/* Not recommended for future use - use BTE instead                  */
//*******************************************************************//
void RA8876_RP2040::putPicture_16bppData8(ru16 x, ru16 y, ru16 width, ru16 height, const unsigned char *data) {
    ru16 i, j;
    graphicMode(true);
    activeWindowXY(x, y);
    activeWindowWH(width, height);
    setPixelCursor(x, y);
    ramAccessPrepare();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            // checkWriteFifoNotFull();  //if high speed mcu and without Xnwait check
            lcdDataWrite(*data);
            data++;
            // checkWriteFifoNotFull();  //if high speed mcu and without Xnwait check
            lcdDataWrite(*data);
            data++;
        }
    }
    checkWriteFifoEmpty(); // if high speed mcu and without Xnwait check
    activeWindowXY(0, 0);
    activeWindowWH(_width, _height);
}

//****************************************************************//
/* Write 16bpp(RGB565) picture data word format from data pointer */
/* Not recommended for future use - use BTE instead               */
//****************************************************************//
void RA8876_RP2040::putPicture_16bppData16(ru16 x, ru16 y, ru16 width, ru16 height, const unsigned short *data) {
    ru16 i, j;
    putPicture_16bpp(x, y, width, height);
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            // checkWriteFifoNotFull();//if high speed mcu and without Xnwait check
            lcdDataWrite16bbp(*data);
            data++;
            // checkWriteFifoEmpty();//if high speed mcu and without Xnwait check
        }
    }
    checkWriteFifoEmpty(); // if high speed mcu and without Xnwait check
    activeWindowXY(0, 0);
    activeWindowWH(_width, _height);
}

//**************************************************************//
// write data after setting, using lcdDataWrite() or lcdDataWrite16bbp()
//**************************************************************//
void RA8876_RP2040::bteMpuWriteWithROP(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width, ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code) {
    check2dBusy();
    graphicMode(true);
    bte_Source1_MemoryStartAddr(s1_addr);
    bte_Source1_ImageWidth(s1_image_width);
    bte_Source1_WindowStartXY(s1_x, s1_y);
    bte_DestinationMemoryStartAddr(des_addr);
    bte_DestinationImageWidth(des_image_width);
    bte_DestinationWindowStartXY(des_x, des_y);
    bte_WindowSize(width, height);
    lcdRegDataWrite(RA8876_BTE_CTRL1, rop_code << 4 | RA8876_BTE_MPU_WRITE_WITH_ROP);                                                             // 91h
    lcdRegDataWrite(RA8876_BTE_COLR, RA8876_S0_COLOR_DEPTH_16BPP << 5 | RA8876_S1_COLOR_DEPTH_16BPP << 2 | RA8876_DESTINATION_COLOR_DEPTH_16BPP); // 92h
    lcdRegDataWrite(RA8876_BTE_CTRL0, RA8876_BTE_ENABLE << 4);                                                                                    // 90h
    ramAccessPrepare();
}

//**************************************************************//
// Send data from the microcontroller to the RA8876
// Does a chromakey (transparent color) to combine with the image already in memory
//**************************************************************//
void RA8876_RP2040::bteMpuWriteWithChromaKeyData8(ru32 des_addr, ru16 des_image_width, ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru16 chromakey_color, const unsigned char *data) {
    bteMpuWriteWithChromaKey(des_addr, des_image_width, des_x, des_y, width, height, chromakey_color);

    startSend();
    _pspi->transfer(RA8876_SPI_DATAWRITE);

    _pspi->transfer(data, NULL, width * height * 2);
    endSend(true);
}
//**************************************************************//
// Chromakey for 16-bit byte-reversed data. (Slower than 8-bit.)
//**************************************************************//
void RA8876_RP2040::bteMpuWriteWithChromaKeyData16(ru32 des_addr, ru16 des_image_width, ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru16 chromakey_color, const unsigned short *data) {
    ru16 i, j;
    bteMpuWriteWithChromaKey(des_addr, des_image_width, des_x, des_y, width, height, chromakey_color);

    startSend();
    _pspi->transfer(RA8876_SPI_DATAWRITE);
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            _pspi->transfer16(*data);
            data++;
        }
    }
    endSend(true);
}
