#ifndef _uid_h
#define _uid_h

#define UID_null  0
#define UID       uint16_t

UID uid(uint8_t type, uint8_t x, uint8_t y);
uint8_t uid_get_type(UID uid);

#endif
