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
  void retransmit_code();
  void loop() override;
  void enable_passthrough(bool enable);
  bool read_relay_state();
  void enable_relay(bool enable);

protected:
  volatile uint32_t last_update_time_{0};
  uint8_t input_states_[2]{0, 0};
  uint8_t passthrough_state_{0};
  uint8_t relay_state_{0};
};

} // namespace gates
} // namespace esphome
