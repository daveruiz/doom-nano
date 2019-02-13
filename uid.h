#ifndef _uid_h
#define _uid_h

#define UID_null  UID()

class UID {
  private:
    uint16_t value;

  public:
    UID();
    UID(uint8_t type, uint8_t x, uint8_t y);
    uint8_t getType();
    bool isNull();
    bool is(UID other);
};

#endif
