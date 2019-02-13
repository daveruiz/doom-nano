#ifndef _static_entity_h
#define _static_entity_h

#include "coords.h"
#include "uid.h"

class StaticEntity  { 
  public:
    UID uid;
    uint8_t x;
    uint8_t y;
    bool active;

    StaticEntity();
    StaticEntity(uint8_t type, uint8_t x,  uint8_t y);
    StaticEntity(UID uid, uint8_t x,  uint8_t y, bool active);
};

#endif
