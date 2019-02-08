#include "level.h"
#include "sprites.h"
#include "display.h"
#include "input.h"
// #include <MemoryFree.h>

// scenes
#define INTRO                 0
#define GAME_PLAY             1

// game
#define GUN_TARGET_POS        18
#define GUN_SHOT_POS          GUN_TARGET_POS + 4
#define ROT_SPEED             .12
#define MOV_SPEED             .2
#define MOV_SPEED_INV         5           // 1 / MOV_SPEED
#define JOGGING_SPEED         .005
#define ENEMY_SPEED           .02
#define FIREBALL_SPEED        .2

#define MAX_ENTITIES          8           // Max num of active entities
#define MAX_DOZED_ENTITIES    28          // Max num of entities in sleep mode
#define FIREBALL_ANGLES       45          // Num of angles per PI

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

// useful macros
#define swap(a, b)            do { typeof(a) temp = a; a = b; b = temp; } while (0)
#define sign(a, b)            (double) (a > b ? 1 : (b > a ? -1 : 0))
#define dist(pos_a, pos_b)    sqrt(sq(pos_a.x - pos_b.x) + sq(pos_a.y - pos_b.y)) * DISTANCE_MULTIPLIER
#define dist_p(pos_p_a, pos_b)  sqrt(sq(pos_p_a->x - pos_b.x) + sq(pos_p_a->y - pos_b.y)) * DISTANCE_MULTIPLIER
#define isSpawnable(block)    (block == E_ENEMY || block & 0b00001000) /* all collectable items */ 
#define isCollider(block)     (block == E_WALL)

struct Coords {
  double x;
  double y;
};

struct Player {
  Coords pos;
  Coords dir;
  Coords plane;
  double velocity;
  uint8_t health;
  uint8_t keys;
};

// Spawned entities
struct Entity {
  uint16_t uid;
  Coords pos;
  uint8_t state;
  uint8_t health;     // angle for fireballs
  uint8_t distance;
  uint8_t timer;
};

// Static entities
struct DozedEntity {
  uint8_t x;
  uint8_t y;
  uint16_t uid;
  bool active;
};

// general
uint8_t scene = INTRO;
bool exit_scene = false;
bool invert_screen = false;
uint8_t flash_screen = 0;

// game
// player and entities
Player player;
Entity entity[MAX_ENTITIES];
DozedEntity dozed_entity[MAX_DOZED_ENTITIES];
uint8_t num_entities = 0;
uint8_t num_dozed_entities = 0;
uint8_t num_static_entities = 0;

void setup(void) {
  Serial.begin(9600);
  setupDisplay();
  setupInput();
}

// Jump to another scene
void jumpTo(uint8_t target_scene) {
  scene = target_scene;
  exit_scene = true;
}

// Finds the player in the map
void initializeLevel(const uint8_t level[]) {
  for (int y = LEVEL_HEIGHT-1; y >= 0; y--) {
    for (int x = 0; x < LEVEL_WIDTH; x++) {
      uint8_t block = getBlockAt(level, x, y);

      if (block == E_PLAYER) {
        player = { { .5 + x, .5 + y }, { 1, 0 }, { 0, -0.66 }, 0, 100, 0 };
        return;
      }
    }
  }
}

// Generates a unique ID for each entity based in it´s position and type
// block types has 4 bits, so 16 possible combinations
uint16_t getEntityUID(uint8_t block_type, uint8_t x, uint8_t y) {
  return (y * LEVEL_WIDTH + x) * 16 + (block_type & 0b00001111);
}

// Calculate the type from the UID. So we don´t need to read the map again
uint8_t getTypeFromUID(uint16_t uid) {
  return uid % 16;
}

uint8_t getBlockAt(const uint8_t level[], uint8_t x, uint8_t y) {
  if (x < 0 || x >= LEVEL_WIDTH || y < 0 || y >= LEVEL_HEIGHT) {
    return E_FLOOR;
  }

  // y is read in inverse order
  return pgm_read_byte(level + (((LEVEL_HEIGHT - 1 - y) * LEVEL_WIDTH + x) / 2))
    >> (!(x%2) * 4)         // displace part of wanted bits
    & 0b1111;               // mask wanted bits
}

void spawnEntity(uint16_t uid, uint8_t x, uint8_t y) {
  // Limit the number of spawned entities
  if (num_entities >= MAX_ENTITIES) {
    return;
  }
  
  uint8_t type = getTypeFromUID(uid);

  // todo: read dozed entity status

  switch (type) {
    case E_ENEMY: 
      if (num_entities < MAX_ENTITIES) {
        entity[num_entities] = { uid, { .5 + x, .5 + y }, S_STAND, 20, 0 };
        num_entities++;
      }
      break;

    case E_KEY: 
      if (num_entities < MAX_ENTITIES) {
        entity[num_entities] = { uid, { .5 + x, .5 + y }, S_STAND, 0, 0 };
        num_entities++;
      }
      break;

    case E_MEDIKIT:
      if (num_entities < MAX_ENTITIES) {
        entity[num_entities] = { uid, { .5 + x, .5 + y }, S_STAND, 0, 0 };
        num_entities++;
      }
      break;     
  }  
}

void spawnFireball(double x, double y) {  
  // Limit the number of spawned entities
  if (num_entities >= MAX_ENTITIES) {
    return;
  }

  uint16_t uid = getEntityUID(E_FIREBALL, int(x), int(y));
  // Remove if already exists, don´t throw anything. Not the best, but shouldn´t happen too often
  if (isSpawned(uid)) return;

  // Calculate direction. 32 angles
  int16_t dir = FIREBALL_ANGLES + atan2(y - player.pos.y, x - player.pos.x) / PI * FIREBALL_ANGLES;
  if (dir < 0) dir += FIREBALL_ANGLES * 2;
  entity[num_entities] = { uid, { x, y }, S_STAND, dir, 0 };
  num_entities++;
}

void removeEntity(uint16_t uid) {
  uint8_t i = 0;
  bool found = false;

  while(i<num_entities) {
    if (!found && entity[i].uid == uid) {
      // todo: doze it      
      found = true;
      num_entities--;
    }

    // displace entities
    if (found) {
      entity[i] = entity[i+1];
    }
    
    i++;
  }
}

bool isSpawned(uint16_t uid) {
  for (uint8_t i=0; i<num_entities; i++) {
    if (entity[i].uid == uid) return true;
  }
  
  return false;
}

bool isDozed(uint16_t uid) {
  for (uint8_t i=0; i<num_dozed_entities; i++) {
    if (dozed_entity[i].uid == uid) return true;
  }
  
  return false;
}

uint16_t detectCollision(Coords *pos, double relative_x, double relative_y, bool only_walls = false) {
  uint8_t type;
  uint16_t distance;
  uint8_t i = 0;
  
  // Wall collision
  uint8_t round_x = int(pos->x + relative_x);
  uint8_t round_y = int(pos->y + relative_y);
  uint8_t block = getBlockAt(sto_level_1, round_x, round_y);
  
  if (isCollider(block)) {
    return getEntityUID(block, round_x, round_y);
  }

  if (only_walls) {
    return 0;
  }

  // Entity collision
  for (; i<num_entities; i++) {
    // Don't collide with itself
    if (&(entity[i].pos) == pos) {
      continue;
    }
    
    type = getTypeFromUID(entity[i].uid);

    // Only ALIVE enemy collision
    if (type != E_ENEMY || entity[i].state == S_DEAD || entity[i].state == S_HIDDEN) {
      continue;
    }

    Coords new_coords = { entity[i].pos.x - relative_x, entity[i].pos.y - relative_y };
    distance = dist_p(pos, new_coords);

    // Check distance and if it´s getting closer
    if (distance < ENEMY_COLLIDER_DIST && distance < entity[i].distance) {
      return entity[i].uid;
    }
  }

  return 0;
}

// Update coords if possible. Return the collided uid, if any
uint16_t updatePosition(Coords *pos, double relative_x, double relative_y, bool only_walls = false) {
  uint16_t collide_x = detectCollision(pos, relative_x, 0, only_walls);
  uint16_t collide_y = detectCollision(pos, 0, relative_y, only_walls);

  if (!collide_x) pos->x += relative_x;
  if (!collide_y) pos->y += relative_y;  

  return collide_x || collide_y || 0;
}

void updateEntities() {  
  uint8_t i = 0;
  while(i<num_entities) {
    // update distance
    entity[i].distance = dist(player.pos, entity[i].pos);

    // Run the timer. Works with actual frames. 
    // Todo: use delta here. But needs double type and more memory
    if (entity[i].timer > 0) entity[i].timer--;

    // too far away. put it in doze mode
    if (entity[i].distance > MAX_ENTITY_DISTANCE) {
      removeEntity(entity[i].uid);
      // don't increase 'i', since current one has been removed
      continue;
    }  

    // bypass render if hidden
    if (entity[i].state == S_HIDDEN) {
      i++;
      continue;
    }

    uint8_t type = getTypeFromUID(entity[i].uid);
   
    switch (type) {
      case E_ENEMY: {
        // Enemy "IA"
        if (entity[i].health == 0) {
          if (entity[i].state != S_DEAD) {
            entity[i].state = S_DEAD;
            entity[i].timer = 6;  
          }
        } else  if (entity[i].state == S_HIT) {
          if (entity[i].timer == 0) {
            // Back to alert state
            entity[i].state = S_ALERT;
            entity[i].timer = 40;     // delay next fireball thrown
          }
        } else if (entity[i].state == S_FIRING) {
          if (entity[i].timer == 0) {
            // Back to alert state
            entity[i].state = S_ALERT;
            entity[i].timer = 40;     // delay next fireball throwm
          }
        } else {
          // ALERT STATE
          if (entity[i].distance > ENEMY_MELEE_DIST && entity[i].distance < MAX_ENEMY_VIEW) {
            if (entity[i].state != S_ALERT) {
              entity[i].state = S_ALERT;
              entity[i].timer = 20;   // used to throw fireballs
            } else {
              if (entity[i].timer == 0) {
                // Throw a fireball
                spawnFireball(entity[i].pos.x, entity[i].pos.y);
                entity[i].state = S_FIRING;
                entity[i].timer = 6; 
              } else {
                // move towards to the player.
                updatePosition(
                  &(entity[i].pos), 
                  sign(player.pos.x, entity[i].pos.x) * ENEMY_SPEED * delta,
                  sign(player.pos.y, entity[i].pos.y) * ENEMY_SPEED * delta,
                  true
                );
              }
            }
          } else if (entity[i].distance <= ENEMY_MELEE_DIST) {
            if (entity[i].state != S_MELEE) {
              // Preparing the melee attack
              entity[i].state = S_MELEE;
              entity[i].timer = 10;
            } else if (entity[i].timer == 0) {
              // Melee attack
              player.health = max(0, player.health - ENEMY_MELEE_DAMAGE);
              entity[i].timer = 14;
              flash_screen = 1;
              updateHud();
            }
          } else {
            // stand
            entity[i].state = S_STAND;
          }
        }
        break;
      }

      case E_FIREBALL: {
        if (entity[i].distance < FIREBALL_COLLIDER_DIST) {
          // Hit the player and disappear
          player.health = max(0, player.health - ENEMY_FIREBALL_DAMAGE);
          flash_screen = 1;
          updateHud();
          removeEntity(entity[i].uid);
          continue; // continue in the loop
        } else {
          // Move. Only collide with walls. 
          // Note: using health to store the angle of the movement
          uint16_t collided = updatePosition(
            &(entity[i].pos), 
            cos((double) entity[i].health / FIREBALL_ANGLES * PI) * FIREBALL_SPEED,
            sin((double) entity[i].health / FIREBALL_ANGLES * PI) * FIREBALL_SPEED,
            true
          );

          if (collided) {
            removeEntity(entity[i].uid);
            continue; // continue in the entity check loop
          }
        }
        break;
      }

      case E_MEDIKIT: {
        if (entity[i].distance < ITEM_COLLIDER_DIST) {
          // pickup
          entity[i].state = S_HIDDEN;
          player.health = min(100, player.health + 50);
          updateHud();
          flash_screen = 1;
        }
        break; 
      }

      case E_KEY: {
        if (entity[i].distance < ITEM_COLLIDER_DIST) {
          // pickup
          entity[i].state = S_HIDDEN;
          player.keys++;
          updateHud();
          flash_screen = 1;
        }
        break; 
      }
    }

    i++;
  }
}

// The map raycaster. Based on https://lodev.org/cgtutor/raycasting.html
void renderMap(const uint8_t level[], double view_height) {
  double camera_x;
  double ray_x; double ray_y;
  uint8_t map_x; uint8_t map_y;
  double side_x; double side_y;
  double delta_x; double delta_y;
  char step_x; char step_y;
  bool hit;
  bool side;
  uint8_t depth;
  double distance;
  uint8_t line_height;
  uint8_t block;
  uint16_t uid;
 
  for (uint8_t x=0; x<SCREEN_WIDTH; x+=RES_DIVIDER) {
    camera_x = 2 * (double) x / SCREEN_WIDTH - 1;
    ray_x = player.dir.x + player.plane.x * camera_x;
    ray_y = player.dir.y + player.plane.y * camera_x;
    map_x = uint8_t(player.pos.x);
    map_y = uint8_t(player.pos.y);
    Coords map_coords = { map_x, map_y };
    delta_x = abs(1 / ray_x);
    delta_y = abs(1 / ray_y);

    if (ray_x < 0) {
      step_x = -1;
      side_x = (player.pos.x - map_x) * delta_x;
    } else {
      step_x = 1;
      side_x = (map_x + 1.0 - player.pos.x) * delta_x;
    }

    if (ray_y < 0) {
      step_y = -1;
      side_y = (player.pos.y - map_y) * delta_y;
    } else {
      step_y = 1;
      side_y = (map_y + 1.0 - player.pos.y) * delta_y;
    }

    // Wall detection
    depth = 0;
    hit = 0;
    while(!hit && depth < MAX_RENDER_DEPTH) {
      if (side_x < side_y) {
        side_x += delta_x;
        map_x += step_x;
        side = 0;
      } else {
        side_y += delta_y;
        map_y += step_y;
        side = 1;
      }

      block = getBlockAt(level, map_x, map_y);

      if (isCollider(block)) {
          hit = 1;
      } else {
        // Spawning entities here, as soon they are visible for the
        // player. Not the best place, but would be a very performance
        // cost scan for them in another loop
        if (isSpawnable(block)) {
          // Check that it's close to the player
          if (dist(player.pos, map_coords) < MAX_ENTITY_DISTANCE) {
            uid = getEntityUID(block, map_x, map_y);
            if (!isSpawned(uid)) {
              spawnEntity(uid, map_x, map_y);
            } 
          }
        }
      }

      depth++;
    }

    if (hit) {
      if (side == 0) {
        distance = max(1, (map_x - player.pos.x + (1 - step_x) / 2) / ray_x);
      } else {
        distance = max(1, (map_y - player.pos.y + (1 - step_y) / 2) / ray_y);
      }
  
      // store zbuffer value for the column
      zbuffer[x / Z_RES_DIVIDER] = min(distance * DISTANCE_MULTIPLIER, 255);
  
      // rendered line height
      line_height = RENDER_HEIGHT / distance;
  
      drawVLine(
        x,
        view_height / distance - line_height / 2 + RENDER_HEIGHT / 2, 
        view_height / distance + line_height / 2 + RENDER_HEIGHT / 2, 
        GRADIENT_COUNT - int(distance / MAX_RENDER_DEPTH * GRADIENT_COUNT) - side * 2
      );
    }
  }
}

// Sort entities from far to close
uint8_t sortEntities() {
  uint8_t gap = num_entities;
  bool swapped = false;
  while(gap > 1 || swapped) {
    //shrink factor 1.3
    gap = (gap * 10) / 13;
    if(gap == 9 || gap == 10) gap = 11;
    if (gap < 1) gap = 1;
    swapped = false;
    for(uint8_t i = 0; i < num_entities - gap; i++)
    {
      uint8_t j = i + gap;
      if(entity[i].distance < entity[j].distance)
      {
        swap(entity[i], entity[j]);
        swapped = true;
      }
    }
  }
}

Coords translateIntoView(Coords *pos) {
  //translate sprite position to relative to camera
  double sprite_x = pos->x - player.pos.x;
  double sprite_y = pos->y - player.pos.y;
  
  //required for correct matrix multiplication
  double inv_det = 1.0 / (player.plane.x * player.dir.y - player.dir.x * player.plane.y); 
  double transform_x = inv_det * (player.dir.y * sprite_x - player.dir.x * sprite_y);
  double transform_y = inv_det * (- player.plane.y * sprite_x + player.plane.x * sprite_y); // Z in screen
  
  return { transform_x, transform_y };
}

void renderEntities(double view_height) {  
  sortEntities();
  
  for (uint8_t i=0; i<num_entities; i++) {
    if (entity[i].state == S_HIDDEN) continue;
    
    Coords transform = translateIntoView(&(entity[i].pos));

    // don´t render if behind the player or too far away
    if (transform.y <= 0.1 || transform.y > MAX_SPRITE_DEPTH) {
      continue;
    }

    int16_t sprite_screen_x = HALF_WIDTH * (1.0 + transform.x / transform.y);
    int8_t sprite_screen_y = RENDER_HEIGHT / 2 + view_height / transform.y;
    uint8_t type = getTypeFromUID(entity[i].uid);

    // don´t try to render if outside of screen
    // doing this pre-shortcut due int16 -> int8 conversion makes out-of-screen
    // values fit into the screen space
    if (sprite_screen_x < - HALF_WIDTH || sprite_screen_x > SCREEN_WIDTH + HALF_WIDTH) {
      continue;
    }
    
    switch(type) {
      case E_ENEMY: {
        uint8_t sprite;
        if (entity[i].state == S_ALERT) {
          // walking
          sprite = int(millis() / 500) % 2;
        } else if (entity[i].state == S_FIRING) {
          // fireball
          sprite = 2;
        } else if (entity[i].state == S_HIT) {
          // hit
          sprite = 3;
        } else if (entity[i].state == S_MELEE) {
          // melee atack
          sprite = entity[i].timer > 10 ? 2 : 1;
        } else if (entity[i].state == S_DEAD) {
          // dying
          sprite = entity[i].timer > 0 ? 3 : 4;
        } else {
          // stand
          sprite = 0;  
        }

        drawSprite(
          sprite_screen_x - BMP_IMP_WIDTH * .5 / transform.y, 
          sprite_screen_y - 8 / transform.y, 
          bmp_imp_bits, 
          bmp_imp_mask, 
          BMP_IMP_WIDTH, 
          BMP_IMP_HEIGHT, 
          sprite, 
          transform.y
        );
        break;
      }

      case E_FIREBALL: {
        drawSprite(
          sprite_screen_x - BMP_FIREBALL_WIDTH / 2 / transform.y, 
          sprite_screen_y - BMP_FIREBALL_HEIGHT / 2 / transform.y, 
          bmp_fireball_bits, 
          bmp_fireball_mask, 
          BMP_FIREBALL_WIDTH, 
          BMP_FIREBALL_HEIGHT, 
          0,
          transform.y
        );
        break;
      }

      case E_MEDIKIT: {
        drawSprite(
          sprite_screen_x - BMP_ITEMS_WIDTH / 2 / transform.y, 
          sprite_screen_y + 5 / transform.y, 
          bmp_items_bits, 
          bmp_items_mask, 
          BMP_ITEMS_WIDTH, 
          BMP_ITEMS_HEIGHT, 
          0,
          transform.y
        );
        break;
      }

      case E_KEY: {
        drawSprite(
          sprite_screen_x - BMP_ITEMS_WIDTH / 2 / transform.y, 
          sprite_screen_y + 5 / transform.y, 
          bmp_items_bits, 
          bmp_items_mask, 
          BMP_ITEMS_WIDTH, 
          BMP_ITEMS_HEIGHT, 
          1,
          transform.y
        );
        break;
      }
    } 
  }
}

void renderGun(uint8_t gun_pos, double amount_jogging) {
  // jogging
  char x = 48 + sin((double) millis() * JOGGING_SPEED) * 10 * amount_jogging;
  char y = RENDER_HEIGHT - gun_pos + abs(cos((double) millis() * JOGGING_SPEED)) * 8 * amount_jogging;

  if (gun_pos > GUN_SHOT_POS - 2) {
    // Gun fire
    display.drawBitmap(x + 6, y - 11, bmp_fire_bits, BMP_FIRE_WIDTH, BMP_FIRE_HEIGHT, 1);
  }

  // Don´t draw over the hud!
  uint8_t clip_height = max(0, min(y + BMP_GUN_HEIGHT, RENDER_HEIGHT) - y);
  
  // Draw the gun (black mask + actual sprite).
  display.drawBitmap(x, y, bmp_gun_mask, BMP_GUN_WIDTH, clip_height, 0);
  display.drawBitmap(x, y, bmp_gun_bits, BMP_GUN_WIDTH, clip_height, 1);
}

// Only needed first time
void renderHud() {
  drawText(2, 58, F("{}"), 0);        // Health symbol
  drawText(40, 58, F("[]"), 0);       // Keys symbol
  updateHud();
}

// Render values for the HUD
void updateHud() {
  display.fillRect(12, 58, 15, 6, 0);
  display.fillRect(50, 58, 5, 6, 0);
  drawText(12, 58, player.health);   
  drawText(50, 58, player.keys);   
}

// Debug stats
void renderStats() {
  display.fillRect(58, 58, 70, 6, 0);
  drawText(114, 58, int(getActualFps()));
  drawText(82, 58, num_entities);
  // drawText(94, 58, freeMemory());
}

// Intro screen
void loopIntro() {
  // fade in effect
  for (uint8_t i=0; i<8; i++) {
    drawBitmap(
      (SCREEN_WIDTH - BMP_LOGO_WIDTH) / 2,
      (SCREEN_HEIGHT - BMP_LOGO_HEIGHT) / 3,
      bmp_logo_bits,
      BMP_LOGO_WIDTH,
      BMP_LOGO_HEIGHT,
      i
    );
    display.display();
    delay(80);
  }

  delay(1000);
  drawText(SCREEN_WIDTH / 2 - 25, SCREEN_HEIGHT * .8, F("PRESS FIRE"));
  display.display();

  // wait for fire
  do { 
    readInput(); 
    if (p_fire) jumpTo(GAME_PLAY);
  } while(!exit_scene);
}

void fire() {
  for (uint8_t i=0; i<num_entities; i++) {
    uint8_t type = getTypeFromUID(entity[i].uid);

    // Shoot only ALIVE enemies
    if (type != E_ENEMY || entity[i].state == S_DEAD || entity[i].state == S_HIDDEN) {
      continue;
    }

    Coords transform = translateIntoView(&(entity[i].pos));
    if (abs(transform.x) < 20 && transform.y > 0) {
      uint8_t damage = (double) GUN_MAX_DAMAGE / (abs(transform.x) * entity[i].distance) / 5; 
      if (damage > 0) {
        entity[i].health = max(0, entity[i].health - damage);
        entity[i].state = S_HIT;
        entity[i].timer = 4; 
      }
    }
  }
}

void loopGamePlay() {
  bool gun_fired = false;
  uint8_t gun_pos = 0;
  double rot_speed;
  double old_dir_x;
  double old_plane_x;
  double view_height;
  double jogging;

  initializeLevel(sto_level_1);
  renderHud();
  
  do {
    fps();
    readInput();

    // Clear only the 3d view
    display.fillRect(0, 0, SCREEN_WIDTH, RENDER_HEIGHT, 0);

    // If the player is alive
    if (player.health > 0) {
      // Player speed
      if (p_up) {
        player.velocity += (MOV_SPEED - player.velocity) * .4;
        jogging = abs(player.velocity) * MOV_SPEED_INV;
      } else if (p_down) {
        player.velocity += (- MOV_SPEED - player.velocity) * .4;
        jogging = abs(player.velocity) * MOV_SPEED_INV;
      } else {
        player.velocity *= .5;
        jogging = abs(player.velocity) * MOV_SPEED_INV;
      }
  
      // Player rotation 
      if (p_right) {
        rot_speed = ROT_SPEED * delta;
        old_dir_x = player.dir.x;
        player.dir.x = player.dir.x * cos(-rot_speed) - player.dir.y * sin(-rot_speed);
        player.dir.y = old_dir_x * sin(-rot_speed) + player.dir.y * cos(-rot_speed);
        old_plane_x = player.plane.x;
        player.plane.x = player.plane.x * cos(-rot_speed) - player.plane.y * sin(-rot_speed);
        player.plane.y = old_plane_x * sin(-rot_speed) + player.plane.y * cos(-rot_speed);
      } else if (p_left) {
        rot_speed = ROT_SPEED * delta;
        old_dir_x = player.dir.x;
        player.dir.x = player.dir.x * cos(rot_speed) - player.dir.y * sin(rot_speed);
        player.dir.y = old_dir_x * sin(rot_speed) + player.dir.y * cos(rot_speed);
        old_plane_x = player.plane.x;
        player.plane.x = player.plane.x * cos(rot_speed) - player.plane.y * sin(rot_speed);
        player.plane.y = old_plane_x * sin(rot_speed) + player.plane.y * cos(rot_speed);
      }

      view_height = abs(sin((double) millis() * JOGGING_SPEED)) * 6 * jogging;

      // Update gun
      if (gun_pos > GUN_TARGET_POS) {
        // Right after fire
        gun_pos -= 1;
      } else if (gun_pos < GUN_TARGET_POS) {
        // Showing up
        gun_pos += 2;
      } else if (!gun_fired && p_fire) {
        // ready to fire and fire pressed
        gun_pos = GUN_SHOT_POS;
        gun_fired = true;
        fire();
      } else if (gun_fired && !p_fire) {
        // just fired and restored position
        gun_fired = false;
      }
    } else {
      // The player is dead
      if (view_height > -10) view_height--;
      else if (p_fire) jumpTo(INTRO);
      
      if (gun_pos > 1) gun_pos -= 2;
    }

    // Player movement
    if (abs(player.velocity) > 0.003) {
      updatePosition(
        &(player.pos), 
        player.dir.x * player.velocity * delta, 
        player.dir.y * player.velocity * delta
      );
    } else {
      player.velocity = 0;
    }

    // Update things
    updateEntities();

    // Render stuff
    renderMap(sto_level_1, view_height);
    renderEntities(view_height);
    renderGun(gun_pos, jogging);
    renderStats();
    
    // flash screen
    if (flash_screen > 0) {
      invert_screen = !invert_screen;
      flash_screen--;
    } else if (invert_screen) {
      invert_screen = 0;
    }
    
    // Draw the frame
    display.invertDisplay(invert_screen);
    display.display();

    // Exit routine
    if (p_left && p_right) {
      jumpTo(INTRO);
    }
  } while (!exit_scene);
}

void loop(void) {  
  switch(scene) {
    case INTRO: { loopIntro(); break; }
    case GAME_PLAY: { loopGamePlay(); break; }
  }

  exit_scene = false;
  meltScreen();
}
