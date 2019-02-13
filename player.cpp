#include <stdint.h>
#include "player.h"
#include "coords.h"

Player::Player(uint8_t x,  uint8_t y): 
    pos({ (double) x + 0.5, (double) y + 0.5 }),
    dir({ 1, 0 }),
    plane({ 0, -0.66 }),
    velocity(0),
    health(100) { }