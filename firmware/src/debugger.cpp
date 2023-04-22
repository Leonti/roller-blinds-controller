#include "debugger.h"

Debugger::Debugger(Commands* commands) {
  _commands = commands;
}

void Debugger::recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  // if (Serial.available() > 0) {
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    //    Serial.printf("Received char %d", rc);
    if (rc != endMarker) {
      if (rc != 13) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
    }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void Debugger::processNewData() {
  if (newData == true) {
    Serial.printf("This just in ... '%s'\n", receivedChars);
    //Serial.println(receivedChars);
    newData = false;

    if (strcmp(receivedChars, "UP_FOR_LIMIT") == 0) {
      Serial.println("UP_FOR_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::UP_FOR_LIMIT);
      nrfBuffer[1] = testedMotor;
      _commands->onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "STOP_AND_SET_TOP_LIMIT") == 0) {
      Serial.println("STOP_AND_SET_TOP_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::STOP_AND_SET_TOP_LIMIT);
      nrfBuffer[1] = testedMotor;
      _commands->onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "GO_DOWN_FOR_LIMIT") == 0) {
      Serial.println("GO_DOWN_FOR_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::GO_DOWN_FOR_LIMIT);
      nrfBuffer[1] = testedMotor;
      _commands->onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "STOP_AND_SET_BOTTOM_LIMIT") == 0) {
      Serial.println("STOP_AND_SET_BOTTOM_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::STOP_AND_SET_BOTTOM_LIMIT);
      nrfBuffer[1] = testedMotor;
      _commands->onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "open") == 0) {
      Serial.println("OPEN");
      nrfBuffer[0] = static_cast<uint8_t>(Command::OPEN);
      nrfBuffer[1] = testedMotor;
      _commands->onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "close") == 0) {
      Serial.println("CLOSE");
      nrfBuffer[0] = static_cast<uint8_t>(Command::CLOSE);
      nrfBuffer[1] = testedMotor;
      _commands->onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "go50") == 0) {
      Serial.println("Go to 50 percent");
      nrfBuffer[0] = static_cast<uint8_t>(Command::MOVE_BLINDS_TO);
      nrfBuffer[1] = testedMotor;
      nrfBuffer[2] = 50;
      _commands->onCommand(nrfBuffer, 3, responseBuffer);
    } else if (strcmp(receivedChars, "status") == 0) {
      Serial.println("STATUS");
      nrfBuffer[0] = static_cast<uint8_t>(Command::PREPARE_STATUS);
      nrfBuffer[1] = testedMotor;
      int responseLength = _commands->onCommand(nrfBuffer, 2, responseBuffer);
      if (responseLength != 11) {
        Serial.println("Invalid response length");
      }
      int32_t position;
      int32_t setpoint;
      bool goalReached = responseBuffer[10];

      memcpy(&position, &responseBuffer[2], 4);
      memcpy(&setpoint, &responseBuffer[6], 4);

      Serial.printf("%d,%d, Position: %d, setpoint: %d, goal reached: %d\n", responseBuffer[0], responseBuffer[1], position, setpoint, goalReached);

    } else if (strcmp(receivedChars, "settings") == 0) {
      Serial.println("SETTINGS");
      nrfBuffer[0] = static_cast<uint8_t>(Command::PREPARE_SETTINGS);
      nrfBuffer[1] = testedMotor;
      int responseLength = _commands->onCommand(nrfBuffer, 2, responseBuffer);
      if (responseLength != 17) {
        Serial.println("Invalid response length");
      }
      Settings settings = Settings();
      settings.parse_settings_from_bytes(&responseBuffer[2], 15);
      Serial.println(settings.bottom_limit);
      Serial.println(settings.default_max_speed);
      Serial.println(settings.manual_speed_up);
      Serial.println(settings.manual_speed_down);
      Serial.println(settings.slowdown_percent);
      Serial.println(settings.pid_kp);
      Serial.println(settings.pid_ki);
      Serial.println(settings.pid_kd);
      //Serial.println(settings.toString());
    } else if (strcmp(receivedChars, "reset_bottom") == 0) {
      nrfBuffer[0] = static_cast<uint8_t>(Command::RESET_BOTTOM);
      nrfBuffer[1] = testedMotor;
      int responseLength = _commands->onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "test") == 0) {
      Serial.println("Testing motor");
      goToPosition(100);
      motorTestingDown = true;
    } else {
      Serial.println("Unknown command");
    }
  }
}

void Debugger::goToPosition(uint8_t position) {
  nrfBuffer[0] = static_cast<uint8_t>(Command::MOVE_BLINDS_TO);
  nrfBuffer[1] = testedMotor;
  nrfBuffer[2] = position;
  _commands->onCommand(nrfBuffer, 3, responseBuffer);
}

bool Debugger::isGoalReached() {
  nrfBuffer[0] = static_cast<uint8_t>(Command::PREPARE_STATUS);
  nrfBuffer[1] = testedMotor;
  int responseLength = _commands->onCommand(nrfBuffer, 2, responseBuffer);
  return responseBuffer[10];  
}

void Debugger::update() {
  recvWithEndMarker();
  processNewData();

  if (motorTestingDown && isGoalReached()) {
    motorTestingDown = false;
    Serial.printf("Finished going down, direction changes\n");
    goToPosition(100);
    motorTestingUp = true;
  } else if (motorTestingUp && isGoalReached()) {
    motorTestingUp = false;
    Serial.printf("Finished going up, direction changes\n");
  }
}