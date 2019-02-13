#include <stdint.h>
#include <Arduino.h>
#include "input.h"
#include "constants.h"

Input::Input(uint8_t pin_left, uint8_t pin_right, uint8_t pin_up, uint8_t pin_down, uint8_t pin_fire) : 
  p_left(pin_left), p_right(pin_right), p_up(pin_up), p_down(pin_down), p_fire(pin_fire) 
{
  // Enable pin inputs
  // todo: use INPUT_PULLUP
  pinMode(pin_left, INPUT);
  pinMode(pin_right, INPUT);
  pinMode(pin_up, INPUT);
  pinMode(pin_down, INPUT);
  pinMode(pin_fire, INPUT);  
};

bool Input::left() {
  return digitalRead(p_left) == HIGH;
};

bool Input::right() {
  return digitalRead(p_right) == HIGH;
};

bool Input::up() {
  return digitalRead(p_up) == HIGH;
};

bool Input::down() {
  return digitalRead(p_down) == HIGH;
};

bool Input::fire() {
  return digitalRead(p_fire) == HIGH;
};
