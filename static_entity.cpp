#include <stdint.h>
#include "static_entity.h"
#include "uid.h"

StaticEntity crate_static_entity(UID uid, uint8_t x,  uint8_t y, bool active) {
  return { uid, x, y, active };
}