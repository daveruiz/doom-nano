/* 
todo: Moving this to CPP looks like it takes more Flash storage. Figure out why.
*/
#include "SSD1306.h"
#include "constants.h"

// Reads a char from an F() string
#define F_char(ifsh, ch)    pgm_read_byte(reinterpret_cast<PGM_P>(ifsh) + ch)

// This is slightly faster than bitRead (also bits are read from left to right)
const static uint8_t PROGMEM bit_mask[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
#define read_bit(b, n)      b & pgm_read_byte(bit_mask + n) ? 1 : 0

void setupDisplay();
void fps();
bool getGradientPixel(uint8_t x, uint8_t y, uint8_t i);
void fadeScreen(uint8_t intensity, bool color);
void drawByte(uint8_t x, uint8_t y, uint8_t b);
uint8_t getByte(uint8_t x, uint8_t y);
void drawPixel(int8_t x, int8_t y, bool color, bool raycasterViewport);
void drawVLine(uint8_t x, int8_t start_y, int8_t end_y, uint8_t intensity);
void drawSprite(int8_t x, int8_t y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h, uint8_t sprite, double distance);
void drawChar(int8_t x, int8_t y, char ch);
void drawText(int8_t x, int8_t y, char *txt, uint8_t space = 1);
void drawText(int8_t x, int8_t y, const __FlashStringHelper txt, uint8_t space = 1);

// Initialize screen. Following line is for OLED 128x64 connected by I2C
Adafruit_SSD1306<SCREEN_WIDTH, SCREEN_HEIGHT> display;

// FPS control
double delta = 1;
uint32_t lastFrameTime = 0;

#ifdef OPTIMIZE_SSD1306
// Optimizations for SSD1306 handles buffer directly
uint8_t *display_buf;
#endif

// We don't handle more than MAX_RENDER_DEPTH depth, so we can safety store
// z values in a byte with 1 decimal and save some memory,
uint8_t zbuffer[ZBUFFER_SIZE];

void setupDisplay() {
  // Setup display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Fixed from 0x3D
    Serial.println(F("SSD1306 allocation failed"));
    while (1); // Don't proceed, loop forever
  }

#ifdef OPTIMIZE_SSD1306
  display_buf = display.getBuffer();
#endif

  // initialize z buffer
  memset(zbuffer, 0xFF, ZBUFFER_SIZE);
}

// Adds a delay to limit play to specified fps
// Calculates also delta to keep movement consistent in lower framerates
void fps() {
  while (millis() - lastFrameTime < FRAME_TIME);
  delta = (double)(millis() - lastFrameTime) / FRAME_TIME;
  lastFrameTime = millis();
}

double getActualFps() {
  return 1000 / (FRAME_TIME * delta);
}

// Faster way to render vertical bits
void drawByte(uint8_t x, uint8_t y, uint8_t b) {
#ifdef OPTIMIZE_SSD1306
  display_buf[(y / 8)*SCREEN_WIDTH + x] = b;
#endif
}

boolean getGradientPixel(uint8_t x, uint8_t y, uint8_t i) {
  if (i == 0) return 0;
  if (i >= GRADIENT_COUNT - 1) return 1;

  uint8_t index = max(0, min(GRADIENT_COUNT - 1, i)) * GRADIENT_WIDTH * GRADIENT_HEIGHT // gradient index
                  + y * GRADIENT_WIDTH % (GRADIENT_WIDTH * GRADIENT_HEIGHT)             // y byte offset
                  + x / GRADIENT_HEIGHT % GRADIENT_WIDTH;                               // x byte offset

  // return the bit based on x
  return read_bit(pgm_read_byte(gradient + index), x % 8);
}

void fadeScreen(uint8_t intensity, bool color = 0) {
  for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {
    for (uint8_t y = 0; y < SCREEN_HEIGHT; y++) {
      if (getGradientPixel(x, y, intensity)) 
        drawPixel(x, y, color, false);
    }
  }
}

// Faster drawPixel than display.drawPixel.
// Avoids some checks to make it faster.
void drawPixel(int8_t x, int8_t y, bool color, bool raycasterViewport = false) {
  // prevent write out of screen buffer
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= (raycasterViewport ? RENDER_HEIGHT : SCREEN_HEIGHT)) {
    return;
  }

#ifdef OPTIMIZE_SSD1306
  if (color) {
    // white
    display_buf[x + (y / 8)*SCREEN_WIDTH] |= (1 << (y & 7));
  } else {
    // black
    display_buf[x + (y / 8)*SCREEN_WIDTH] &= ~(1 << (y & 7));
  }
#else
  display.drawPixel(x, y, color);
#endif
}

// For raycaster only
// Custom draw Vertical lines that fills with a pattern to simulate
// different brightness. Affected by RES_DIVIDER
void drawVLine(uint8_t x, int8_t start_y, int8_t end_y, uint8_t intensity) {
  int8_t y;
  int8_t lower_y = max(min(start_y, end_y), 0);
  int8_t higher_y = min(max(start_y, end_y), RENDER_HEIGHT - 1);
  uint8_t c;

#ifdef OPTIMIZE_SSD1306
  uint8_t bp;
  uint8_t b;
  for (c = 0; c < RES_DIVIDER; c++) {
    y = lower_y;
    b = 0;
    while (y <= higher_y) {
      bp = y % 8;
      b = b | getGradientPixel(x + c, y, intensity) << bp;

      if (bp == 7) {
        // write the whole byte
        drawByte(x + c, y, b);
        b = 0;
      }

      y++;
    }

    // draw last byte
    if (bp != 7) {
      drawByte(x + c, y - 1, b);
    }
  }
#else
  y = lower_y;
  while (y <= higher_y) {
    for (c = 0; c < RES_DIVIDER; c++) {
      // bypass black pixels
      if (getGradientPixel(x + c, y, intensity)) {
        drawPixel(x + c, y, 1, true);
      }
    }

    y++;
  }
#endif
}

// Custom drawBitmap method with scale support, mask, zindex and pattern filling
void drawSprite(
  int8_t x, int8_t y,
  const uint8_t bitmap[], const uint8_t mask[],
  int16_t w, int16_t h,
  uint8_t sprite,
  double distance
) {
  uint8_t tw = (double) w / distance;
  uint8_t th = (double) h / distance;
  uint8_t byte_width = w / 8;
  uint8_t pixel_size = max(1, 1.0 / distance);
  uint16_t sprite_offset = byte_width * h * sprite;

  bool pixel;
  bool maskPixel;

  // Don't draw the whole sprite if the anchor is hidden by z buffer
  // Not checked per pixel for performance reasons
  if (zbuffer[min(max(x, 0), ZBUFFER_SIZE - 1) / Z_RES_DIVIDER] < distance * DISTANCE_MULTIPLIER) {
    return;
  }

  for (uint8_t ty = 0; ty < th; ty += pixel_size) {
    // Don't draw out of screen
    if (y + ty < 0 || y + ty >= RENDER_HEIGHT) {
      continue;
    }

    uint8_t sy = ty * distance; // The y from the sprite

    for (uint8_t tx = 0; tx < tw; tx += pixel_size) {
      uint8_t sx = tx * distance; // The x from the sprite
      uint16_t byte_offset = sprite_offset + sy * byte_width + sx / 8;

      // Don't draw out of screen
      if (x + tx < 0 || x + tx >= SCREEN_WIDTH) {
        continue;
      }

      maskPixel = read_bit(pgm_read_byte(mask + byte_offset), sx % 8);

      if (maskPixel) {
        pixel = read_bit(pgm_read_byte(bitmap + byte_offset), sx % 8);
        for (uint8_t ox = 0; ox < pixel_size; ox++) {
          for (uint8_t oy = 0; oy < pixel_size; oy++) {
            drawPixel(x + tx + ox, y + ty + oy, pixel, true);
          }
        }
      }
    }
  }
}

// Draw a single character.
// Made for a custom font with some useful sprites. Char size 4 x 6
// Uses less memory than display.print()
void drawChar(int8_t x, int8_t y, char ch) {
  uint8_t c = 0;
  uint8_t n;
  uint8_t bOffset;
  uint8_t b;
  uint8_t line = 0;

  // Find the character
  while (CHAR_MAP[c] != ch && CHAR_MAP[c] != '\0') c++;

  bOffset = c / 2;
  for (; line < CHAR_HEIGHT; line++) {
    b = pgm_read_byte(bmp_font + (line * bmp_font_width + bOffset));
    for (n = 0; n < CHAR_WIDTH; n++)
      if (read_bit(b, (c % 2 == 0 ? 0 : 4) + n))
        drawPixel(x + n, y + line, 1, false);
  }
}

// Draw a string
void drawText(int8_t x, int8_t y, char *txt, uint8_t space) {
  uint8_t pos = x;
  uint8_t i = 0;
  char ch;
  while ((ch = txt[i]) != '\0') {
    drawChar(pos, y, ch);
    i++;
    pos += CHAR_WIDTH + space;

    // shortcut on end of screen
    if (pos > SCREEN_WIDTH) return;
  }
}

// Draw a F() string
void drawText(int8_t x, int8_t y, const __FlashStringHelper *txt_p, uint8_t space = 1) {
  uint8_t pos = x;
  uint8_t i = 0;
  char ch;
  while (pos < SCREEN_WIDTH && (ch = F_char(txt_p, i)) != '\0') {
    drawChar(pos, y, ch);
    i++;
    pos += CHAR_WIDTH + space;
  }
}

// Draw an integer (3 digit max!)
void drawText(uint8_t x, uint8_t y, uint8_t num) {
  char buf[4]; // 3 char + \0
  itoa(num, buf, 10);
  drawText(x, y, buf);
}
