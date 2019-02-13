#ifndef _coords_h
#define _coords_h

class Coords  {
  public:
    double x;
    double y;

    Coords(double x, double y);
    uint8_t distanceTo(Coords other);
    bool isSame(Coords *other);
};

#endif

