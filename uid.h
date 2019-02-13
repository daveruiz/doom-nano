#ifndef _uid_h
#define _uid_h

#define UID_null  0

typedef uint16_t UID;

UID create_uid(uint8_t type, uint8_t x, uint8_t y);
uint8_t uid_get_type(UID uid);

#endif
