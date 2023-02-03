#pragma once

#include "../PT2323.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace pt2323 {

class PT2323Switch : public switch_::Switch, public Component {
public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override {
    return esphome::setup_priority::AFTER_WIFI;
  }
  void set_type(uint8_t type) { this->type_ = type; }
  void set_channel_a(uint8_t channel_a) { this->channel_a_ = channel_a; }
  void set_channel_b(uint8_t channel_b) { this->channel_b_ = channel_b; }
  void set_parent(PT2323 *parent) { this->parent_ = parent; }

protected:
  void write_state(bool state) override;

  PT2323 *parent_;
  uint8_t type_{0};
  uint8_t channel_a_{0};
  uint8_t channel_b_{0};
  bool state_;
};

} // namespace pt2323
} // namespace esphome