#ifndef COMMANDS_H
#define COMMANDS_H

#include "Arduino.h"
#include "motor.h"
#include "store.h"

enum class Command {
  UP_FOR_LIMIT,
  STOP_AND_SET_TOP_LIMIT,
  GO_DOWN_FOR_LIMIT,
  STOP_AND_SET_BOTTOM_LIMIT,
  OPEN,
  CLOSE,
  MOVE_BLINDS_TO,
  READ_RESPONSE,
  PREPARE_STATUS,
  PREPARE_SETTINGS,
  UPDATE_SETTINGS,
  RESET_BOTTOM,
  Command_MAX = RESET_BOTTOM
};

class Commands {
public:
  Commands(Motor* motor0, Motor* motor1, Store* store0, Store* store1);
  int onCommand(uint8_t* command, int length, uint8_t* nextPayload);
  void tick();
private:
  Motor* _motor0;
  Motor* _motor1;
  Store* _store0;
  Store* _store1;
  Motor* getMotor(uint8_t side);
  Store* getStore(uint8_t side);
};
#endif