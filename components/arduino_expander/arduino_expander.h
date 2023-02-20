#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"

namespace esphome {
namespace arduino_expander {

class ArduinoExpander : public PollingComponent, public i2c::I2CDevice {
public:
  ArduinoExpander() : PollingComponent(1) {}
  bool digital_read(uint8_t pin);
  void digital_write(uint8_t pin, bool value);
  uint16_t read_analog(uint8_t pin);
  void pin_mode(uint8_t pin, gpio::Flags flags);
  void loop() override;
  void setup() override;
  void update() override;

protected:
  bool configure_{true};
  long configure_timeout_{0};
  uint16_t pin_modes_{0};
  uint16_t pullup_pins_{0};
  uint16_t input_states_{0};
  uint16_t output_states_{0};
  uint16_t analog_reads_[6]{0, 0, 0, 0, 0, 0};
};

class ArduinoGPIOPin : public GPIOPin {
public:
  void setup() override;
  void pin_mode(gpio::Flags flags) override;
  bool digital_read() override;
  void digital_write(bool value) override;
  std::string dump_summary() const override;

  void set_parent(ArduinoExpander *parent) { parent_ = parent; }
  void set_pin(uint8_t pin) { pin_ = pin; }
  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { flags_ = flags; }

protected:
  ArduinoExpander *parent_;
  uint8_t pin_;
  bool inverted_;
  gpio::Flags flags_;
};

} // namespace arduino_expander
} // namespace esphome
