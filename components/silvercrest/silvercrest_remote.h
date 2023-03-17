#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace silvercrest {

enum Channel { A, B, C, D, MASTER };

class Silvercrest : public Component {
public:
  void setup() override;
  void set_transmitter_pin(GPIOPin *transmitter_pin) {
    transmitter_pin_ = transmitter_pin;
  }
  void control_channel(Channel channel, bool state, uint8_t repeat_times = 5);

private:
  GPIOPin *transmitter_pin_;
  void send_code_(const bool code[24], uint8_t repeat_times);
};

} // namespace silvercrest
} // namespace esphome