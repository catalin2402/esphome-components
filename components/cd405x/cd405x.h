#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

namespace esphome::cd405x {

class CD405x : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void select_channel(int channel);
  void inhibit(bool inhibit);
  void toggle_inhibit() { this->inhibit(!this->inhibited_); }
  uint8_t get_channel() const { return this->channel_; }
  uint8_t get_channel_count() const { return this->pin_c_ == nullptr ? 4 : 8; }
  bool is_inhibited() const { return this->inhibited_; }

  void set_pin_a(GPIOPin *pin) { this->pin_a_ = pin; }
  void set_pin_b(GPIOPin *pin) { this->pin_b_ = pin; }
  void set_pin_c(GPIOPin *pin) { this->pin_c_ = pin; }
  void set_pin_inhibit(GPIOPin *pin) { this->pin_inhibit_ = pin; }
  void add_on_channel_callback(std::function<void(int)> &&callback) {
    this->channel_callback_.add(std::move(callback));
  }
  void add_on_inhibit_callback(std::function<void(bool)> &&callback) {
    this->inhibit_callback_.add(std::move(callback));
  }

 protected:
  void write_state_();

  GPIOPin *pin_a_{nullptr};
  GPIOPin *pin_b_{nullptr};
  GPIOPin *pin_c_{nullptr};
  GPIOPin *pin_inhibit_{nullptr};
  CallbackManager<void(int)> channel_callback_;
  CallbackManager<void(bool)> inhibit_callback_;
  uint8_t channel_{0};
  bool inhibited_{false};
  bool initialized_{false};
};

}  // namespace esphome::cd405x
