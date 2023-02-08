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

bool Gates::read_pin(uint8_t pin) {
  uint8_t data[1]{0};
  bool success;
  this->read_register(CMD_READ_DIGITAL + pin, data, 1);
  return data[0] == 1;
}

bool Gates::read_passthrough_state() {
  uint8_t data[1]{0};
  bool success;
  this->read_register(CMD_READ_PASSTHROUGH_STATE, data, 1);
  return data[0] == 1;
}

void Gates::enable_passthrough(bool enabled) {
  this->write_register(
      enabled ? CMD_ENABLE_PASSTHROUGH : CMD_DISABLE_PASSTHROUGH, nullptr, 0);
}

bool Gates::read_relay_state() {
  uint8_t data[1]{0};
  bool success;
  this->read_register(CMD_RELAY_STATUS, data, 1);
  return data[0] == 1;
}

void Gates::enable_relay(bool enabled) {
  this->write_register(
      enabled ? CMD_RELAY_TURN_ON : CMD_RELAY_TURN_OFF, nullptr, 0);
}

void Gates::send_code() { this->write_register(CMD_SEND_CODE, nullptr, 0); }

} // namespace gates
} // namespace esphome