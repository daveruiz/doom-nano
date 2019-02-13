#include <stdint.h>
#include "uid.h"
#include "constants.h"

UID create_uid(uint8_t type, uint8_t x, uint8_t y) {
  return (y * LEVEL_WIDTH + x) * 16 + (type & 0b00001111);
}
  
uint8_t uid_get_type(UID uid) {
  return uid % 16;
}