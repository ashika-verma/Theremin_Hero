#ifndef Button_h
#define Button_h
#include "Arduino.h"

class Button {
  private:
    uint32_t t_of_state_2;
    uint32_t t_of_button_change;
    uint32_t debounce_time;
    uint32_t long_press_time;
    uint8_t pin;
    uint8_t flag;
    bool button_pressed;
    uint8_t state;

  public:
    Button(int p);
    void read();
    int update();
};
#endif