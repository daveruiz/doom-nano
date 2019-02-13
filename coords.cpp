#include <stdint.h>
#include <math.h>
#include "coords.h"
#include "constants.h"

#define sq(val)   (val) * (val)

Coords::Coords(double x, double y): x(x), y(y) { };

uint8_t Coords::distanceTo(Coords other) {
  return sqrt(sq(x - other.x) + sq(y - other.y)) * DISTANCE_MULTIPLIER;
}

bool Coords::isSame(Coords *other) {
  return this == other;
}

