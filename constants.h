#ifndef _constants_h
#define _constants_h

// Key pinout
// #define USE_INPUT_PULLUPS
#define K_LEFT              6
#define K_RIGHT             7
#define K_UP                8
#define K_DOWN              9
#define K_FIRE              10

// GFX settings
#define FRAME_TIME          66.666666   // Desired time per frame in ms (66.666666 is ~15 fps)
#define RES_DIVIDER         2           // Hgher values will result in lower horizontal resolution when rasterize and lower process and memory usage
                                        // Lower will require more process and memory, but looks nicer
#define Z_RES_DIVIDER       2           // Zbuffer resolution divider. We sacrifice resolution to save memory
#define DISTANCE_MULTIPLIER 20          // Distances are stored as uint8_t, mutiplying the distance we can obtain more precision taking care
                                        // of keep numbers inside the type range. Max is 256 / MAX_RENDER_DEPTH
#define MAX_RENDER_DEPTH    12
#define MAX_SPRITE_DEPTH    8

#define ZBUFFER_SIZE        SCREEN_WIDTH / Z_RES_DIVIDER

// Level 
#define LEVEL_WIDTH         64
#define LEVEL_HEIGHT        57
#define LEVEL_SIZE          LEVEL_WIDTH / 2 * LEVEL_HEIGHT

// Entity types (legend applies to level.h)
#define E_FLOOR             0x0   // . (also null)
#define E_WALL              0xF   // #
#define E_PLAYER            0x1   // P
#define E_ENEMY             0x2   // E
#define E_DOOR              0x4   // D
#define E_LOCKEDDOOR        0x5   // L
#define E_EXIT              0x7   // X
// collectable entities >= 0x8
#define E_MEDIKIT           0x8   // M
#define E_KEY               0x9   // K
#define E_FIREBALL          0xA   // not in map

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

// entity status
#define S_STAND               0
#define S_ALERT               1
#define S_FIRING              2
#define S_MELEE               3
#define S_HIT                 4
#define S_DEAD                5
#define S_HIDDEN              6
#define S_OPEN                7
#define S_CLOSE               8

// useful macros
#define swap(a, b)            do { typeof(a) temp = a; a = b; b = temp; } while (0)
#define sign(a, b)            (double) (a > b ? 1 : (b > a ? -1 : 0))
// #define dist(pos_a, pos_b)    sqrt(sq(pos_a.x - pos_b.x) + sq(pos_a.y - pos_b.y)) * DISTANCE_MULTIPLIER
// #define dist_p(pos_p_a, pos_b)  sqrt(sq(pos_p_a->x - pos_b.x) + sq(pos_p_a->y - pos_b.y)) * DISTANCE_MULTIPLIER
// #define isCollider(block)     (block == E_WALL)

#endif