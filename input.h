#ifndef _input_h
#define _input_h

class Input {
  private:
    uint8_t p_up;
    uint8_t p_down;
    uint8_t p_left;
    uint8_t p_right;
    uint8_t p_fire;
  
  public:
    bool up();
    bool down();
    bool left();
    bool right();
    bool fire();

    Input(uint8_t pin_left, uint8_t pin_right, uint8_t pin_up, uint8_t pin_down, uint8_t pin_fire);
};

#endif

