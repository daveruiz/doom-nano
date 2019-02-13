#include <stdint.h>
#include "entity.h"
#include "static_entity.h"
#include "coords.h"
#include "uid.h"
#include "constants.h"

Entity::Entity(): 
  uid(UID(E_FLOOR, 0, 0)), 
  pos(Coords(0, 0)),
  state(S_HIDDEN),
  health(0) { }

Entity::Entity(uint8_t type, uint8_t x,  uint8_t y, uint8_t initialState, uint8_t initialHealth):
  uid(UID(type, x, y)),
  pos(Coords((double) x + .5, (double) y + .5)),
  state(initialState),
  health(initialHealth) { }

Entity::Entity(StaticEntity staticEntity) :
  uid(staticEntity.uid),
  pos(staticEntity.x, staticEntity.y),
  state(S_STAND),
  health(100) { }

bool Entity::is(UID uid) {
  return uid.is(uid);
}

Entity instantiate_from_static(StaticEntity staticEntity) {
  return Entity(staticEntity);
}

Entity instantiate_enemy(uint8_t x, uint8_t y) {
  return Entity(E_ENEMY, x, y, S_STAND, 100);
}

Entity instantiate_medikit(uint8_t x, uint8_t y) {
  return Entity(E_MEDIKIT, x, y, S_STAND, 0);
}

Entity instantiate_key(uint8_t x, uint8_t y) {
  return Entity(E_KEY, x, y, S_STAND, 0);
}

Entity instantiate_fireball(uint8_t x, uint8_t y, uint8_t direction) {
  return Entity(E_KEY, x, y, S_STAND, direction);
}