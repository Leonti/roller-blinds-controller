#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "Arduino.h"
#include "commands.h"

class Debugger {
public:
  Debugger(Commands* commands);
  void update();
private:
  boolean newData = false;
  const byte numChars = 32;
  char receivedChars[32]; // an array to store the received data from serial
  void recvWithEndMarker();
  void processNewData();
  uint8_t nrfBuffer[32];
  uint8_t responseBuffer[32];
  uint8_t testedMotor = 1;
  bool motorTestingDown = false;
  bool motorTestingUp = false;
  Commands* _commands;
  bool isGoalReached();
  void goToPosition(uint8_t position);
};
#endif