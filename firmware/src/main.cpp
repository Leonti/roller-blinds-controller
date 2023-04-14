#include "Arduino.h"
#include <SPI.h>

#include "LittleFS.h"

#include "motor.h"

#include "settings.h"
#include "commands.h"

#define IS_LEFT true

Settings settings0 = Settings();
Store store0 = Store(0, &settings0);
Motor motor0 = Motor(
  IS_LEFT,
  &store0,
  9,
  8,
  10,
  16,
  20,
  21);

Settings settings1 = Settings();
Store store1 = Store(1, &settings1);
Motor motor1 = Motor(
  IS_LEFT,
  &store1,
  14,
  15,
  12,
  16,
  18,
  19);

void motor0Enc1handler() {
  motor0.handleEnc1();
}

void motor1Enc1handler() {
  motor1.handleEnc1();
}

Commands commands = Commands(&motor0, &motor1, &store0, &store1);
uint8_t nrfBuffer[32];

int16_t be16_to_cpu_signed(const uint8_t high, const uint8_t low) {
    int16_t r;
    uint16_t u = (unsigned)low | ((unsigned)high << 8);
    memcpy(&r, &u, sizeof r);
    return r;
}

void setup() {
  attachInterrupt(digitalPinToInterrupt(motor0.getEnc1()), motor0Enc1handler, RISING);
  attachInterrupt(digitalPinToInterrupt(motor1.getEnc1()), motor1Enc1handler, RISING);

  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }

  if (!LittleFS.begin()) {
    Serial.println("LittleFS failed to initialise");
  }

  Serial.println("Starting");
  store0.begin();
  store1.begin();
  motor0.begin();
  motor1.begin();

  Serial.printf("Command size is %d\n", Command::Command_MAX);

  Command command = static_cast<Command>(2);

  if (command == Command::GO_DOWN_FOR_LIMIT) {
    Serial.println("Correctly cast command");
  }

  uint8_t test[2];
  test[0] = 3;
  test[1] = 231;

  Serial.printf("Values from bytes: %d, %d\n", (int)test[0], (int)(test[1]));

  uint8_t high = highByte(-10234);
  uint8_t low = lowByte(-10234);
  int combined = (high << 8) + low;

  int16_t combined2 = be16_to_cpu_signed(high, low);


  Serial.printf("Integer from bytes: %d, %d", combined, combined2);

  // settings.bottom_limit = 1;
  // settings.slowdown_percent = 2;
  // settings.manual_speed_up = 3;
  // settings.manual_speed_down = 4;
  // settings.default_max_speed = 0.1;
  // settings.pid_kp = 0.2;
  // settings.pid_kd = 0.3;
  // settings.pid_ki = 0.4;

  // Serial.println(settings.toString());

  // char asBytes[15];
  // settings.get_settings_as_bytes(asBytes);

  // Settings another = Settings();
  // another.parse_settings_from_bytes(asBytes, 15);
  // Serial.println(another.toString());
}

unsigned long last_update_ms = millis();

const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data

boolean newData = false;

void recvWithEndMarker() {
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

uint8_t responseBuffer[32];

void processNewData() {
  if (newData == true) {
    Serial.printf("This just in ... '%s'\n", receivedChars);
    //Serial.println(receivedChars);
    newData = false;

    if (strcmp(receivedChars, "UP_FOR_LIMIT") == 0) {
      Serial.println("UP_FOR_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::UP_FOR_LIMIT);
      nrfBuffer[1] = 0;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "STOP_AND_SET_TOP_LIMIT") == 0) {
      Serial.println("STOP_AND_SET_TOP_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::STOP_AND_SET_TOP_LIMIT);
      nrfBuffer[1] = 0;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "GO_DOWN_FOR_LIMIT") == 0) {
      Serial.println("GO_DOWN_FOR_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::GO_DOWN_FOR_LIMIT);
      nrfBuffer[1] = 0;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "STOP_AND_SET_BOTTOM_LIMIT") == 0) {
      Serial.println("STOP_AND_SET_BOTTOM_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::STOP_AND_SET_BOTTOM_LIMIT);
      nrfBuffer[1] = 0;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "open") == 0) {
      Serial.println("OPEN");
      nrfBuffer[0] = static_cast<uint8_t>(Command::OPEN);
      nrfBuffer[1] = 0;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "close") == 0) {
      Serial.println("CLOSE");
      nrfBuffer[0] = static_cast<uint8_t>(Command::CLOSE);
      nrfBuffer[1] = 0;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "status") == 0) {
      Serial.println("STATUS");
      nrfBuffer[0] = static_cast<uint8_t>(Command::PREPARE_STATUS);
      nrfBuffer[1] = 0;
      int responseLength = commands.onCommand(nrfBuffer, 2, responseBuffer);
      if (responseLength != 9) {
        Serial.println("Invalid response length");
      }
      int position = be16_to_cpu_signed(responseBuffer[0], responseBuffer[1]);
      int setpoint = be16_to_cpu_signed(responseBuffer[2], responseBuffer[3]);
      bool goalReached = responseBuffer[4];
      int directionChanges = be16_to_cpu_signed(responseBuffer[5], responseBuffer[6]);
      int falseTriggers = be16_to_cpu_signed(responseBuffer[7], responseBuffer[8]);
      Serial.printf("Position: %d, setpoint: %d, goal reached: %d, direction changes: %d, falseTriggers: %d\n", position, setpoint, goalReached, directionChanges, falseTriggers);

    } else if (strcmp(receivedChars, "settings") == 0) {
      Serial.println("SETTINGS");
      nrfBuffer[0] = static_cast<uint8_t>(Command::PREPARE_SETTINGS);
      nrfBuffer[1] = 0;
      int responseLength = commands.onCommand(nrfBuffer, 2, responseBuffer);
      if (responseLength != 15) {
        Serial.println("Invalid response length");
      }
      Settings settings = Settings();
      settings.parse_settings_from_bytes(responseBuffer, 15);
      Serial.println(settings.bottom_limit);
      Serial.println(settings.default_max_speed);
      Serial.println(settings.manual_speed_up);
      Serial.println(settings.manual_speed_down);
      Serial.println(settings.slowdown_percent);
      Serial.println(settings.pid_kp);
      Serial.println(settings.pid_ki);
      Serial.println(settings.pid_kd);
      //Serial.println(settings.toString());
    } else {
      Serial.println("Unknown command");
    }
  }
}

void loop() {

  if (millis() - last_update_ms > 3000) {
    last_update_ms = millis();
    //    Serial.printf("Motor0 pos: %d, ft: %d, dc: %d, Motor1 pos: %d, ft: %d, dc: %d,\n", motor0.getPos(), motor0.getFalseTriggers(), motor0.getDirectionChanges(), motor1.getPos(), motor1.getFalseTriggers(), motor1.getDirectionChanges());

    // int pos = store0.getLastPosition();
    // Serial.printf("Last stored position %d\n", pos);
    // store0.storeLastPosition(-12);
    // pos = store0.getLastPosition();
    // Serial.printf("Last stored position after update %d\n", pos);
  }

  recvWithEndMarker();
  processNewData();

  commands.tick();
}