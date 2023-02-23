#include <Arduino.h>
#include <Wire.h>
#include <avr/wdt.h>


#define I2C_ADDRESS 0x10

#define CMD_READ 0x01
#define CMD_SETUP_PINS 0x02
#define CMD_SETUP_INPUT_PULLUP_PINS 0x03
#define CMD_WRITE_DIGITAL_HIGH 0x04
#define CMD_WRITE_DIGITAL_LOW 0x05
#define CMD_RESTORE_OUTPUTS 0x06
#define CMD_ADC_SOURCE 0x07

// #define DEBUG

uint8_t buffer[14] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  wdt_disable();
  for (uint8_t i = 0; i < 6; i++) {
    uint8_t pin = (i >= 4) ? i + 2 : i;
    pinMode(A0 + i, INPUT);
  }
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest);
  Wire.onReceive(onReceive);
  wdt_enable(WDTO_2S);
}

void loop() {
  wdt_reset();
}

void readData() {
  for (uint8_t i = 0; i < 14; i++)
    buffer[i] = 0x00;
  for (uint8_t i = 0; i < 8; i++)
    if (digitalRead(i)) buffer[0] |= (1 << i);
  for (uint8_t i = 0; i < 6; i++)
    if (digitalRead(i + 8)) buffer[1] |= (1 << i);
  for (uint8_t i = 0; i < 6; i++) {
    uint8_t pin = (i >= 4) ? i + 2 : i;
    uint16_t val = analogRead(A0 + pin);
    buffer[i * 2 + 2] = val & 0xFF;
    buffer[i * 2 + 3] = val >> 8;
  }
}

void onRequest() {
  Wire.write(buffer, 14);
}

void onReceive(int numBytes) {
  switch (Wire.read()) {
    case CMD_READ:
      readData();
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
      digitalWrite(Wire.read(), LOW);
      break;
    case CMD_WRITE_DIGITAL_HIGH:
      digitalWrite(Wire.read(), HIGH);
      break;
    case CMD_ADC_SOURCE:
      setAdcSource(Wire.read(), Wire.read());
      break;
  }
}

void setupPins(uint8_t data1, uint8_t data0) {
  for (uint8_t i = 0; i < 8; i++) {
    bool mode = data0 & (1 << i);
#ifdef DEBUG
    Serial.print("Setting pin: ");
    Serial.print(i);
    Serial.print(" as: ");
    Serial.println((mode) ? "OUTPUT" : "INPUT");
#endif
    pinMode(i, mode);
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
    if (data0 & (1 << i)) {
#ifdef DEBUG
      Serial.print("Setting pin: ");
      Serial.print(i);
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
      pinMode(i + 8, INPUT_PULLUP);
#endif
    }
  }
}

void restoreOutputs(uint8_t data1, uint8_t data0) {
  for (uint8_t i = 0; i < 8; i++) {
    if (data0 & (1 << i)) {
#ifdef DEBUG
      Serial.print("Restoring pin: ");
      Serial.print(i);
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

void setAdcSource(uint8_t data1, uint8_t data0) {
  switch (data1) {
    case 0:
#ifdef DEBUG
      Serial.println("Setting analog reference: DEFAULT");
#endif
      analogReference(DEFAULT);
      break;
    case 1:
#ifdef DEBUG
      Serial.println("Setting analog reference: INTERNAL");
#endif
      analogReference(INTERNAL);
      break;
    case 2:
#ifdef DEBUG
      Serial.println("Setting analog reference: EXTERNAL");
#endif
      analogReference(EXTERNAL);
      break;
  }
}
