#ifndef _constants_h
#define _constants_h

// Key pinout
#define USE_INPUT_PULLUP
#define K_LEFT              6
#define K_RIGHT             7
#define K_UP                8
#define K_DOWN              3
#define K_FIRE              10

// SNES Controller
// uncomment following line to enable snes controller support
 #define SNES_CONTROLLER
constexpr uint8_t DATA_CLOCK   = 2;
constexpr uint8_t DATA_LATCH   = 3;
constexpr uint8_t DATA_SERIAL  = 4;

// Sound
constexpr uint8_t SOUND_PIN   = 9; // do not change, belongs to used timer

// GFX settings

#define FRAME_TIME          40   // Desired time per frame in ms (40 is 25 fps)
#define RES_DIVIDER         2           // Higher values will result in lower horizontal resolution when rasterize and lower process and memory usage
                                        // Lower will require more process and memory, but looks nicer
#define Z_RES_DIVIDER       2           // Zbuffer resolution divider. We sacrifice resolution to save memory
#define DISTANCE_MULTIPLIER 20          // Distances are stored as uint8_t, multiplying the distance we can obtain more precision taking care
                                        // of keep numbers inside the type range. Max is 256 / MAX_RENDER_DEPTH
#define MAX_RENDER_DEPTH    12
#define MAX_SPRITE_DEPTH    8

#define ZBUFFER_SIZE        SCREEN_WIDTH / Z_RES_DIVIDER

// Level 
#define LEVEL_WIDTH_BASE    6
#define LEVEL_WIDTH         (1 << LEVEL_WIDTH_BASE)
#define LEVEL_HEIGHT        57
#define LEVEL_SIZE          LEVEL_WIDTH / 2 * LEVEL_HEIGHT

// scenes
#define INTRO                 0
#define GAME_PLAY             1

// Game
#define GUN_TARGET_POS        18
#define GUN_SHOT_POS          GUN_TARGET_POS + 4

#define ROT_SPEED             .12
#define MOV_SPEED             .2
#define MOV_SPEED_INV         5           // 1 / MOV_SPEED

#define JOGGING_SPEED         .005
#define ENEMY_SPEED           .02
#define FIREBALL_SPEED        .2
#define FIREBALL_ANGLES       45          // Num of angles per PI

#define MAX_ENTITIES          10          // Max num of active entities
#define MAX_STATIC_ENTITIES   28          // Max num of entities in sleep mode

#define MAX_ENTITY_DISTANCE   200         // * DISTANCE_MULTIPLIER
#define MAX_ENEMY_VIEW        80          // * DISTANCE_MULTIPLIER
#define ITEM_COLLIDER_DIST    6           // * DISTANCE_MULTIPLIER
#define ENEMY_COLLIDER_DIST   4           // * DISTANCE_MULTIPLIER
#define FIREBALL_COLLIDER_DIST 2          // * DISTANCE_MULTIPLIER
#define ENEMY_MELEE_DIST      6           // * DISTANCE_MULTIPLIER
#define WALL_COLLIDER_DIST    .2

#define ENEMY_MELEE_DAMAGE    8
#define ENEMY_FIREBALL_DAMAGE 20
#define GUN_MAX_DAMAGE        15

// display
constexpr uint8_t SCREEN_WIDTH     =  128;
constexpr uint8_t SCREEN_HEIGHT    =  64;
constexpr uint8_t HALF_WIDTH       =  SCREEN_WIDTH/2;
constexpr uint8_t RENDER_HEIGHT    =  56;         // raycaster working height (the rest is for the hud)
constexpr uint8_t HALF_HEIGHT      =  SCREEN_HEIGHT/2;
constexpr uint8_t dcPin = 5;
constexpr uint8_t csPin = 10;
constexpr uint8_t I2C_ADDR = ((SCREEN_HEIGHT == 32) ? 0x3C : 0x3D); // (0x3C for 32-pixel-tall displays, 0x3D for all others).
enum DisplayMode {
  I2C_MODE,
  SPI_MODE
};
constexpr DisplayMode MODE = SPI_MODE;

#endif
