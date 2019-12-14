#ifndef _input_h
#define _input_h

enum BUTTONS {
  B = 0x0001,
  Y = 0x0002,
  SELECT = 0x0004,
  START = 0x0008,
  UP = 0x0010,
  DOWN = 0x0020,
  LEFT = 0x0040,
  RIGHT = 0x0080,
  A = 0x0100,
  X = 0x0200,
  LB = 0x0400,
  RB = 0x0800
};

void input_setup();
bool input_up();
bool input_down();
bool input_left();
bool input_right();
bool input_fire();

#ifdef SNES_CONTROLLER
bool input_start();
void getControllerData(void);
#endif

#endif
