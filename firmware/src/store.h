#ifndef STORE_H
#define STORE_H

#include "Arduino.h"
#include "settings.h"

class Store {
public:
  Store(int motorId, Settings* settings);
  void begin();
  void storeLastPosition(int lastPosition);
  int getLastPosition();
  void storeBottomLimit(int bottomLimit);
  int getBottomLimit();
  void storeDefaultMaxSpeed(float defaultMaxSpeed);
  float getDefaultMaxSpeed();
  void storeSettingsFromBytes(uint8_t* bytes);
  int writeSettingsToBytes(uint8_t* buffer);
  int getSlowdownPercent();
  int getManualSpeedUp();
  int getManualSpeedDown();
  int getPidKp();
  int getPidKd();
  int getPidKi();
private:
  int _motorId;
  Settings* _settings;
  void persistSettings();
  int cachedPosition;
};
#endif