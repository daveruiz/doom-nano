#ifndef _coords_h
#define _coords_h

struct Coords {
  double x;
  double y;
};

Coords coords(double x, double y);
uint8_t coords_distance(Coords* a, Coords* b);

#endif

