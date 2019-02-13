#include <stdint.h>
#include "static_entity.h"
#include "entity.h"
#include "coords.h"
#include "uid.h"

StaticEntity::StaticEntity(): 
  uid(UID_null),
  x(0),
  y(0),
  active(false) { }

StaticEntity::StaticEntity(uint8_t type, uint8_t x,  uint8_t y):
  uid(UID(type, x, y)),
  x(x),
  y(y),
  active(true) { }

StaticEntity::StaticEntity(UID uid, uint8_t x,  uint8_t y, bool active):
  uid(uid),
  x(x),
  y(y),
  active(active) { }

bool StaticEntity::is(UID uid) {
  return this->uid.is(uid);
}
