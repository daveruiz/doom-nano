/*!
 * @file Adafruit_SSD1306.h
 *
 * This is part of for Adafruit's SSD1306 library for monochrome
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
 * Written by Limor Fried/Ladyada for Adafruit Industries, with
 * contributions from the open source community.
 *
 * BSD license, all text above, and the splash screen header file,
 * must be included in any redistribution.
 *
 */

#ifndef _Adafruit_SSD1306_H_
#define _Adafruit_SSD1306_H_

#include "TWI_Master.h"
#include <SPI.h>
#include "string.h"
#include "constants.h"

/// The following "raw" color names are kept for backwards client compatability
/// They can be disabled by predefining this macro before including the Adafruit header
/// client code will then need to be modified to use the scoped enum values directly
#ifndef NO_ADAFRUIT_SSD1306_COLOR_COMPATIBILITY
#define BLACK                     SSD1306_BLACK    ///< Draw 'off' pixels
#define WHITE                     SSD1306_WHITE    ///< Draw 'on' pixels
#define INVERSE                   SSD1306_INVERSE  ///< Invert pixels
#endif
        /// fit into the SSD1306_ naming scheme
#define SSD1306_BLACK               0    ///< Draw 'off' pixels
#define SSD1306_WHITE               1    ///< Draw 'on' pixels
#define SSD1306_INVERSE             2    ///< Invert pixels

#define SSD1306_MEMORYMODE          0x20 ///< See datasheet
#define SSD1306_COLUMNADDR          0x21 ///< See datasheet
#define SSD1306_PAGEADDR            0x22 ///< See datasheet
#define SSD1306_SETCONTRAST         0x81 ///< See datasheet
#define SSD1306_CHARGEPUMP          0x8D ///< See datasheet
#define SSD1306_SEGREMAP            0xA0 ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< See datasheet
#define SSD1306_DISPLAYALLON        0xA5 ///< Not currently used
#define SSD1306_NORMALDISPLAY       0xA6 ///< See datasheet
#define SSD1306_INVERTDISPLAY       0xA7 ///< See datasheet
#define SSD1306_SETMULTIPLEX        0xA8 ///< See datasheet
#define SSD1306_DISPLAYOFF          0xAE ///< See datasheet
#define SSD1306_DISPLAYON           0xAF ///< See datasheet
#define SSD1306_COMSCANINC          0xC0 ///< Not currently used
#define SSD1306_COMSCANDEC          0xC8 ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET    0xD3 ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5 ///< See datasheet
#define SSD1306_SETPRECHARGE        0xD9 ///< See datasheet
#define SSD1306_SETCOMPINS          0xDA ///< See datasheet
#define SSD1306_SETVCOMDETECT       0xDB ///< See datasheet

#define SSD1306_SETLOWCOLUMN        0x00 ///< Not currently used
#define SSD1306_SETHIGHCOLUMN       0x10 ///< Not currently used
#define SSD1306_SETSTARTLINE        0x40 ///< See datasheet

#define SSD1306_EXTERNALVCC         0x01 ///< External display voltage source
#define SSD1306_SWITCHCAPVCC        0x02 ///< Gen. display voltage from 3.3V

#define SSD1306_RIGHT_HORIZONTAL_SCROLL              0x26 ///< Init rt scroll
#define SSD1306_LEFT_HORIZONTAL_SCROLL               0x27 ///< Init left scroll
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29 ///< Init diag scroll
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A ///< Init diag scroll
#define SSD1306_DEACTIVATE_SCROLL                    0x2E ///< Stop scroll
#define SSD1306_ACTIVATE_SCROLL                      0x2F ///< Start scroll
#define SSD1306_SET_VERTICAL_SCROLL_AREA             0xA3 ///< Set scroll range

/*! 
    @brief  Class that stores state and functions for interacting with
            SSD1306 OLED displays.
*/
template <uint8_t WIDTH, uint8_t HEIGHT, DisplayMode DISPLAY_MODE>
class Adafruit_SSD1306 {
 public:
  Adafruit_SSD1306() = default;

  ~Adafruit_SSD1306(void) = default;

  bool begin(uint8_t switchvcc=SSD1306_SWITCHCAPVCC);
  void display(void) __attribute__ ((optimize(3)));

  /*!
    @brief  Clear contents of display buffer (set all pixels to off).
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
  */
  void clearDisplay(void) { memset(buffer, 0, WIDTH * ((HEIGHT + 7) / 8)); }

  // Faster drawPixel than display.drawPixel.
  // Avoids some checks to make it faster.
  void drawPixel(uint8_t x, uint8_t y, bool color, bool raycasterViewport = false);

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
  void         invertDisplay(bool i) { ssd1306_command1(i ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY); }

  /*!
    @brief  Get base address of display buffer for direct reading or writing.
    @return Pointer to an unsigned 8-bit array, column-major, columns padded
            to full byte boundary if needed.
  */
  uint8_t     *getBuffer(void) {  return buffer; }
  void clearRect(uint8_t, uint8_t, uint8_t, uint8_t);
  void drawBitmap(uint8_t x, uint8_t y, const uint8_t bitmap[], uint8_t w, uint8_t h, uint8_t color)  __attribute__ ((optimize(3)));
  // Faster way to render vertical bits
  void drawByte(uint8_t x, uint8_t y, uint8_t b) {
    buffer[(y / 8)*SCREEN_WIDTH + x] = b;
  }
 private:
  void         drawFastVLineInternal(uint8_t x, uint8_t y, uint8_t h) __attribute__ ((optimize(3)));
  void         ssd1306_command1(uint8_t c);
  void         ssd1306_commandList(const uint8_t *c, uint8_t n);

  uint8_t     buffer[WIDTH * ((HEIGHT + 7) / 8)];
  int8_t      vccstate;
};

template class Adafruit_SSD1306<SCREEN_WIDTH, SCREEN_HEIGHT, MODE>;

#endif // _Adafruit_SSD1306_H_
