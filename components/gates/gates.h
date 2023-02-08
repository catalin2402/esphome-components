#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"

namespace esphome {
namespace gates {

class Gates : public Component, public i2c::I2CDevice {
public:
  float get_setup_priority() const override;
  bool read_pin(uint8_t pin);
  bool read_passthrough_state();
  void send_code();
  void enable_passthrough(bool enable);
};

} // namespace gates
} // namespace esphome
