#ifndef _entity_h
#define _entity_h

#include "coords.h"
#include "uid.h"
#include "static_entity.h"

class Entity  { 
  public:
    Coords pos;
    UID uid;
    uint8_t state;
    uint8_t health;     // angle for fireballs
    uint8_t distance;
    uint8_t timer;

    Entity();
    Entity(uint8_t type, uint8_t x,  uint8_t y, uint8_t initialState, uint8_t initialHealth);
    Entity(StaticEntity staticEntity);
};

Entity instantiate_from_static(StaticEntity staticEntity);
Entity instantiate_enemy(uint8_t x, uint8_t y);
Entity instantiate_medikit(uint8_t x, uint8_t y);
Entity instantiate_key(uint8_t x, uint8_t y);
Entity instantiate_fireball(uint8_t x, uint8_t y, uint8_t direction);

#endif

