#include <stdint.h>
#include "entity.h"
#include "static_entity.h"
#include "coords.h"
#include "uid.h"
#include "constants.h"

Entity create_entity(uint8_t type, uint8_t x,  uint8_t y, uint8_t initialState, uint8_t initialHealth) {
  UID uid = create_uid(type, x, y);
  Coords pos = coords((double) x + .5, (double) y + .5);
  Entity new_entity = { uid, pos, initialState, initialHealth, 0, 0 };
  return new_entity;
}