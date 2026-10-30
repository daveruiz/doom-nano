#include <Arduino.h>
#include "input.h"
#include "constants.h"

#ifdef SNES_CONTROLLER
uint16_t buttons = 0;

void input_setup() {
  pinMode(DATA_CLOCK, OUTPUT);
  digitalWrite(DATA_CLOCK, HIGH);
  pinMode(DATA_LATCH, OUTPUT);
  digitalWrite(DATA_LATCH, LOW);
  pinMode(DATA_SERIAL, OUTPUT);
  digitalWrite(DATA_SERIAL, HIGH);
  pinMode(DATA_SERIAL, INPUT);
}

void getControllerData(void) {
  digitalWrite(DATA_LATCH, HIGH);
  delayMicroseconds(12);
  digitalWrite(DATA_LATCH, LOW);
  delayMicroseconds(6);
  buttons = 0;
  for (uint8_t i = 0; i < 16; ++i) {
    digitalWrite(DATA_CLOCK, LOW);
    delayMicroseconds(6);
    buttons |= !digitalRead(DATA_SERIAL) << i;
    digitalWrite(DATA_CLOCK, HIGH);
    delayMicroseconds(6);
  }
}

float input_move() {
  return (buttons & UP) ? 1.0f : ((buttons & DOWN) ? -1.0f : 0.0f);
}

float input_turn() {
  return (buttons & RIGHT) ? 1.0f : ((buttons & LEFT) ? -1.0f : 0.0f);
}

bool input_fire() {
  return buttons & Y;
}

bool input_start() {
  return buttons & START;
}

#else

void input_setup() {
  pinMode(K_FIRE, INPUT_PULLUP);
}

// Convert one analog axis into a normalized value from -1.0 to +1.0.
// The dead zone suppresses small ADC fluctuations around the center.
static float readJoystickAxis(uint8_t pin, bool invert) {
  int value = analogRead(pin);
  int offset = value - JOYSTICK_CENTER;
  int magnitude = abs(offset);

  if (magnitude <= JOYSTICK_DEADZONE) return 0.0f;

  float result;
  if (offset > 0) {
    result = (float)(magnitude - JOYSTICK_DEADZONE) /
             (1023 - JOYSTICK_CENTER - JOYSTICK_DEADZONE);
  } else {
    result = -(float)(magnitude - JOYSTICK_DEADZONE) /
             (JOYSTICK_CENTER - JOYSTICK_DEADZONE);
  }

  result = constrain(result, -1.0f, 1.0f);
  return invert ? -result : result;
}

float input_move() {
  // On common joystick modules, pushing forward lowers the Y reading.
  return readJoystickAxis(JOYSTICK_Y_PIN, true);
}

float input_turn() {
  // Positive X means turning right.
  return readJoystickAxis(JOYSTICK_X_PIN, false);
}

bool input_fire() {
  return digitalRead(K_FIRE) == LOW;
}
#endif
