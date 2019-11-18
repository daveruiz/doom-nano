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
#include "string.h"

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
class Adafruit_SSD1306 {
 public:
  Adafruit_SSD1306(uint8_t w, uint8_t h) :
    WIDTH(w), HEIGHT(h) {
  }

  ~Adafruit_SSD1306(void);

  bool      begin(uint8_t switchvcc=SSD1306_SWITCHCAPVCC,
                 uint8_t i2caddr=0);
  void         display(void);
  void         clearDisplay(void);
  void         invertDisplay(bool i);
  void         drawPixel(int16_t x, int16_t y, uint16_t color);
  virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  bool      getPixel(int16_t x, int16_t y);
  uint8_t     *getBuffer(void);
  void clearRect(uint8_t, uint8_t, uint8_t, uint8_t);
  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);

 private:
  void         drawFastVLineInternal(int16_t x, int16_t y, int16_t h,
                 uint16_t color);
  void         ssd1306_command1(uint8_t c);
  void         ssd1306_commandList(const uint8_t *c, uint8_t n);

  uint8_t     *buffer;
  int8_t       i2caddr, vccstate, page_end;
  uint16_t  WIDTH,          ///< This is the 'raw' display width - never changes
    HEIGHT;         ///< This is the 'raw' display height - never changes
};

#endif // _Adafruit_SSD1306_H_
