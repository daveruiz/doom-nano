#include "level.h"
#include "sprites.h"
#include "display.h"
#include "input.h"

// scenes
#define INTRO             0
#define GAME_PLAY         1
#define GAME_OVER         2

// game
#define GUN_TARGET_POS    26
#define GUN_SHOT_POS      30
#define ROT_SPEED         0.12
#define MOV_SPEED         .2
#define MOV_SPEED_INV     5     // 1 / MOV_SPEED
#define MAX_ENTITIES      23

#define S_STAND           0
#define S_ALERT           1
#define S_FIRING          2
#define S_HIDDEN          3

#define T_NULL            0
#define T_ENEMY           1
#define T_KEY             2
#define T_MEDIKIT         3
#define T_FURNITURE       4

struct player_def {
  double x;
  double y;
  double dir_x;
  double dir_y;
  double plane_x;
  double plane_y;
  double velocity;
  uint8_t health;
  uint8_t keys;
};

struct entity_def {
  uint8_t type;
  double x;
  double y;
  uint8_t state;
  uint8_t health;
};

// general
uint8_t scene = INTRO;
bool exit_scene = false;

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

uint8_t initializeLevel(const uint8_t level[], player_def *player, entity_def *entity) {
  uint8_t num_entities = 0;
  
  // Find the player and entities
  for (int y = LEVEL_HEIGHT-1; y >= 0; y--) {
    for (int x = 0; x < LEVEL_WIDTH; x++) {
      uint8_t b = getBlockAt(level, x, y);

      switch(b) {          
        case E_PLAYER: 
          *player = { (double) x + .5, (double) y + .5, 1, 0, 0, -0.66, 0, 100, 0 };
          break;
          
        case E_ENEMY: 
          if (num_entities < MAX_ENTITIES) {
            entity[num_entities] = { T_ENEMY, (double) x + .5, (double) y + .5, S_STAND, 20 };
            num_entities++;
          }
          break;

        case E_KEY: 
          if (num_entities < MAX_ENTITIES) {
            entity[num_entities] = { T_KEY, (double) x + .5, (double) y + .5, S_STAND, 0 };
            num_entities++;
          }
          break;

        case E_MEDIKIT:
          if (num_entities < MAX_ENTITIES) {
            entity[num_entities] = { T_MEDIKIT, (double) x + .5, (double) y + .5, S_STAND, 0 };
            num_entities++;
          }
          break;     
      }
    }
  }

  Serial.print(F("Total entities: "));
  Serial.println(num_entities);

  return num_entities;
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

// The map raycaster. Based on https://lodev.org/cgtutor/raycasting.html
void renderMap(const uint8_t level[], player_def *player, double amount_jogging) {
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
  double jogging = abs(sin((double) millis() / 200)) * 6 * amount_jogging;
 
  for (uint8_t x=0; x<SCREEN_WIDTH; x+=RES_DIVIDER) {
    camera_x = 2 * (double) x / SCREEN_WIDTH - 1;
    ray_x = player->dir_x + player->plane_x * camera_x;
    ray_y = player->dir_y + player->plane_y * camera_x;
    map_x = byte(player->x);
    map_y = byte(player->y);
    delta_x = abs(1 / ray_x);
    delta_y = abs(1 / ray_y);

    if (ray_x < 0) {
      step_x = -1;
      side_x = (player->x - map_x) * delta_x;
    } else {
      step_x = 1;
      side_x = (map_x + 1.0 - player->x) * delta_x;
    }

    if (ray_y < 0) {
      step_y = -1;
      side_y = (player->y - map_y) * delta_y;
    } else {
      step_y = 1;
      side_y = (map_y + 1.0 - player->y) * delta_y;
    }

    // Detection
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

      if (getBlockAt(level, map_x, map_y) == E_WALL) {
          hit = 1;
      }

      depth++;
    }

    if (hit) {
      if (side == 0) {
        distance = max(1, (map_x - player->x + (1 - step_x) / 2) / ray_x);
      } else {
        distance = max(1, (map_y - player->y + (1 - step_y) / 2) / ray_y);
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

void renderEntities(entity_def* entity, player_def* player, uint8_t num_entities) {  
  // calculate distances to the player
  /*
  for(int i = 0; i < num_entities; i++) {
    entity[i].distance = ((player.x - entity[i].x) * (player.x - entity[i].x) + (player.y - entity[i].y) * (player.y - entity[i].y)); 
  }
  */

  // todo: sort sprites from far to close
  
  for (int i=0; i<num_entities; i++) {
    if (entity[i].state == S_HIDDEN) continue;
    
    //translate sprite position to relative to camera
    double sprite_x = entity[i].x - player->x;
    double sprite_y = entity[i].y - player->y;
    
    //required for correct matrix multiplication
    double inv_det = 1.0 / (player->plane_x * player->dir_y - player->dir_x * player->plane_y); 

    double transform_x = inv_det * (player->dir_y * sprite_x - player->dir_x * sprite_y);
    double transform_y = inv_det * (-player->plane_y * sprite_x + player->plane_x * sprite_y); // Z in screen

    if (transform_y < 0 || transform_y > MAX_SPRITE_DEPTH) {
      continue;
    }

    int sprite_screen_x = int((SCREEN_WIDTH / 2) * (1 + transform_x / transform_y));

    // don't draw if it's outside of screen
    if (sprite_screen_x < 0 || sprite_screen_x > SCREEN_WIDTH) {
      continue;
    }

    switch(entity[i].type) {
      case T_ENEMY:
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

      case T_MEDIKIT:
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

      case T_KEY:
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
  char x = 48 + sin((double) millis() / 400) * 10 * amount_jogging;
  char y = 64 - gun_pos + abs(cos((double) millis() / 400)) * 8 * amount_jogging;

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

void loopGamePlay() {
  bool gun_fired = false;
  uint8_t gun_pos = 0;

  // player and entities
  player_def player;
  entity_def entity[MAX_ENTITIES];
  uint8_t num_entities = 0;

  num_entities = initializeLevel(sto_level_1, &player, entity);
  
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

    if (abs(player.velocity) > 0.003) {
      if (getBlockAt(sto_level_1, player.x + player.dir_x * player.velocity * 2, player.y) != E_WALL) 
        player.x += player.dir_x * player.velocity * delta;
      if (getBlockAt(sto_level_1, player.x, player.y + player.dir_y * player.velocity * 2) != E_WALL) 
        player.y += player.dir_y * player.velocity * delta;
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

    // Render stuff
    renderMap(sto_level_1, &player, abs(player.velocity) * MOV_SPEED_INV);
    renderEntities(entity, &player, num_entities);
    renderGun(gun_pos, abs(player.velocity) * MOV_SPEED_INV);
    renderFps();
    
    // Draw the frame
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
