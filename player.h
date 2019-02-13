#ifndef _player_h
#define _player_h

#include "coords.h";

class Player { 
  public:
    Coords pos;
    Coords dir;
    Coords plane;
    double velocity;
    uint8_t health;
    uint8_t keys;  

    Player(uint8_t x, uint8_t y);
};

#endif
