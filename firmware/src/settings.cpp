#include "Arduino.h"

#include "settings.h"

void Settings::get_settings_as_bytes(uint8_t* bytes) {
  memcpy(&bytes[0], &this->bottom_limit, 4);
  bytes[4] = this->slowdown_percent;
  bytes[5] = this->manual_speed_up;
  bytes[6] = this->manual_speed_down;

  uint8_t floatBuffer[2];
  float_to_bytes(this->default_max_speed, floatBuffer);
  bytes[7] = floatBuffer[0];
  bytes[8] = floatBuffer[1];

  float_to_bytes(this->pid_kp, floatBuffer);
  bytes[9] = floatBuffer[0];
  bytes[10] = floatBuffer[1];

  float_to_bytes(this->pid_kd, floatBuffer);
  bytes[11] = floatBuffer[0];
  bytes[12] = floatBuffer[1];

  float_to_bytes(this->pid_ki, floatBuffer);
  bytes[13] = floatBuffer[0];
  bytes[14] = floatBuffer[1];
}

void Settings::parse_settings_from_bytes(uint8_t* bytes, int length) {
  memcpy(&this->bottom_limit, &bytes[0], 4);
  this->slowdown_percent = length >= 5 ? bytes[4] : 20;
  this->manual_speed_up = length >= 6 ? bytes[5] : 70;
  this->manual_speed_down = length >= 7 ? bytes[6] : 50;
  this->default_max_speed = length >= 9 ? bytes_to_float(&bytes[7]) : 0.0;
  this->pid_kp = length >= 11 ? bytes_to_float(&bytes[9]) : 175.0;
  this->pid_kd = length >= 13 ? bytes_to_float(&bytes[11]) : 5.0;
  this->pid_ki = length >= 15 ? bytes_to_float(&bytes[13]) : 4.0;
}

String Settings::toString() {
  char buffer[255];

  String maxSpeed = String(this->default_max_speed, 2);
  char maxSpeedChars[maxSpeed.length() + 1];
  maxSpeed.toCharArray(maxSpeedChars, maxSpeed.length() + 1);

  String kp = String(this->pid_kp, 2);
  char kpChars[kp.length() + 1];
  kp.toCharArray(kpChars, kp.length() + 1);

  String kd = String(this->pid_kd, 2);
  char kdChars[kd.length() + 1];
  kd.toCharArray(kdChars, kd.length() + 1);

  String ki = String(this->pid_ki, 2);
  char kiChars[ki.length() + 1];
  ki.toCharArray(kiChars, ki.length() + 1);

  sprintf(buffer, "Current settings:\nBottom limit: %d\nSlowdown percent: %d\nSpeed up: %d\nSpeed down: %d\nMax speed: %skP%s\nkD%s\nkI%s",
    this->bottom_limit,
    this->slowdown_percent,
    this->manual_speed_up,
    this->manual_speed_down,
    maxSpeedChars,
    kpChars,
    kdChars,
    kiChars);
  return buffer;
}

void Settings::float_to_bytes(float value, uint8_t* bytes) {
  int integer_part = value;
  float fractional_part = value - integer_part;
  int fractional_scaled = fractional_part * 100;
  bytes[0] = integer_part;
  bytes[1] = fractional_scaled;
}

float Settings::bytes_to_float(uint8_t* bytes) {
  int integer_part = bytes[0];
  int fractional_scaled = bytes[1];
  float fractional_part = fractional_scaled / 100.0;
  return integer_part + fractional_part;
}