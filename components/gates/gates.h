#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"

namespace esphome {
namespace gates {

class Gates : public PollingComponent, public i2c::I2CDevice {
public:
  Gates() : PollingComponent(100) {}
  void setup() override;
  void loop() override;
  void update() override;
  void pin_mode(uint8_t pin, gpio::Flags flags);
  bool digital_read(uint8_t pin);
  void digital_write(uint8_t pin, bool value);
  void retransmit_code();

protected:
  bool configure_{true};
  long configure_timeout_{0};
  uint16_t pin_modes_{0};
  uint16_t pullup_pins_{0};
  uint16_t input_states_{0};
  uint16_t output_states_{0};
  long last_update_time_{0};
  bool passthrough_state_;
};

class GatesGPIOPin : public GPIOPin {
public:
  void setup() override;
  void pin_mode(gpio::Flags flags) override;
  bool digital_read() override;
  void digital_write(bool value) override;
  std::string dump_summary() const override;

  void set_parent(Gates *parent) { parent_ = parent; }
  void set_pin(uint8_t pin) { pin_ = pin; }
  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { flags_ = flags; }

protected:
  Gates *parent_;
  uint8_t pin_;
  bool inverted_;
  gpio::Flags flags_;
};

} // namespace gates
} // namespace esphome
