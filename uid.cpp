#include <stdint.h>
#include "uid.h"
#include "constants.h"

UID::UID() : value(0) { }

UID::UID(uint8_t type, uint8_t x,  uint8_t y) 
  : value((y * LEVEL_WIDTH + x) * 16 + (type & 0b00001111)) { }

uint8_t UID::getType() {
  return (uint8_t) (value % 16);
}

bool UID::isNull() {
  return value == 0;
}

bool UID::is(UID other) {
  this->value == other.value;
}
