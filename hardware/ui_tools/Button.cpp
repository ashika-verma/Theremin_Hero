#include "Arduino.h"
#include "Button.h"

// Creates a new button for the pin, p
Button::Button(int p) {
  flag = 0;
  state = 0;
  pin = p;
  t_of_state_2 = millis(); //init
  t_of_button_change = millis(); //init
  debounce_time = 10;
  long_press_time = 1000;
  button_pressed = 0;
}

// reads the current value of the button 
// and updates this state
void Button::read() {
  uint8_t button_state = digitalRead(pin);
  button_pressed = !button_state;
}

// updates the current state of this button
// returns 0 on no press, 1 on short press, 
// or 2 on long press
int Button::update() {
  read();
  flag = 0;
  switch (state) {
    case 0:
      if (button_pressed) {
        state = 1;
        t_of_button_change = millis();
      }
      break;
    case 1:
      if (button_pressed) {
        if (millis() - t_of_button_change >= debounce_time) {
          state = 2;
          t_of_state_2 = millis();
        }
      } else {
        t_of_button_change = millis();
        state = 0;
      }
      break;
    case 2:
      if (button_pressed) {
        if (millis() - t_of_state_2 >= long_press_time) {
          state = 3;
        }
      } else {
        t_of_button_change = millis();
        state = 4;
      }
      break;
    case 3:
      if (!button_pressed) {
        t_of_button_change = millis();
        state = 4;
      }
      break;
    case 4:
      if (button_pressed) {
        if (millis() - t_of_state_2 < long_press_time) {
          state = 2;
        } else {
          state = 3;
        }
        t_of_button_change = millis();
      } else {
        if (millis() - t_of_button_change >= debounce_time) {
          state = 0;
          if (millis() - t_of_state_2 < long_press_time) {
            flag = 1;
          } else {
            flag = 2;
          }
        }
      }
  }
  return flag;
}