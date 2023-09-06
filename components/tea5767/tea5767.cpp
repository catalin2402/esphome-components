#include "tea5767.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tea5767 {

static const char *const TAG = "tea5767";

void TEA5767::dump_config() {
  ESP_LOGCONFIG(TAG, "TEA5767:");
  LOG_I2C_DEVICE(this);
}

void TEA5767::setup() {
  registers[0] = 0x00;
  registers[1] = 0x00;
  registers[2] = 0xB0;
  registers[REG_4] = REG_4_XTAL | REG_4_SMUTE;
  
  if (inEurope_) {
    registers[REG_4] &= ~REG_4_BL;
    registers[REG_5] = 0;
  } else {
    registers[REG_4] |= REG_4_BL;
    registers[REG_5] = REG_5_DTC;
  }
}

void TEA5767::setMono(bool switchOn) {
  if (switchOn) {
    registers[REG_3] |= REG_3_MS;
  } else {
    registers[REG_3] &= ~REG_3_MS;
  }
  saveRegisters();
}

void TEA5767::setMute(bool switchOn) {
  if (switchOn) {
    registers[REG_1] |= REG_1_MUTE;
  } else {
    registers[REG_1] &= ~REG_1_MUTE;
  }
  saveRegisters();
}

uint16_t TEA5767::getFrequency() {
  readRegisters();

  unsigned long frequencyW = ((status[REG_1] & REG_1_PLL) << 8) | status[REG_2];
  frequencyW = ((frequencyW * QUARTZ / 4) - FILTER) / 10000;

  return frequencyW;
}

void TEA5767::setFrequency(uint16_t newF) {
  unsigned int frequencyB = 4 * (newF * 10000L + FILTER) / QUARTZ;
  registers[0] = frequencyB >> 8;
  registers[1] = frequencyB & 0XFF;
  saveRegisters();
}

void TEA5767::readRegisters() {
  if (!this->status_has_error()) {
    uint8_t result = this->read(status, 5);
    if (result != i2c::ERROR_OK) {
      this->status_set_error();
    }
  }
}

void TEA5767::saveRegisters() {
  uint8_t result = this->write(registers, 5);
  if (result != i2c::ERROR_OK) {
    this->status_set_error();
  }
}

} // namespace tea5767
} // namespace esphome