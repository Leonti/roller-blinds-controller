#ifndef SETTINGS_H
#define SETTINGS_H

#include "Arduino.h"

class Settings {

public:
  uint32_t bottom_limit = 0;
  uint8_t slowdown_percent = 20;
  uint8_t manual_speed_up = 70;
  uint8_t manual_speed_down = 50;
  float default_max_speed = 0;
  float pid_kp = 175;
  float pid_kd = 5;
  float pid_ki = 4;

  void get_settings_as_bytes(uint8_t* bytes);
  void parse_settings_from_bytes(uint8_t* bytes, int length);
  String toString();

private:
  void float_to_bytes(float value, uint8_t* bytes);
  float bytes_to_float(uint8_t* bytes);
};
#endif