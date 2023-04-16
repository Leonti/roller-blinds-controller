#include "led.h"

Led::Led(int pin) {
  _pin = pin;
}

void Led::begin() {
  pinMode(_pin, OUTPUT);
}

void Led::update() {
  if (millis() - last_update_time_ms > 2000) {
    last_update_time_ms = millis();
    last_led_on_time_ms = millis();
    digitalWrite(_pin, HIGH);
  }

  if (last_led_on_time_ms != 0 && millis() - last_led_on_time_ms > 10) {
    last_led_on_time_ms = 0;
    digitalWrite(_pin, LOW);
  }
}