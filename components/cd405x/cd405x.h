
#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cd405x {

class CD405x : public Component {
public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void activate_channel(uint8_t chnnel);
  void inhibit(bool inhibit);

  void set_pin_a(GPIOPin *pin) { this->pin_a_ = pin; }
  void set_pin_b(GPIOPin *pin) { this->pin_b_ = pin; }
  void set_pin_c(GPIOPin *pin) { this->pin_c_ = pin; }
  void set_pin_inhibit(GPIOPin *pin) { this->pin_inhibit_ = pin; }

protected:
  GPIOPin *pin_a_;
  GPIOPin *pin_b_;
  GPIOPin *pin_c_;
  GPIOPin *pin_inhibit_;
};

} // namespace cd405x
} // namespace esphome
