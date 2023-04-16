#include "Arduino.h"
#include <SPI.h>

#include "LittleFS.h"

#include "motor.h"

#include "settings.h"
#include "commands.h"
#include "led.h"
#include "pio_rotary_encoder.h"

#include "RF24.h"

#define IS_LEFT true
uint8_t listening_address[6] = "Node3";

#define CE_PIN 17
#define CSN_PIN 5
#define MISO_PIN 4
#define MOSI_PIN 7
#define SCK_PIN 6

#define LDO_ENABLE_PIN 0
#define NRF_IRQ_PIN 3

RF24 radio(CE_PIN, CSN_PIN);

int RotaryEncoder::rotation_motor_a = 0;
int RotaryEncoder::rotation_motor_b = 0;

// // create motor object
RotaryEncoder my_encoder_a(20, 21,   MOTOR_A_SM);
RotaryEncoder my_encoder_b(18, 19,   MOTOR_B_SM);

Settings settings0 = Settings();
Store store0 = Store(0, &settings0);
Motor motor0 = Motor(
  IS_LEFT,
  &store0,
  &my_encoder_a,
  9,
  8,
  10,
  16);

Settings settings1 = Settings();
Store store1 = Store(1, &settings1);
Motor motor1 = Motor(
  IS_LEFT,
  &store1,
  &my_encoder_b,
  14,
  15,
  12,
  16);

Commands commands = Commands(&motor0, &motor1, &store0, &store1);
Led led = Led(25);

uint8_t nrfBuffer[32];



int16_t be16_to_cpu_signed(const uint8_t high, const uint8_t low) {
    int16_t r;
    uint16_t u = (unsigned)low | ((unsigned)high << 8);
    memcpy(&r, &u, sizeof r);
    return r;
}

void setup() {
//  attachInterrupt(digitalPinToInterrupt(motor0.getEnc1()), motor0Enc1handler, RISING);
//  attachInterrupt(digitalPinToInterrupt(motor1.getEnc1()), motor1Enc1handler, RISING);


  // enable radio LDO
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LDO_ENABLE_PIN, OUTPUT);
  digitalWrite(LDO_ENABLE_PIN, HIGH);

  // SPI used for radio 
  SPI.setRX(MISO_PIN);
  SPI.setTX(MOSI_PIN);
  SPI.setSCK(SCK_PIN);
  SPI.setCS(5);
  SPI.begin();

  pinMode(NRF_IRQ_PIN, INPUT);

//  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
//  }

  if (!radio.begin()) {
    Serial.println("radio hardware is not responding!!\n");
  } else {
    Serial.println("Connected to radio with SPI");
    radio.openReadingPipe(1, listening_address);

    radio.setPALevel(RF24_PA_LOW);
    radio.setChannel(100);
    radio.setRetries(3, 5);
    radio.setDataRate(RF24_1MBPS);
    radio.enableDynamicPayloads();
    radio.enableAckPayload();
    
    // radio.writeAckPayload(1, &payload, sizeof(payload));
    radio.startListening();  // put radio in RX mode
  }


  if (!LittleFS.begin()) {
    Serial.println("LittleFS failed to initialise");
  }

  Serial.println("Starting");
  store0.begin();
  store1.begin();
  motor0.begin();
  motor1.begin();

  led.begin();

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

  Serial.printf("Store0 position: %d, bottomLimit: %d", store0.getLastPosition(), store0.getBottomLimit());

//  my_encoder_a.set_rotation(0);

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

uint8_t testedMotor = 1;

bool motorTestingDown = false;
bool motorTestingUp = false;

void processNewData() {
  if (newData == true) {
    Serial.printf("This just in ... '%s'\n", receivedChars);
    //Serial.println(receivedChars);
    newData = false;

    if (strcmp(receivedChars, "UP_FOR_LIMIT") == 0) {
      Serial.println("UP_FOR_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::UP_FOR_LIMIT);
      nrfBuffer[1] = testedMotor;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "STOP_AND_SET_TOP_LIMIT") == 0) {
      Serial.println("STOP_AND_SET_TOP_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::STOP_AND_SET_TOP_LIMIT);
      nrfBuffer[1] = testedMotor;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "GO_DOWN_FOR_LIMIT") == 0) {
      Serial.println("GO_DOWN_FOR_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::GO_DOWN_FOR_LIMIT);
      nrfBuffer[1] = testedMotor;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "STOP_AND_SET_BOTTOM_LIMIT") == 0) {
      Serial.println("STOP_AND_SET_BOTTOM_LIMIT");
      nrfBuffer[0] = static_cast<uint8_t>(Command::STOP_AND_SET_BOTTOM_LIMIT);
      nrfBuffer[1] = testedMotor;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "open") == 0) {
      Serial.println("OPEN");
      nrfBuffer[0] = static_cast<uint8_t>(Command::OPEN);
      nrfBuffer[1] = testedMotor;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "close") == 0) {
      Serial.println("CLOSE");
      nrfBuffer[0] = static_cast<uint8_t>(Command::CLOSE);
      nrfBuffer[1] = testedMotor;
      commands.onCommand(nrfBuffer, 2, responseBuffer);
    } else if (strcmp(receivedChars, "go50") == 0) {
      Serial.println("Go to 50 percent");
      nrfBuffer[0] = static_cast<uint8_t>(Command::MOVE_BLINDS_TO);
      nrfBuffer[1] = testedMotor;
      nrfBuffer[2] = 50;
      commands.onCommand(nrfBuffer, 3, responseBuffer);
    } else if (strcmp(receivedChars, "status") == 0) {
      Serial.println("STATUS");
      nrfBuffer[0] = static_cast<uint8_t>(Command::PREPARE_STATUS);
      nrfBuffer[1] = testedMotor;
      int responseLength = commands.onCommand(nrfBuffer, 2, responseBuffer);
      if (responseLength != 9) {
        Serial.println("Invalid response length");
      }
      int32_t position;
      int32_t setpoint;
      bool goalReached = responseBuffer[8];

      memcpy(&position, &responseBuffer[0], 4);
      memcpy(&setpoint, &responseBuffer[4], 4);

      Serial.printf("Position: %d, setpoint: %d, goal reached: %d\n", position, setpoint, goalReached);

    } else if (strcmp(receivedChars, "settings") == 0) {
      Serial.println("SETTINGS");
      nrfBuffer[0] = static_cast<uint8_t>(Command::PREPARE_SETTINGS);
      nrfBuffer[1] = testedMotor;
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
    } else if (strcmp(receivedChars, "test") == 0) {
      Serial.println("Testing motor");
      motor0.goToPositionPercent(100);
      motorTestingDown = true;
    } else {
      Serial.println("Unknown command");
    }
  }
}

void loop() {

  if (digitalRead(NRF_IRQ_PIN) == LOW && radio.available()) {
    Serial.println("New radio data received!");
    uint8_t payloadSize = radio.getDynamicPayloadSize();
    radio.read(&nrfBuffer, payloadSize);
    uint8_t ackSize = commands.onCommand(nrfBuffer, 2, responseBuffer);
    radio.writeAckPayload(1, &responseBuffer, ackSize);
  }

  if (motorTestingDown && motor0.isGoalReached()) {
    motorTestingDown = false;
    Serial.printf("Finished going down, direction changes\n");
    motor0.goToPositionPercent(0);
    motorTestingUp = true;
  } else if (motorTestingUp && motor0.isGoalReached()) {
    motorTestingUp = false;
    Serial.printf("Finished going up, direction changes\n");
  }

  if (millis() - last_update_ms > 5000) {
    last_update_ms = millis();
   // Serial.printf("Motor0 rotation %d\n", motor0.getPos());
    //Serial.printf("Motor1 rotation %d\n", motor1.getPos());

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
  led.update();
}