#include "RF433recv.h"
#include "RF433send.h"
#include <Arduino.h>
#include <Wire.h>

#define PIN_RFIN 2
#define PIN_RFOUT 4
#define PIN_RELAY 13
#define I2C_ADDRESS 0x10
#define CMD_READ_DIGITAL 0x80
#define CMD_ENABLE_PASSTHROUGH 0x70
#define CMD_DISABLE_PASSTHROUGH 0x71
#define CMD_READ_PASSTHROUGH_STATE 0x72
#define CMD_SEND_CODE 0x73
#define CMD_RELAY_STATUS 0x74
#define CMD_RELAY_TURN_ON 0x75
#define CMD_RELAY_TURN_OFF 0x76

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

int rollingCodeIndex1 = 0;
int rollingCodeIndex2 = 0;
bool passthrough_enabled = true;
bool relay_status = false;
uint8_t buffer[2] = { 0x00, 0x00 };
long codeReceivedTime = 0;
bool sending_code = false;
RF_manager receiver(PIN_RFIN, 0);
RfSend *transmitter;

void setup() {
  for (int i = 0; i <= 13; i++) {
    pinMode(i, INPUT);
  }
  pinMode(PIN_RFOUT, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, false);

  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest);
  Wire.onReceive(onReceive);

  receiver.register_Receiver(RFMOD_TRIBIT_INVERTED, 25460, 2700, 808, 400, 400, 796, 0, 0, 380, 25460, 51, callback_anycode, 1000);
  transmitter = rfsend_builder(RfSendEncoding::TRIBIT_INVERTED, PIN_RFOUT, 0, 4, nullptr, 25460, 2700, 808, 400, 400, 800, 0, 0, 380, 25460, 51);
  receiver.activate_interrupts_handler();
}

void loop() {
  receiver.do_events();
  codeReceived();
}

void callback_anycode(const BitVector *recorded) {
  if (passthrough_enabled) {
    if (recorded->get_nb_bits() != 51)
      return;
    codeReceivedTime = millis();
    sending_code = true;
    transmit_data[0] = recorded->get_nth_byte(6);
    transmit_data[1] = recorded->get_nth_byte(5);
    transmit_data[2] = recorded->get_nth_byte(4);
    transmit_data[3] = recorded->get_nth_byte(3);
    transmit_data[4] = recorded->get_nth_byte(2);
    transmit_data[5] = recorded->get_nth_byte(1);
    transmit_data[6] = recorded->get_nth_byte(0);

    transmitter->send(sizeof(transmit_data), transmit_data);
    sending_code = false;
  }
}

void sendCode() {
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

void codeReceived() {
  if (millis() - codeReceivedTime >= 3000) {
    buffer[1] |= (0 << 6);
  } else {
    buffer[1] |= (1 << 6);
  }
}

void readDigital() {
  if (!sending_code) {
    buffer[0] = 0;
    buffer[1] = 0;
    for (int i = 0; i < 8; i++) {
      if (i != PIN_RFIN && i != PIN_RFOUT) {
        if (digitalRead(i)) buffer[0] |= (1 << i);
      }
    }
    for (int i = 0; i < 6; i++) {
      if (i + 8 != PIN_RELAY) {
        if (digitalRead(i + 8)) buffer[1] |= (1 << i);
      }
    }
  }
}

void onRequest() {
  Wire.write(buffer, 2);
}

void onReceive(int numBytes) {
  switch (Wire.read()) {
    case CMD_READ_DIGITAL:
      readDigital();
      break;
    case CMD_ENABLE_PASSTHROUGH:
      passthrough_enabled = true;
      break;
    case CMD_DISABLE_PASSTHROUGH:
      passthrough_enabled = false;
      break;
    case CMD_READ_PASSTHROUGH_STATE:
      buffer[0] = passthrough_enabled;
      break;
    case CMD_SEND_CODE:
      sendCode();
      break;
    case CMD_RELAY_STATUS:
      buffer[0] = relay_status;
      break;
    case CMD_RELAY_TURN_ON:
      relay_status = true;
      digitalWrite(PIN_RELAY, relay_status);
      break;
    case CMD_RELAY_TURN_OFF:
      relay_status = false;
      digitalWrite(PIN_RELAY, relay_status);
      break;
  }
}
