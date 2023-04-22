#include "commands.h"


Commands::Commands(Motor* motor0, Motor* motor1, Store* store0, Store* store1) {
  _motor0 = motor0;
  _motor1 = motor1;
  _store0 = store0;
  _store1 = store1;
}

Motor* Commands::getMotor(uint8_t side) {
  return side == 0 ? _motor0 : _motor1;
}

Store* Commands::getStore(uint8_t side) {
  return side == 0 ? _store0 : _store1;
}

int Commands::onCommand(uint8_t* data, int length, uint8_t* nextPayload) {
  // return first 2 bytes to correlate ack payload in the receiver
  nextPayload[0] = data[0];
  nextPayload[1] = data[1];
  if (data[0] > static_cast<int>(Command::Command_MAX)) {
    Serial.printf("Command id '%d' is out of bounds\n", data[0]);
    return 2;
  }
  Command command = static_cast<Command>(data[0]);
  uint8_t side = data[1];
  if (command == Command::PREPARE_SETTINGS) {
    return getStore(side)->writeSettingsToBytes(&nextPayload[2]) + 2;
  } else if (command == Command::PREPARE_STATUS) {
    return getMotor(side)->writeStatus(&nextPayload[2]) + 2;
  } else if (command == Command::READ_RESPONSE) {
    // Serial.println("Reading response, doing nothing");
  } else if (command == Command::UPDATE_SETTINGS) {
    if (length != 17) {
      Serial.println("Updating settings failed, invalid size of the message");
      return 2;
    }
    getStore(side)->storeSettingsFromBytes(&(data[2]));
  } else if (command == Command::UP_FOR_LIMIT) {
    Serial.println("Processing UP_FOR_LIMIT");
    getMotor(side)->goSetupUp();
  } else if (command == Command::STOP_AND_SET_TOP_LIMIT) {
    getMotor(side)->stopAndSetTopLimit();
  } else if (command == Command::GO_DOWN_FOR_LIMIT) {
    getMotor(side)->goSetupDown();
  } else if (command == Command::STOP_AND_SET_BOTTOM_LIMIT) {
    getMotor(side)->stopAndFinishSetup();
  } else if (command == Command::MOVE_BLINDS_TO) {
    if (length != 3) {
      Serial.println("MOVE_BLINDS_TO failed, invalid size of the message");
      return 2;
    }
    getMotor(side)->goToPositionPercent(data[2]);
  } else if (command == Command::OPEN) {
    getMotor(side)->goToPositionPercent(0);
  } else if (command == Command::CLOSE) {
    getMotor(side)->goToPositionPercent(100);
  } else if (command == Command::RESET_BOTTOM) {
    getMotor(side)->resetBottom();
  } else {
    Serial.printf("Unknown command for id %d\n", data[0]);
  }

  return 2;
}

void Commands::tick() {
  _motor0->update();
  _motor1->update();
}