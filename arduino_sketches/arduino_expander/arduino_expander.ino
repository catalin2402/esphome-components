#include <Arduino.h>
#include <Wire.h>
#include <avr/wdt.h>

#define I2C_ADDRESS 0x10

#define CMD_READ_DIGITAL 0x01
#define CMD_SETUP_PINS 0x02
#define CMD_SETUP_ANALOG_PINS 0x03
#define CMD_SETUP_INPUT_PULLUP_PINS 0x04
#define CMD_WRITE_DIGITAL_HIGH 0x05
#define CMD_WRITE_DIGITAL_LOW 0x06
#define CMD_RESTORE_OUTPUTS 0x07
#define CMD_READ_ANALOG 0x10

uint8_t buffer[2] = { 0x00, 0x00 };

void setup() {
  wdt_disable();
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest);
  Wire.onReceive(onReceive);
  wdt_enable(WDTO_2S);
}

void loop() {
  wdt_reset();
}

void readDigital() {
  buffer[0] = 0;
  buffer[1] = 0;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(i)) buffer[0] |= (1 << i);
  }
  for (int i = 0; i < 6; i++) {
    if (digitalRead(i + 8)) buffer[1] |= (1 << i);
  }
}

void readAnalog(int pin) {
  uint16_t val = analogRead(A0 + pin);
  buffer[0] = val & 0xFF;
  buffer[1] = val >> 8;
}

void onRequest() {
  Wire.write(buffer, 2);
}

void onReceive(int numBytes) {
  int command = Wire.read();
  switch (command) {
    case CMD_READ_DIGITAL:
      readDigital();
      break;
    case CMD_SETUP_PINS:
      setupPins(Wire.read(), Wire.read());
      break;
    case CMD_SETUP_INPUT_PULLUP_PINS:
      setupInputPullupPins(Wire.read(), Wire.read());
      break;
    case CMD_SETUP_ANALOG_PINS:
      setupAnalogPins(Wire.read(), Wire.read());
      break;
    case CMD_RESTORE_OUTPUTS:
      restoreOutputs(Wire.read(), Wire.read());
      break;
    case CMD_WRITE_DIGITAL_LOW:
      digitalWrite(Wire.read(), LOW);
      break;
    case CMD_WRITE_DIGITAL_HIGH:
      digitalWrite(Wire.read(), HIGH);
      break;
  }
  if (command >= CMD_READ_ANALOG && command <= CMD_READ_ANALOG + 7) {
    readAnalog(command - CMD_READ_ANALOG);
  }
}

void setupPins(uint8_t data1, uint8_t data0) {
  for (int i = 0; i < 8; i++)
    pinMode(i, (1 << i));
  for (int i = 0; i < 6; i++)
    pinMode(i + 8, (1 << i));
}

void setupInputPullupPins(uint8_t data1, uint8_t data0) {
  for (int i = 0; i < 8; i++) {
    if (data0 & (1 << i)) pinMode(i, INPUT_PULLUP);
  }
  for (int i = 0; i < 6; i++) {
    if (data1 & (1 << i)) pinMode(i + 8, INPUT_PULLUP);
  }
}

void setupAnalogPins(uint8_t data1, uint8_t data0) {
  for (int i = 0; i < 8; i++) {
    if (data0 & (1 << i)) {
      pinMode(A0 + i, INPUT);
    }
  }
}

void restoreOutputs(uint8_t data1, uint8_t data0) {
  for (int i = 0; i < 8; i++) {
    if (data0 & (1 << i)) digitalWrite(i, HIGH);
  }
  for (int i = 0; i < 6; i++) {
    if (data1 & (1 << i)) digitalWrite(i + 8, HIGH);
  }
}
