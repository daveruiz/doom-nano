#include <Arduino.h>
#include "input.h"
#include "constants.h"

bool input_left() {
  return digitalRead(K_LEFT) == HIGH;
};

bool input_right() {
  return digitalRead(K_RIGHT) == HIGH;
};

bool input_up() {
  return digitalRead(K_UP) == HIGH;
};

bool input_down() {
  return digitalRead(K_DOWN) == HIGH;
};

bool input_fire() {
  return digitalRead(K_FIRE) == HIGH;
};
