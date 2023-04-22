#include "Arduino.h"
#include <SPI.h>

#include "LittleFS.h"

#include "motor.h"

#include "settings.h"
#include "commands.h"
#include "led.h"
#include "debugger.h"
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
RotaryEncoder my_encoder_a(20, 21, MOTOR_A_SM);
RotaryEncoder my_encoder_b(18, 19, MOTOR_B_SM);

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
Debugger debugger = Debugger(&commands);

uint8_t nrfBuffer[32];
uint8_t responseBuffer[32];

volatile bool new_nrf_data = false;

void nrfInterruptHandler() {
  new_nrf_data = true;
}

void setup() {
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
  attachInterrupt(digitalPinToInterrupt(NRF_IRQ_PIN), nrfInterruptHandler, FALLING);

  // uncomment to wait for serial, useful for debugging
  //  while (!Serial) {
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
    // let IRQ pin only trigger on "data ready" event in RX mode
    radio.maskIRQ(1, 1, 0);

    radio.flush_tx();
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
}

void loop() {

  if (new_nrf_data) {
    bool tx_ds, tx_df, rx_dr;
    radio.whatHappened(tx_ds, tx_df, rx_dr);

    if (radio.available()) {
      if (radio.rxFifoFull()) {
        Serial.println("RX FIFO FULL");
      }

      new_nrf_data = false;
      uint8_t payloadSize = radio.getDynamicPayloadSize();
      radio.read(&nrfBuffer, payloadSize);
      uint8_t ackSize = commands.onCommand(nrfBuffer, 2, responseBuffer);
      if (radio.available()) {
        radio.read(&nrfBuffer, radio.getDynamicPayloadSize());
        uint8_t ackSize = commands.onCommand(nrfBuffer, 2, responseBuffer);
      }
      if (!radio.writeAckPayload(1, &responseBuffer, ackSize)) {
        Serial.println("ACK payload wasn't written");
      }
    }
  }

  debugger.update();

  commands.tick();
  led.update();
}