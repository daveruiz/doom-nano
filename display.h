#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define HALF_WIDTH          64
#define HALF_HEIGHT         32
#define OLED_RESET          -1          // Reset pin # (or -1 if sharing Arduino reset pin)

// GFX settings
#define FRAME_TIME          66.666666   // Desired time per frame in ms (66.666666 is ~15 fps)
#define RES_DIVIDER         2           // Hgher values will result in lower horizontal resolution when raasterize and lower process and memory usage
                                        // Lower will require more process and memory, but looks nicer
#define MAX_RENDER_DEPTH    12
#define MAX_SPRITE_DEPTH    8
#define MELT_SPEED          6

void setupDisplay();
void fps();
bool getMeltedPixel(uint8_t x, uint8_t y);
bool getGradientPixel(uint8_t x, uint8_t y, uint8_t i);
void meltScreen();
void drawPixel(uint8_t x, uint8_t y, boolean color);
void drawVLine(int8_t x, int8_t start_y, int8_t end_y, uint8_t intensity);
void drawSprite(uint8_t x, uint8_t y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h, uint8_t sprite, double distance);
void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint8_t brightness);

// Initalize screen. Following line is for OLED 128x64 connected by I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// FPS control 
double delta = 1;
double lastFrameTime = 0;

// For optimizations
const static char PROGMEM melt_offsets[] = "1234543234323454343456754321234321234543456543212345432123432123432345676";
const static uint8_t PROGMEM bit_mask[8] = { B10000000, B01000000, B00100000, B00010000, B00001000, B00000100, B00000010, B00000001 };

uint8_t *display_buf;
// We don't handle more than MAX_RENDER_DEPTH depth, so we can safety store
// z values in a byte with 1 decimal and save some memory,
// uint8_t zbuffer[SCREEN_WIDTH/RES_DIVIDER];

void setupDisplay() {
  // Setup display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Fixed from 0x3D
    Serial.println(F("SSD1306 allocation failed"));
    while(1); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display_buf = display.getBuffer();

  // initialize z buffer
  // for (uint8_t i=0; i<SCREEN_WIDTH/RES_DIVIDER; i++) zbuffer[i] = 255;
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

// Helper for melting screen. Picks the relative pixel after melt effect
// Similar to adafruit::getPixel but removed some checks to make it faster. 
bool getMeltedPixel(uint8_t frame, uint8_t x, uint8_t y) {
  uint8_t offset = pgm_read_byte(melt_offsets + x%64) - 48; // get "random:" numbers from 0 - 9

  // Return same pixel
  if (frame < offset) 
    return display_buf[x + (y / 8) * SCREEN_WIDTH] & (1 << (y & 7));

  int8_t dy = y - MELT_SPEED;
  
  // Return black
  if (dy < 0) return false; 
   
  return display_buf[x + (dy / 8) * SCREEN_WIDTH] & (1 << (dy & 7));
}

// Melt the screen DOOM style
void meltScreen() {
  uint8_t frames = 0;
  uint8_t x;
  int8_t y;

  do {  
    fps();

    // The screen distribution is 8 rows of 128x8 pixels
    for (y = SCREEN_HEIGHT - 8; y >= 0; y-=8) {
      for (x = 0; x < SCREEN_WIDTH;  x++) {
        // To speed up the animation, it writes directly to buffer the full byte
        display_buf[(y/8)*SCREEN_WIDTH + x] = 
            (getMeltedPixel(frames, x, y+7) << 7)
          | (getMeltedPixel(frames, x, y+6) << 6)
          | (getMeltedPixel(frames, x, y+5) << 5)
          | (getMeltedPixel(frames, x, y+4) << 4)
          | (getMeltedPixel(frames, x, y+3) << 3)
          | (getMeltedPixel(frames, x, y+2) << 2)
          | (getMeltedPixel(frames, x, y+1) << 1)
          | getMeltedPixel(frames, x, y);
      }
    }

    display.display();

    frames++;
  } while(frames < 30);
}

boolean getGradientPixel(uint8_t x, uint8_t y, uint8_t i) {
  if (i <= 0) return 0;
  if (i >= gradient_count - 1) return 1;
  
  uint8_t index = max(0, min(gradient_count - 1, i)) * gradient_width * gradient_height// gradient index
    + y * gradient_width % (gradient_width * gradient_height)                       // y byte offset
    + x / gradient_height % gradient_width;                                         // x byte offset

  // return the bit based on x
  return pgm_read_byte_near(gradient + index) & pgm_read_byte(bit_mask + x % 8) ? 1 : 0;  
}

// Faster drawPixel
void drawPixel(uint8_t x, uint8_t y, boolean color) {
  if (color) {
    // white
    display_buf[x + (y/8)*SCREEN_WIDTH] |= (1 << (y&7));
  } else {
    // black
    display_buf[x + (y/8)*SCREEN_WIDTH] &= ~(1 << (y&7));
  }
}

// Custom draw Vertical lines that fills with a pattern to simulate
// different brightness. Affected by RES_DIVIDER
void drawVLine(uint8_t x, int8_t start_y, int8_t end_y, uint8_t intensity) {
  int8_t y = start_y;
  int8_t s = end_y > y ? 1 : -1;

  while (y != end_y) {
    if (y>=0 && y<SCREEN_HEIGHT) {
      for (uint8_t c=0; c<RES_DIVIDER; c++) {
        // bypass black pixels
        if (getGradientPixel(x+c, y, intensity)) {
          drawPixel(x+c, y, 1);
        }
      }
    }
    
    y+=s;
  }
}

// Custom drawBitmap method with scale support, mask, zindex and pattern filling
void drawSprite(
  uint8_t x, uint8_t y, 
  const uint8_t bitmap[], const uint8_t mask[],
  int16_t w, int16_t h, 
  uint8_t sprite, 
  double distance
) {
  uint8_t tw = (double) w / distance;
  uint8_t th = (double) h / distance;
  uint8_t byte_width = w / 8;
  uint16_t sprite_offset = byte_width * h * sprite;
  uint8_t intensity = gradient_count - int(distance / MAX_RENDER_DEPTH * gradient_count);
  
  bool pixel;
  bool maskPixel;

  for (uint8_t ty=0; ty<th; ty++) {
    if (y + ty < 0 || y + ty >= SCREEN_HEIGHT) {
      continue;
    }

    uint8_t sy = ty * distance; // The y from the sprite
    
    for (uint8_t tx=0; tx<tw; tx++) {
      uint8_t sx = tx * distance; // The x from the sprite
      uint8_t byte_offset = sprite_offset + sy * byte_width + sx / 8;

      // don't draw if out of screen 
      if (x + tx < 0 || x + tx >= SCREEN_WIDTH) {
        continue;
      }

      // don't draw if the pixel is hidden by z buffer
      // if (zbuffer[int(tx)] < distance*10) {
      // continue;
      // }

      pixel = pgm_read_byte(bitmap + byte_offset) & pgm_read_byte(bit_mask + sx % 8) ? 1 : 0;
      maskPixel = pgm_read_byte(mask + byte_offset) & pgm_read_byte(bit_mask + sx % 8) ? 1 : 0;
      
      if (maskPixel) {
        drawPixel(x + tx, y + ty, pixel
          ? getGradientPixel(tx, ty, intensity) 
          : 0);
      }
    }
  }
}

// Custom draw bitmap with brighness support
// Note: is much more slower than display.drawBitmap
void drawBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h, uint8_t brightness) {

  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t dot = 0;

  for(int16_t j=0; j<h; j++, y++) {
      for(int16_t i=0; i<w; i++) {
          if(i & 7) dot <<= 1;
          else      dot   = pgm_read_byte_near(&bitmap[j * byteWidth + i / 8]);
          if(dot & 0x80) drawPixel(x+i, y, getGradientPixel(j, i, brightness));
      }
  }
}
