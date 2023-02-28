#include "RF433recv.h"
#include "RF433send.h"
#include <Arduino.h>
#include <Wire.h>
#include <avr/wdt.h>

#define I2C_ADDRESS 0x10
#define PIN_RFIN 2
#define PIN_RFOUT 4

#define CMD_READ 0x01
#define CMD_SETUP_PINS 0x02
#define CMD_SETUP_INPUT_PULLUP_PINS 0x03
#define CMD_WRITE_DIGITAL_HIGH 0x04
#define CMD_WRITE_DIGITAL_LOW 0x05
#define CMD_RESTORE_OUTPUTS 0x06

#define CMD_RETRANSMIT_CODE 0x14

//#define DEBUG

uint8_t transmit_data[] = { 0x06, 0xFF, 0xE8, 0xFC, 0xBF, 0x00, 0x00 };
uint8_t buffer[2] = { 0x00, 0x00 };

bool sending_code = false;

long codeReceivedTime = -1000;
long singleGateReceivedTime = -1000;

RF_manager receiver(PIN_RFIN, 0);
RfSend *transmitter;

void setup() {
  wdt_disable();
#ifdef DEBUG
  Serial.begin(115200);
#endif
  pinMode(PIN_RFIN, INPUT);
  pinMode(PIN_RFOUT, OUTPUT);

  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest);
  Wire.onReceive(onReceive);

  receiver.register_Receiver(RFMOD_TRIBIT_INVERTED, 25460, 2700, 808, 400, 400,
                             796, 0, 0, 380, 25460, 51, callback_anycode, 1000);
  transmitter =
    rfsend_builder(RfSendEncoding::TRIBIT_INVERTED, PIN_RFOUT, 0, 4, nullptr,
                   25460, 2700, 808, 400, 400, 800, 0, 0, 380, 25460, 51);
  receiver.activate_interrupts_handler();
  wdt_enable(WDTO_2S);
}

void loop() {
  receiver.do_events();
  codeReceived();
  singleGateReceived();
  wdt_reset();
}

void callback_anycode(const BitVector *recorded) {
  if (recorded->get_nb_bits() != 51)
    return;
#ifdef DEBUG
  Serial.println("Code received");
#endif

  transmit_data[0] = recorded->get_nth_byte(6);
  transmit_data[1] = recorded->get_nth_byte(5);
  transmit_data[2] = recorded->get_nth_byte(4);
  transmit_data[3] = recorded->get_nth_byte(3);
  transmit_data[4] = recorded->get_nth_byte(2);
  transmit_data[5] = recorded->get_nth_byte(1);
  transmit_data[6] = recorded->get_nth_byte(0);

  switch (transmit_data[4]) {
    case 0xBF:
    case 0xEF:
      singleGateReceivedTime = millis();
      break;
    case 0x7F:
    case 0xDF:
      codeReceivedTime = millis();
      break;
  }

#ifdef DEBUG
  Serial.print("Code received: ");
  Serial.print(transmit_data[0], HEX);
  Serial.print(" ");
  Serial.print(transmit_data[1], HEX);
  Serial.print(" ");
  Serial.print(transmit_data[2], HEX);
  Serial.print(" ");
  Serial.print(transmit_data[3], HEX);
  Serial.print(" ");
  Serial.print(transmit_data[4], HEX);
  Serial.print(" ");
  Serial.print(transmit_data[5], HEX);
  Serial.print(" ");
  Serial.println(transmit_data[6], HEX);
#endif
}


void retransmitCode() {
#ifdef DEBUG
    Serial.println("Retransmitting code");
#endif
    Wire.end();
    sending_code = true;
    transmitter->send(sizeof(transmit_data), transmit_data);
    sending_code = false;
    Wire.begin(I2C_ADDRESS);
}

void codeReceived() {
  if (millis() - codeReceivedTime >= 1000) {
    buffer[1] |= (0 << 6);
  } else {
    buffer[1] |= (1 << 6);
  }
}

void singleGateReceived() {
  if (millis() - singleGateReceivedTime >= 1000) {
    buffer[1] |= (0 << 7);
  } else {
    buffer[1] |= (1 << 7);
  }
}

void readDigital() {
  if (!sending_code) {
    buffer[0] = 0;
    buffer[1] = 0;
    for (uint8_t i = 0; i < 8; i++) {
      if (i == PIN_RFIN) {
        if (digitalRead(A0))
          buffer[0] |= (1 << i);
      } else if (i == PIN_RFOUT) {
        if (digitalRead(A1))
          buffer[0] |= (1 << i);
      } else {
        if (digitalRead(i))
          buffer[0] |= (1 << i);
      }
    }
    for (uint8_t i = 0; i < 6; i++) {
      if (digitalRead(i + 8))
        buffer[1] |= (1 << i);
    }
  }
}

void onRequest() {
  Wire.write(buffer, 2);
}

void onReceive(int numBytes) {
  uint8_t pin = -1;
  switch (Wire.read()) {
    case CMD_READ:
      readDigital();
      break;
    case CMD_SETUP_PINS:
      setupPins(Wire.read(), Wire.read());
      break;
    case CMD_SETUP_INPUT_PULLUP_PINS:
      setupInputPullupPins(Wire.read(), Wire.read());
      break;
    case CMD_RESTORE_OUTPUTS:
      restoreOutputs(Wire.read(), Wire.read());
      break;
    case CMD_WRITE_DIGITAL_LOW:
      pin = Wire.read();
#ifdef DEBUG
      Serial.print("Setting pin: ");
      Serial.print(pin);
      Serial.println(" LOW");
#endif
      digitalWrite(pin, LOW);
      break;
    case CMD_WRITE_DIGITAL_HIGH:
      pin = Wire.read();
#ifdef DEBUG
      Serial.print("Setting pin: ");
      Serial.print(pin);
      Serial.println(" HIGH");
#endif
      digitalWrite(pin, HIGH);
      break;
    case CMD_RETRANSMIT_CODE:
      retransmitCode();
      break;
  }
}

uint8_t getPin(uint8_t pin) {
  switch (pin) {
    case 2:
      return A0;
    case 4:
      return A1;
    default:
      return pin;
  }
}

void setupPins(uint8_t data1, uint8_t data0) {
  for (uint8_t i = 0; i < 8; i++) {
    bool mode = data0 & (1 << i);
    uint8_t pin = getPin(i);
#ifdef DEBUG
    Serial.print("Setting pin: ");
    Serial.print(pin);
    Serial.print(" as: ");
    Serial.println((mode) ? "OUTPUT" : "INPUT");
#endif
    pinMode(pin, mode);
  }
  for (uint8_t i = 0; i < 6; i++) {
    bool mode = data1 & (1 << i);
#ifdef DEBUG
    Serial.print("Setting pin: ");
    Serial.print(i + 8);
    Serial.print(" as: ");
    Serial.println((mode) ? "OUTPUT" : "INPUT");
#endif
    pinMode(i + 8, mode);
  }
}

void setupInputPullupPins(uint8_t data1, uint8_t data0) {
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t pin = getPin(i);
    if (data0 & (1 << i)) {
#ifdef DEBUG
      Serial.print("Setting pin: ");
      Serial.print(pin);
      Serial.println(" as: INPUT_PULLUP ");
#endif
      pinMode(i, INPUT_PULLUP);
    }
  }

  for (uint8_t i = 0; i < 6; i++) {
    if (data1 & (1 << i)) {
#ifdef DEBUG
      Serial.print("Setting pin: ");
      Serial.print(i + 8);
      Serial.println(" as: INPUT_PULLUP ");
#endif
      pinMode(i + 8, INPUT_PULLUP);
    }
  }
}

void restoreOutputs(uint8_t data1, uint8_t data0) {
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t pin = getPin(i);
    if (data0 & (1 << i)) {
#ifdef DEBUG
      Serial.print("Restoring pin: ");
      Serial.print(pin);
      Serial.println(" as: HIGH ");
#endif
      digitalWrite(i, HIGH);
    }
  }

  for (uint8_t i = 0; i < 6; i++) {
    if (data1 & (1 << i)) {
#ifdef DEBUG
      Serial.print("Restoring pin: ");
      Serial.print(i + 8);
      Serial.println(" as: HIGH ");
#endif
      digitalWrite(i + 8, HIGH);
    }
  }
}