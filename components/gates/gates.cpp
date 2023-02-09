#include "gates.h"

#define CMD_READ_DIGITAL 0x80
#define CMD_ENABLE_PASSTHROUGH 0x70
#define CMD_DISABLE_PASSTHROUGH 0x71
#define CMD_READ_PASSTHROUGH_STATE 0x72
#define CMD_SEND_CODE 0x73
#define CMD_RELAY_STATUS 0x74
#define CMD_RELAY_TURN_ON 0x75
#define CMD_RELAY_TURN_OFF 0x76
static const char *TAGape = "gates";

namespace esphome {
namespace gates {

float Gates::get_setup_priority() const { return setup_priority::AFTER_WIFI; }

void Gates::loop() {
  if (millis() - this->last_update_time_ < 100)
    return;
  uint8_t data[2]{0, 0};
  uint8_t result = this->read_register(CMD_READ_DIGITAL, data, 2);
  if (result == i2c::ERROR_OK) {
    this->input_states_[0] = data[0];
    this->input_states_[1] = data[1];
  } else {
    ESP_LOGD(TAGape, "Error code:%d", result);
  }
  this->last_update_time_ = millis();
}

bool Gates::read_pin(uint8_t pin) {
  bool result;
  if (pin < 8) {
    result = this->input_states_[0] & (1 << pin);
  } else {
    result = this->input_states_[1] & (1 << (pin - 8));
  }
  return result;
}

bool Gates::read_passthrough_state() {
  uint8_t data[1]{0};
  if (this->read_register(CMD_READ_PASSTHROUGH_STATE, data, 1) ==
      i2c::ERROR_OK) {
    this->passthrough_state_ = data[0];
  }
  return this->passthrough_state_;
}

void Gates::enable_passthrough(bool enabled) {
  this->write_register(
      enabled ? CMD_ENABLE_PASSTHROUGH : CMD_DISABLE_PASSTHROUGH, nullptr, 0);
}

bool Gates::read_relay_state() {
  uint8_t data[1]{0};
  if (this->read_register(CMD_RELAY_STATUS, data, 1) == i2c::ERROR_OK) {
    this->relay_state_ = data[0];
  }
  return this->relay_state_;
}

void Gates::enable_relay(bool enabled) {
  this->write_register(enabled ? CMD_RELAY_TURN_ON : CMD_RELAY_TURN_OFF,
                       nullptr, 0);
}

void Gates::send_code() { this->write_register(CMD_SEND_CODE, nullptr, 0); }

} // namespace gates
} // namespace esphome