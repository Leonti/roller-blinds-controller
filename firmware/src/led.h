#ifndef LED_H
#define LED_H

#include "Arduino.h"

class Led {
public:
  Led(int pin);
  void begin();
  void update();
private:
  int _pin;
  uint32_t last_update_time_ms = 0;
  uint32_t last_led_on_time_ms = 0;
};
#endif