#include "level.h"
#include "sprites.h"
#include "display.h"
#include "input.h"

// scenes
#define INTRO                 0
#define GAME_PLAY             1
#define GAME_OVER             2

// game
#define GUN_TARGET_POS        26
#define GUN_SHOT_POS          30
#define ROT_SPEED             .12
#define MOV_SPEED             .2
#define MOV_SPEED_INV         5         // 1 / MOV_SPEED
#define JOGGING_SPEED         .005

#define DISTANCE_MULTIPLIER   10
#define MAX_ENTITIES          8
#define MAX_DOZED_ENTITIES    32

#define MAX_ENTITY_DISTANCE   80        // * DISTANCE_MULTIPLIER
#define MAX_ENEMY_VIEW        60        // * DISTANCE_MULTIPLIER
#define ITEM_COLLIDER_RAD     3         // * DISTANCE_MULTIPLIER
#define ENEMY_COLLIDER_RAD    3         // * DISTANCE_MULTIPLIER

// entity status
#define S_STAND               0
#define S_ALERT               1
#define S_FIRING              2
#define S_HIDDEN              3

struct position_def {
  double x;
  double y;
};

struct player_def {
  position_def pos;
  double dir_x;
  double dir_y;
  double plane_x;
  double plane_y;
  double velocity;
  uint8_t health;
  uint8_t keys;
};

// Spawned entities
struct entity_def {
  uint16_t uid;
  position_def pos;
  uint8_t state;
  uint8_t health;
  uint8_t distance;
};

// Static entities
struct dozed_entity_def {
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
player_def player;
entity_def entity[MAX_ENTITIES];
dozed_entity_def dozed_entity[MAX_DOZED_ENTITIES];
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
        player = { { (double) x + .5, (double) y + .5 }, 1, 0, 0, -0.66, 0, 100, 0 };
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

  // todo: read sleeping status

  switch (type) {
    case E_ENEMY: 
      if (num_entities < MAX_ENTITIES) {
        entity[num_entities] = { uid, { (double) x + .5, (double) y + .5 }, S_STAND, 20 };
        num_entities++;
      }
      break;

    case E_KEY: 
      if (num_entities < MAX_ENTITIES) {
        entity[num_entities] = { uid, { (double) x + .5, (double) y + .5 }, S_STAND, 0 };
        num_entities++;
      }
      break;

    case E_MEDIKIT:
      if (num_entities < MAX_ENTITIES) {
        entity[num_entities] = { uid, { (double) x + .5, (double) y + .5 }, S_STAND, 0 };
        num_entities++;
      }
      break;     
  }

  /*
  Serial.print(F("Spawned ID "));
  Serial.print(uid);
  Serial.print(F(" "));
  Serial.print(type, BIN);
  Serial.print(F(" "));
  Serial.print(x);
  Serial.print(F(" "));
  Serial.print(y);
  Serial.print(F(" (count "));
  Serial.print(num_entities);
  Serial.print(F(")"));
  Serial.println();
  */
}

uint8_t getDistance(position_def pos_a, position_def pos_b) {
  return sqrt(sq(pos_a.x - pos_b.x) + sq(pos_a.y - pos_b.y)) * DISTANCE_MULTIPLIER;
}

void removeEntity(uint16_t uid) {
  uint8_t i = 0;
  bool found = false;

  while(i<num_entities) {
    if (!found && entity[i].uid == uid) {
      // todo: doze it      
      found = true;
      num_entities--;

      /*
      Serial.print(F("Removed ID "));
      Serial.print(uid);
      Serial.print(F(" "));
      Serial.print(getTypeFromUID(uid), BIN);
      Serial.print(F(" (count "));
      Serial.print(num_entities);
      Serial.print(F(")"));
      Serial.println();
      */
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

void updateEntities() {  
  uint8_t i = 0;
  while(i<num_entities) {
    // update distance
    entity[i].distance = getDistance(player.pos, entity[i].pos);

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
        if (entity[i].distance < MAX_ENEMY_VIEW) {
          // move towards to the player. todo
        }
        break;
      }

      case E_MEDIKIT: {
        if (entity[i].distance < ITEM_COLLIDER_RAD) {
          // pickup
          entity[i].state = S_HIDDEN;
          player.health = min(100, player.health + 50);
          flash_screen = 1;
        }
        break; 
      }

      case E_KEY: {
        if (entity[i].distance < ITEM_COLLIDER_RAD) {
          // pickup
          entity[i].state = S_HIDDEN;
          player.keys++;
          flash_screen = 1;
        }
        break; 
      }
    }

    i++;
  }
}

// The map raycaster. Based on https://lodev.org/cgtutor/raycasting.html
void renderMap(const uint8_t level[], double amount_jogging) {
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
  double jogging = abs(sin((double) millis() * JOGGING_SPEED)) * 6 * amount_jogging;
  uint8_t block;
  uint16_t uid;
 
  for (uint8_t x=0; x<SCREEN_WIDTH; x+=RES_DIVIDER) {
    camera_x = 2 * (double) x / SCREEN_WIDTH - 1;
    ray_x = player.dir_x + player.plane_x * camera_x;
    ray_y = player.dir_y + player.plane_y * camera_x;
    map_x = uint8_t(player.pos.x);
    map_y = uint8_t(player.pos.y);
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

      if (block == E_WALL) {
          hit = 1;
      } else {
        // Spawning entities here, as soon they are visible for the
        // player. Not the best place, but would be a very performance
        // cost scan for them in another loop
        if (block == E_ENEMY || block & 0b00001000) {
          // Check that it's close to the player
          if (getDistance(player.pos, { map_x, map_y }) < MAX_ENTITY_DISTANCE) {
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
      zbuffer[int((double) x / Z_RES_DIVIDER)] = min(distance * 10, 255);
  
      // rendered line height
      line_height = SCREEN_HEIGHT / distance;
  
      drawVLine(
        x,
        jogging / distance - line_height / 2 + SCREEN_HEIGHT / 2, 
        jogging / distance + line_height / 2 + SCREEN_HEIGHT / 2, 
        gradient_count - int(distance / MAX_RENDER_DEPTH * gradient_count) - side * 2
      );
    }
  }
}

void renderEntities() {  
  // todo: sort sprites from far to close
  
  for (uint8_t i=0; i<num_entities; i++) {
    if (entity[i].state == S_HIDDEN) continue;
    
    //translate sprite position to relative to camera
    double sprite_x = entity[i].pos.x - player.pos.x;
    double sprite_y = entity[i].pos.y - player.pos.y;
    
    //required for correct matrix multiplication
    double inv_det = 1.0 / (player.plane_x * player.dir_y - player.dir_x * player.plane_y); 

    double transform_x = inv_det * (player.dir_y * sprite_x - player.dir_x * sprite_y);
    double transform_y = inv_det * (- player.plane_y * sprite_x + player.plane_x * sprite_y); // Z in screen
    double half_width = .5 / transform_y;

    // behind the player or too far away
    if (transform_y <= 0 || transform_y > MAX_SPRITE_DEPTH) {
      continue;
    }

    // double because int causes artifacts when sprites are out of screen
    // but close on Z
    double sprite_screen_x = (double) HALF_WIDTH * (1.0 + transform_x / transform_y);

    // don't draw if it's outside of screen
    if (sprite_screen_x < 0 - half_width || sprite_screen_x > SCREEN_WIDTH + half_width) {
      continue;
    }

    uint8_t type = getTypeFromUID(entity[i].uid);

    /*
    Serial.print(F("Render ID "));
    Serial.print(entity[i].uid);
    Serial.print(F(" "));
    Serial.print(type, BIN);
    Serial.print(F(" x:"));
    Serial.print(entity[i].pos.x);
    Serial.print(F(" y:"));
    Serial.print(entity[i].pos.y);
    Serial.print(F(" z:"));
    Serial.print(transform_y);
    Serial.print(F(" ("));
    Serial.print(i);
    Serial.print(F(")"));
    Serial.println();
    */

    switch(type) {
      case E_ENEMY:
        drawSprite(
          sprite_screen_x - bmp_imp_width * .5 / transform_y, 
          SCREEN_HEIGHT / 2 - 12 / transform_y, 
          bmp_imp_bits, 
          bmp_imp_mask, 
          bmp_imp_width, 
          bmp_imp_height, 
          int(millis() / 500) % 2, 
          transform_y
        );
        break;

      case E_MEDIKIT:
        drawSprite(
          sprite_screen_x - bmp_imp_width * .5 / transform_y, 
          SCREEN_HEIGHT / 2 + 5 / transform_y, 
          bmp_items_bits, 
          bmp_items_mask, 
          bmp_items_width, 
          bmp_items_height, 
          0,
          transform_y
        );
        break;

      case E_KEY:
        drawSprite(
          sprite_screen_x - bmp_imp_width * .5 / transform_y, 
          SCREEN_HEIGHT / 2 + 5 / transform_y, 
          bmp_items_bits, 
          bmp_items_mask, 
          bmp_items_width, 
          bmp_items_height, 
          1,
          transform_y
        );
        break;
    }
  }
}

void renderGun(uint8_t gun_pos, double amount_jogging) {
  // jogging
  char x = 48 + sin((double) millis() * JOGGING_SPEED) * 10 * amount_jogging;
  char y = 64 - gun_pos + abs(cos((double) millis() * JOGGING_SPEED)) * 8 * amount_jogging;

  if (gun_pos > GUN_SHOT_POS - 2) {
    // Gun fire
    display.drawBitmap(x + 6, y - 11, bmp_fire_bits, bmp_fire_width, bmp_fire_height, 1);
  }
  
  // Draw the gun (black mask + actual sprite).
  display.drawBitmap(x, y, bmp_gun_mask, bmp_gun_width, bmp_gun_height, 0);
  display.drawBitmap(x, y, bmp_gun_bits, bmp_gun_width, bmp_gun_height, 1);
}

void renderFps() {
  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.setCursor(0, -0);
  display.print(int(getActualFps()));
}

void loopIntro() {
  // fade in effect
  for (uint8_t i=0; i<8; i++) {
    drawBitmap(
      (SCREEN_WIDTH - bmp_logo_width) / 2,
      (SCREEN_HEIGHT - bmp_logo_height) / 2,
      bmp_logo_bits,
      bmp_logo_width,
      bmp_logo_height,
      i
    );
    display.display();
    delay(100);
  }

  delay(1000);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(33, 56);
  display.print(F("PRESS FIRE"));
  display.display();

  // wait for fire
  do { 
    readInput(); 
    if (p_fire) jumpTo(GAME_PLAY);
  } while(!exit_scene);
}

bool detectPlayerCollision(double relative_x, double relative_y) {
  // Wall collision
  if (getBlockAt(sto_level_1, player.pos.x + relative_x, player.pos.y + relative_y) == E_WALL) {
    return true;
  }

  for (uint8_t i = 0; i<num_entities; i++) {
    uint8_t type = getTypeFromUID(entity[i].uid);

    if (type == E_MEDIKIT || type == E_KEY) {
      // we don´t collide with those entities
      continue;
    }

    uint16_t distance = getDistance(player.pos, { entity[i].pos.x - relative_x, entity[i].pos.y - relative_y });

    /*
    Serial.print(relative_x, 4);
    Serial.print(F(", "));
    Serial.print(relative_y, 4);
    Serial.print(F(" | "));
    Serial.print(entity[i].distance, 4);
    Serial.print(F(" > "));
    Serial.print(distance, 4);
    Serial.println();
    */

    // Check distance and if it´s getting closer
    if (distance < ENEMY_COLLIDER_RAD && distance < entity[i].distance) {
      return true;
    }
  }

  return false;
}

void loopGamePlay() {
  bool gun_fired = false;
  uint8_t gun_pos = 0;

  initializeLevel(sto_level_1);
  
  do {
    fps();
    readInput();

    display.clearDisplay();

    // Player movement
    if (p_up) {
      player.velocity += (MOV_SPEED - player.velocity) * .4; ;
    } else if (p_down) {
      player.velocity += (- MOV_SPEED - player.velocity) * .4;
    } else {
      player.velocity *= .5;
    }

    // Collision
    if (abs(player.velocity) > 0.003) {
      if (!detectPlayerCollision(player.dir_x * player.velocity * 2, 0)) 
        player.pos.x += player.dir_x * player.velocity * delta;
      if (!detectPlayerCollision(0, player.dir_y * player.velocity * 2)) 
        player.pos.y += player.dir_y * player.velocity * delta;
        
    } else {
      player.velocity = 0;
    }

    // Player rotation 
    
    if (p_right) {
      double rot_speed = ROT_SPEED * delta;
      double old_dir_x = player.dir_x;
      player.dir_x = player.dir_x * cos(-rot_speed) - player.dir_y * sin(-rot_speed);
      player.dir_y = old_dir_x * sin(-rot_speed) + player.dir_y * cos(-rot_speed);
      double old_plane_x = player.plane_x;
      player.plane_x = player.plane_x * cos(-rot_speed) - player.plane_y * sin(-rot_speed);
      player.plane_y = old_plane_x * sin(-rot_speed) + player.plane_y * cos(-rot_speed);
    }
    
    if (p_left) {
      double rot_speed = ROT_SPEED * delta;
      double old_dir_x = player.dir_x;
      player.dir_x = player.dir_x * cos(rot_speed) - player.dir_y * sin(rot_speed);
      player.dir_y = old_dir_x * sin(rot_speed) + player.dir_y * cos(rot_speed);
      double old_plane_x = player.plane_x;
      player.plane_x = player.plane_x * cos(rot_speed) - player.plane_y * sin(rot_speed);
      player.plane_y = old_plane_x * sin(rot_speed) + player.plane_y * cos(rot_speed);
    }

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
    } else if (gun_fired && !p_fire) {
      // just fired and restored position
      gun_fired = false;
    }

    // Update things
    updateEntities();

    // Render stuff
    renderMap(sto_level_1, abs(player.velocity) * MOV_SPEED_INV);
    renderEntities();
    renderGun(gun_pos, abs(player.velocity) * MOV_SPEED_INV);
    renderFps();

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

void loopGameOver() {
   jumpTo(INTRO);    
}

void loop(void) {  
  switch(scene) {
    case INTRO: { loopIntro(); break; }
    case GAME_PLAY: { loopGamePlay(); break; }
    case GAME_OVER: { loopGameOver(); break; }
  }

  exit_scene = false;
  meltScreen();
}
