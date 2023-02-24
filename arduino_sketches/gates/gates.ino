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

#define CMD_ENABLE_PASSTHROUGH 0x10
#define CMD_DISABLE_PASSTHROUGH 0x11
#define CMD_READ_PASSTHROUGH_STATE 0x12
#define CMD_SEND_CODE 0x13
#define CMD_RETRANSMIT_CODE 0x14

// #define DEBUG

uint8_t rollingCodeKeys1[5] = {
  0x25, 0xE1, 0x19, 0x98, 0x67
};

uint8_t rollingCodeKeys2[145] = {
  0x00, 0xCC, 0x6E, 0xAF, 0x20,
  0x01, 0xD3, 0x7D, 0xCB, 0x6C,
  0xA3, 0x1B, 0x0B, 0xE4, 0x91,
  0xF0, 0xAA, 0x2E, 0x27, 0x12,
  0xF3, 0xB1, 0x3D, 0x43, 0x5E,
  0x8D, 0xE9, 0xA4, 0x19, 0x0F,
  0xEC, 0xA2, 0x1D, 0x07, 0xDF,
  0x8A, 0xE3, 0x93, 0xF4, 0xB3,
  0x39, 0x4B, 0x6D, 0xAD, 0x24,
  0x18, 0xFF, 0xCE, 0x62, 0x94,
  0xF6, 0xBF, 0x46, 0x50, 0x67,
  0x9A, 0x0C, 0xE6, 0x9D, 0x06,
  0xD9, 0x86, 0xD8, 0x77, 0xB8,
  0x33, 0x38, 0x32, 0x3E, 0x41,
  0x5A, 0x85, 0xDA, 0x84, 0xD4,
  0x7F, 0xCF, 0x64, 0x90, 0xEF,
  0xA8, 0x11, 0xF1, 0xB5, 0x35,
  0x34, 0x3A, 0x49, 0x69, 0xA5,
  0x17, 0xFD, 0xCA, 0x6A, 0xA7,
  0x13, 0xF5, 0xBD, 0x42, 0x58,
  0x76, 0xBE, 0x40, 0x45, 0x52,
  0x7A, 0xC1, 0x5B, 0x83, 0xD6,
  0x73, 0xB0, 0x22, 0x1C, 0x09,
  0xE0, 0x88, 0xDC, 0x80, 0xCD,
  0x60, 0x89, 0xE1, 0x97, 0xFC,
  0xC4, 0x5D, 0x8F, 0xED, 0xAC,
  0x2A, 0x2F, 0x21, 0x1E, 0x05,
  0xDB, 0x82, 0xD0, 0x66, 0x9C,
  0x08, 0xDD, 0x8E, 0xEB, 0xA0
};

uint8_t transmit_data[] = { 0x06, 0xFF, 0xE8, 0xFC, 0xBF, 0x00, 0x00 };
uint8_t buffer[2] = { 0x00, 0x00 };

int rollingCodeIndex1 = 0;
int rollingCodeIndex2 = 0;

bool passthrough_enabled = true;
bool sending_code = false;

long codeReceivedTime = -1000;

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
  wdt_reset();
}

void callback_anycode(const BitVector *recorded) {
  if (recorded->get_nb_bits() != 51)
    return;
#ifdef DEBUG
  Serial.println("Code received");
#endif
  codeReceivedTime = millis();
  transmit_data[0] = recorded->get_nth_byte(6);
  transmit_data[1] = recorded->get_nth_byte(5);
  transmit_data[2] = recorded->get_nth_byte(4);
  transmit_data[3] = recorded->get_nth_byte(3);
  transmit_data[4] = recorded->get_nth_byte(2);
  transmit_data[5] = recorded->get_nth_byte(1);
  transmit_data[6] = recorded->get_nth_byte(0);
}

void sendCode() {
#ifdef DEBUG
  Serial.println("Sending code");
#endif
  Wire.end();
  sending_code = true;
  transmit_data[0] = 0x06;
  transmit_data[1] = 0xFF;
  transmit_data[2] = 0xE8;
  transmit_data[3] = 0xFC;
  transmit_data[4] = 0xBF;
  transmit_data[5] = rollingCodeKeys1[rollingCodeIndex1];
  transmit_data[6] = rollingCodeKeys2[rollingCodeIndex2];

  rollingCodeIndex1++;
  rollingCodeIndex2++;
  if (rollingCodeIndex1 > 4) {
    rollingCodeIndex1 = 0;
  }
  if (rollingCodeIndex2 > 145) {
    rollingCodeIndex2 = 0;
  }

  transmitter->send(sizeof(transmit_data), transmit_data);
  sending_code = false;
  Wire.begin(I2C_ADDRESS);
}

void retransmitCode() {
  if (passthrough_enabled) {
#ifdef DEBUG
    Serial.println("Retransmitting code");
#endif
    Wire.end();
    sending_code = true;
    transmitter->send(sizeof(transmit_data), transmit_data);
    sending_code = false;
    Wire.begin(I2C_ADDRESS);
  }
}

void codeReceived() {
  if (millis() - codeReceivedTime >= 1000) {
    buffer[1] |= (0 << 6);
  } else {
    buffer[1] |= (1 << 6);
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
    case CMD_ENABLE_PASSTHROUGH:
#ifdef DEBUG
      Serial.println("Passthrough enabled");
#endif
      passthrough_enabled = true;
      break;
    case CMD_DISABLE_PASSTHROUGH:
#ifdef DEBUG
      Serial.println("Passthrough disabled");
#endif
      passthrough_enabled = false;
      break;
    case CMD_READ_PASSTHROUGH_STATE:
      buffer[0] = passthrough_enabled;
#ifdef DEBUG
      Serial.print("Sending passthrough state: ");
      Serial.println(passthrough_enabled);
#endif
      break;
    case CMD_SEND_CODE:
      sendCode();
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