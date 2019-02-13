#ifndef _static_entity_h
#define _static_entity_h

#include <stdint.h>
#include "uid.h"

struct StaticEntity  { 
  UID uid;
  uint8_t x;
  uint8_t y;
  bool active;
};

StaticEntity create_static_entity(UID uid, uint8_t x,  uint8_t y, bool active);

#endif
