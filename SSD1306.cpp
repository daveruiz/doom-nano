/*!
 * @file Adafruit_SSD1306.cpp
 *
 * @mainpage Arduino library for monochrome OLEDs based on SSD1306 drivers.
 *
 * @section intro_sec Introduction
 *
 * This is documentation for Adafruit's SSD1306 library for monochrome
 * OLED displays: http://www.adafruit.com/category/63_98
 *
 * These displays use I2C or SPI to communicate. I2C requires 2 pins
 * (SCL+SDA) and optionally a RESET pin. SPI requires 4 pins (MOSI, SCK,
 * select, data/command) and optionally a reset pin. Hardware SPI or
 * 'bitbang' software SPI are both supported.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section dependencies Dependencies
 *
 * This library depends on <a href="https://github.com/adafruit/Adafruit-GFX-Library">
 * Adafruit_GFX</a> being present on your system. Please make sure you have
 * installed the latest version before using this library.
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries, with
 * contributions from the open source community.
 *
 * @section license License
 *
 * BSD license, all text above, and the splash screen included below,
 * must be included in any redistribution.
 *
 */

#ifdef __AVR__
 #include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
 #include <pgmspace.h>
#else
 #define pgm_read_byte(addr) \
  (*(const unsigned char *)(addr)) ///< PROGMEM workaround for non-AVR
#endif

#if !defined(__ARM_ARCH) && !defined(ENERGIA) && !defined(ESP8266) && !defined(ESP32) && !defined(__arc__)
 #include <util/delay.h>
#endif

#include "SSD1306.h"
#include <stdlib.h>     /* malloc, free, rand */

// SOME DEFINES AND STATIC VARIABLES USED INTERNALLY -----------------------

constexpr uint16_t WIRE_MAX = 256;


#define ssd1306_swap(a, b) \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation


// CONSTRUCTORS, DESTRUCTOR ------------------------------------------------

/*!
    @brief  Destructor for Adafruit_SSD1306 object.
*/
Adafruit_SSD1306::~Adafruit_SSD1306(void) {
  if(buffer) {
    free(buffer);
    buffer = NULL;
  }
}

// LOW-LEVEL UTILS ---------------------------------------------------------

// Issue single command to SSD1306, using I2C or hard/soft SPI as needed.
// Because command calls are often grouped, SPI transaction and selection
// must be started/ended in calling function for efficiency.
// This is a private function, not exposed (see ssd1306_command() instead).
void Adafruit_SSD1306::ssd1306_command1(uint8_t c) {
  uint8_t cmd = 0x00; // Co = 0, D/C = 0
  TWI_Start_Transceiver_With_Data(cmd, &c, 1);
}

// Issue list of commands to SSD1306, same rules as above re: transactions.
// This is a private function, not exposed.
void Adafruit_SSD1306::ssd1306_commandList(const uint8_t *c, uint8_t n) {
  uint8_t cmd = 0x00; // Co = 0, D/C = 0
  uint8_t tmp[n];

  uint8_t * tmpptr = static_cast<uint8_t *>(memcpy_P(tmp, c, n));
  TWI_Start_Transceiver_With_Data(cmd, tmpptr, n);
}

// ALLOCATE & INIT DISPLAY -------------------------------------------------

/*!
    @brief  Allocate RAM for image buffer, initialize peripherals and pins.
    @param  vcs
            VCC selection. Pass SSD1306_SWITCHCAPVCC to generate the display
            voltage (step up) from the 3.3V source, or SSD1306_EXTERNALVCC
            otherwise. Most situations with Adafruit SSD1306 breakouts will
            want SSD1306_SWITCHCAPVCC.
    @param  addr
            I2C address of corresponding SSD1306 display (or pass 0 to use
            default of 0x3C for 128x32 display, 0x3D for all others).
            SPI displays (hardware or software) do not use addresses, but
            this argument is still required (pass 0 or any value really,
            it will simply be ignored). Default if unspecified is 0.
    @param  reset
            If true, and if the reset pin passed to the constructor is
            valid, a hard reset will be performed before initializing the
            display. If using multiple SSD1306 displays on the same bus, and
            if they all share the same reset pin, you should only pass true
            on the first display being initialized, false on all others,
            else the already-initialized displays would be reset. Default if
            unspecified is true.
    @param  periphBegin
            If true, and if a hardware peripheral is being used (I2C or SPI,
            but not software SPI), call that peripheral's begin() function,
            else (false) it has already been done in one's sketch code.
            Cases where false might be used include multiple displays or
            other devices sharing a common bus, or situations on some
            platforms where a nonstandard begin() function is available
            (e.g. a TwoWire interface on non-default pins, as can be done
            on the ESP8266 and perhaps others).
    @return true on successful allocation/init, false otherwise.
            Well-behaved code should check the return value before
            proceeding.
    @note   MUST call this function before any drawing or updates!
*/
bool Adafruit_SSD1306::begin(uint8_t vcs, uint8_t addr) {
  if((!buffer) && !(buffer = (uint8_t *)malloc(WIDTH * ((HEIGHT + 7) / 8))))
    return false;

  clearDisplay();

  vccstate = vcs;

  // Setup pin directions
    // If I2C address is unspecified, use default
    // (0x3C for 32-pixel-tall displays, 0x3D for all others).
    i2caddr = addr ? addr : ((HEIGHT == 32) ? 0x3C : 0x3D);
    // TwoWire begin() function might be already performed by the calling
    // function if it has unusual circumstances (e.g. TWI variants that
    // can accept different SDA/SCL pins, or if two SSD1306 instances
    // with different addresses -- only a single begin() is needed).
  TWI_Master_Initialise();

  // Init sequence
  static const uint8_t PROGMEM init1[] = {
    SSD1306_DISPLAYOFF,                   // 0xAE
    SSD1306_SETDISPLAYCLOCKDIV,           // 0xD5
    0x80,                                 // the suggested ratio 0x80
    SSD1306_SETMULTIPLEX };               // 0xA8
  ssd1306_commandList(init1, sizeof(init1));
  ssd1306_command1(HEIGHT - 1);

  static const uint8_t PROGMEM init2[] = {
    SSD1306_SETDISPLAYOFFSET,             // 0xD3
    0x0,                                  // no offset
    SSD1306_SETSTARTLINE | 0x0,           // line #0
    SSD1306_CHARGEPUMP };                 // 0x8D
  ssd1306_commandList(init2, sizeof(init2));

  ssd1306_command1((vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0x14);

  static const uint8_t PROGMEM init3[] = {
    SSD1306_MEMORYMODE,                   // 0x20
    0x00,                                 // 0x0 act like ks0108
    SSD1306_SEGREMAP | 0x1,
    SSD1306_COMSCANDEC };
  ssd1306_commandList(init3, sizeof(init3));

  if((WIDTH == 128) && (HEIGHT == 32)) {
    static const uint8_t PROGMEM init4a[] = {
      SSD1306_SETCOMPINS,                 // 0xDA
      0x02,
      SSD1306_SETCONTRAST,                // 0x81
      0x8F };
    ssd1306_commandList(init4a, sizeof(init4a));
  } else if((WIDTH == 128) && (HEIGHT == 64)) {
    static const uint8_t PROGMEM init4b[] = {
      SSD1306_SETCOMPINS,                 // 0xDA
      0x12,
      SSD1306_SETCONTRAST };              // 0x81
    ssd1306_commandList(init4b, sizeof(init4b));
    ssd1306_command1((vccstate == SSD1306_EXTERNALVCC) ? 0x9F : 0xCF);
  } else if((WIDTH == 96) && (HEIGHT == 16)) {
    static const uint8_t PROGMEM init4c[] = {
      SSD1306_SETCOMPINS,                 // 0xDA
      0x2,    // ada x12
      SSD1306_SETCONTRAST };              // 0x81
    ssd1306_commandList(init4c, sizeof(init4c));
    ssd1306_command1((vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0xAF);
  } else {
    // Other screen varieties -- TBD
  }

  ssd1306_command1(SSD1306_SETPRECHARGE); // 0xd9
  ssd1306_command1((vccstate == SSD1306_EXTERNALVCC) ? 0x22 : 0xF1);
  static const uint8_t PROGMEM init5[] = {
    SSD1306_SETVCOMDETECT,               // 0xDB
    0x40,
    SSD1306_DISPLAYALLON_RESUME,         // 0xA4
    SSD1306_NORMALDISPLAY,               // 0xA6
    SSD1306_DEACTIVATE_SCROLL,
    SSD1306_DISPLAYON };                 // Main screen turn on
  ssd1306_commandList(init5, sizeof(init5));

  return true; // Success
}

// DRAWING FUNCTIONS -------------------------------------------------------

/*!
    @brief  Set/clear/invert a single pixel. This is also invoked by the
            Adafruit_GFX library in generating many higher-level graphics
            primitives.
    @param  x
            Column of display -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @param  color
            Pixel color, one of: SSD1306_BLACK, SSD1306_WHITE or SSD1306_INVERT.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void Adafruit_SSD1306::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT)) {
    // Pixel is in-bounds. Rotate coordinates if needed.
    switch(color) {
     case SSD1306_WHITE:   buffer[x + (y/8)*WIDTH] |=  (1 << (y&7)); break;
     case SSD1306_BLACK:   buffer[x + (y/8)*WIDTH] &= ~(1 << (y&7)); break;
     case SSD1306_INVERSE: buffer[x + (y/8)*WIDTH] ^=  (1 << (y&7)); break;
    }
  }
}

/*!
    @brief  Clear contents of display buffer (set all pixels to off).
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void Adafruit_SSD1306::clearDisplay(void) {
  memset(buffer, 0, WIDTH * ((HEIGHT + 7) / 8));
}

/*!
    @brief  Draw a vertical line. This is also invoked by the Adafruit_GFX
            library in generating many higher-level graphics primitives.
    @param  x
            Column of display -- 0 at left to (screen width -1) at right.
    @param  y
            Topmost row -- 0 at top to (screen height - 1) at bottom.
    @param  h
            Height of line, in pixels.
    @param  color
            Line color, one of: SSD1306_BLACK, SSD1306_WHITE or SSD1306_INVERT.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void Adafruit_SSD1306::drawFastVLine(
  int16_t x, int16_t y, int16_t h, uint16_t color) {
  drawFastVLineInternal(x, y, h, color);
}

void Adafruit_SSD1306::drawFastVLineInternal(
  int16_t x, int16_t __y, int16_t __h, uint16_t color) {

  if((x >= 0) && (x < WIDTH)) { // X coord in bounds?
    if(__y < 0) { // Clip top
      __h += __y;
      __y = 0;
    }
    if((__y + __h) > HEIGHT) { // Clip bottom
      __h = (HEIGHT - __y);
    }
    if(__h > 0) { // Proceed only if height is now positive
      // this display doesn't need ints for coordinates,
      // use local byte registers for faster juggling
      uint8_t  y = __y, h = __h;
      uint8_t *pBuf = &buffer[(y / 8) * WIDTH + x];

      // do the first partial byte, if necessary - this requires some masking
      uint8_t mod = (y & 7);
      if(mod) {
        // mask off the high n bits we want to set
        mod = 8 - mod;
        // note - lookup table results in a nearly 10% performance
        // improvement in fill* functions
        // uint8_t mask = ~(0xFF >> mod);
        static const uint8_t PROGMEM premask[8] =
          { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };
        uint8_t mask = pgm_read_byte(&premask[mod]);
        // adjust the mask if we're not going to reach the end of this byte
        if(h < mod) mask &= (0XFF >> (mod - h));

        switch(color) {
         case SSD1306_WHITE:   *pBuf |=  mask; break;
         case SSD1306_BLACK:   *pBuf &= ~mask; break;
         case SSD1306_INVERSE: *pBuf ^=  mask; break;
        }
        pBuf += WIDTH;
      }

      if(h >= mod) { // More to go?
        h -= mod;
        // Write solid bytes while we can - effectively 8 rows at a time
        if(h >= 8) {
          if(color == SSD1306_INVERSE) {
            // separate copy of the code so we don't impact performance of
            // black/white write version with an extra comparison per loop
            do {
              *pBuf ^= 0xFF;  // Invert byte
              pBuf  += WIDTH; // Advance pointer 8 rows
              h     -= 8;     // Subtract 8 rows from height
            } while(h >= 8);
          } else {
            // store a local value to work with
            uint8_t val = (color != SSD1306_BLACK) ? 255 : 0;
            do {
              *pBuf = val;    // Set byte
              pBuf += WIDTH;  // Advance pointer 8 rows
              h    -= 8;      // Subtract 8 rows from height
            } while(h >= 8);
          }
        }

        if(h) { // Do the final partial byte, if necessary
          mod = h & 7;
          // this time we want to mask the low bits of the byte,
          // vs the high bits we did above
          // uint8_t mask = (1 << mod) - 1;
          // note - lookup table results in a nearly 10% performance
          // improvement in fill* functions
          static const uint8_t PROGMEM postmask[8] =
            { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };
          uint8_t mask = pgm_read_byte(&postmask[mod]);
          switch(color) {
           case SSD1306_WHITE:   *pBuf |=  mask; break;
           case SSD1306_BLACK:   *pBuf &= ~mask; break;
           case SSD1306_INVERSE: *pBuf ^=  mask; break;
          }
        }
      }
    } // endif positive height
  } // endif x in bounds
}

void Adafruit_SSD1306::clearRect(uint8_t x, uint8_t y, uint8_t w , uint8_t h) {
  for (int16_t i=x; i<x+w; i++) {
    drawFastVLineInternal(i, y, h, 0);
  }
}

void Adafruit_SSD1306::drawBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            if(byte & 0x80) drawPixel(x+i, y, color);
        }
    }
}

/*!
    @brief  Return color of a single pixel in display buffer.
    @param  x
            Column of display -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @return true if pixel is set (usually SSD1306_WHITE, unless display invert mode
            is enabled), false if clear (SSD1306_BLACK).
    @note   Reads from buffer contents; may not reflect current contents of
            screen if display() has not been called.
*/
bool Adafruit_SSD1306::getPixel(int16_t x, int16_t y) {
  if((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT)) {
    // Pixel is in-bounds. Rotate coordinates if needed.
    return (buffer[x + (y / 8) * WIDTH] & (1 << (y & 7)));
  }
  return false; // Pixel out of bounds
}

/*!
    @brief  Get base address of display buffer for direct reading or writing.
    @return Pointer to an unsigned 8-bit array, column-major, columns padded
            to full byte boundary if needed.
*/
uint8_t *Adafruit_SSD1306::getBuffer(void) {
  return buffer;
}

// REFRESH DISPLAY ---------------------------------------------------------

/*!
    @brief  Push data currently in RAM to SSD1306 display.
    @return None (void).
    @note   Drawing operations are not visible until this function is
            called. Call after each graphics command, or after a whole set
            of graphics commands, as best needed by one's own application.
*/
void Adafruit_SSD1306::display(void) {
  static const uint8_t PROGMEM dlist1[] = {
    SSD1306_PAGEADDR,
    0,                         // Page start address
    0xFF,                      // Page end (not really, but works here)
    SSD1306_COLUMNADDR,
    0 };                       // Column start address
  ssd1306_commandList(dlist1, sizeof(dlist1));
  ssd1306_command1(WIDTH - 1); // Column end address

  uint16_t count = WIDTH * ((HEIGHT + 7) / 8);
  uint8_t *ptr   = buffer;
  uint8_t cmd = 0x40;
  while(count >= (WIRE_MAX)){
    TWI_Start_Transceiver_With_Data(cmd, ptr, WIRE_MAX);
    count -= (WIRE_MAX);
    ptr += (WIRE_MAX);
  }
  if(count > 0) {
    TWI_Start_Transceiver_With_Data(cmd, ptr, count);
  }
}

// OTHER HARDWARE SETTINGS -------------------------------------------------

/*!
    @brief  Enable or disable display invert mode (white-on-black vs
            black-on-white).
    @param  i
            If true, switch to invert mode (black-on-white), else normal
            mode (white-on-black).
    @return None (void).
    @note   This has an immediate effect on the display, no need to call the
            display() function -- buffer contents are not changed, rather a
            different pixel mode of the display hardware is used. When
            enabled, drawing SSD1306_BLACK (value 0) pixels will actually draw white,
            SSD1306_WHITE (value 1) will draw black.
*/
void Adafruit_SSD1306::invertDisplay(bool i) {
  ssd1306_command1(i ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
}
