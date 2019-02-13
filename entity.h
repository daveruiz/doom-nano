#ifndef _entity_h
#define _entity_h

#include "coords.h"
#include "uid.h"
#include "static_entity.h"

// Shortcuts
#define create_enemy(x, y)            create_entity(E_ENEMY, x, y, S_STAND, 100)
#define create_medikit(x, y)          create_entity(E_MEDIKIT, x, y, S_STAND, 0)
#define create_key(x, y)              create_entity(E_KEY, x, y, S_STAND, 0)
#define create_fireball(x, y, dir)    create_entity(E_KEY, x, y, S_STAND, dir)

struct Entity {
  UID uid;
  Coords pos;
  uint8_t state;
  uint8_t health;     // angle for fireballs
  uint8_t distance;
  uint8_t timer;
};

Entity create_entity(uint8_t type, uint8_t x,  uint8_t y, uint8_t initialState, uint8_t initialHealth);

#endif

