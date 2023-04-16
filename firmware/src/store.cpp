#include "store.h"

#include "LittleFS.h"

Store::Store(int motorId, Settings* settings) {
  _motorId = motorId;
  _settings = settings;
}

void Store::begin() {
  File f = LittleFS.open("settings_" + String(_motorId), "r");
  uint8_t buffer[15];
  if (f) {
    f.read(buffer, 15);
    f.close();
    _settings->parse_settings_from_bytes(buffer, 15);
  }  
}

void Store::storeSettingsFromBytes(uint8_t* bytes) {
  _settings->parse_settings_from_bytes(bytes, 15);
  persistSettings();
}

int Store::writeSettingsToBytes(uint8_t* buffer) {
  _settings->get_settings_as_bytes(buffer);
  return 15;
}

int32_t Store::getLastPosition() {
  File f = LittleFS.open("position_" + String(_motorId), "r");
  int32_t pos;
  if (f) {
    uint8_t buf[4];
    f.read(buf, 4);
    memcpy(&pos, &buf[0], 4);
  }
  cachedPosition = pos;
  return pos;
}

void Store::storeLastPosition(int32_t pos) {
  if (pos == cachedPosition) return; 
  File f = LittleFS.open("position_" + String(_motorId), "w");
  uint8_t buf[4];
  memcpy(&buf[0], &pos, 4);
  f.write(buf, 4);
  f.close();
}

void Store::persistSettings() {
  uint8_t buffer[15];
  _settings->get_settings_as_bytes(buffer);
  File f = LittleFS.open("settings_" + String(_motorId), "w");
  f.write(buffer, 15);
  f.close();
}

void Store::storeBottomLimit(int32_t bottomLimit) {
  Serial.printf("Storing bottom limit: %d\n", bottomLimit);
  _settings->bottom_limit = bottomLimit;
  persistSettings();
}

int32_t Store::getBottomLimit() {
  return _settings->bottom_limit;
}

void Store::storeDefaultMaxSpeed(float defaultMaxSpeed) {
  _settings->default_max_speed = defaultMaxSpeed;
  persistSettings();
}
float Store::getDefaultMaxSpeed() {
  return _settings->default_max_speed;
}

int Store::getSlowdownPercent() {
  return _settings->slowdown_percent;
}
int Store::getManualSpeedUp() {
  return _settings->manual_speed_up;
}
int Store::getManualSpeedDown() {
  return _settings->manual_speed_down;
}
int Store::getPidKp() {
  return _settings->pid_kp;
}
int Store::getPidKd() {
  return _settings->pid_kd;
}
int Store::getPidKi() {
  return _settings->pid_ki;
}