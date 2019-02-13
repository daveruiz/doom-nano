#ifndef _player_h
#define _player_h

#include "coords.h";

#define create_player(x, y)   { \
    coords((double) x + 0.5, (double) y + 0.5), \
    coords(1, 0), \
    coords(0, -0.66), \
    0, \
    100,  \
  }

struct Player { 
  Coords pos;
  Coords dir;
  Coords plane;
  double velocity;
  uint8_t health;
  uint8_t keys;  
};

#endif
