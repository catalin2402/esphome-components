#pragma once

#include "../pt2258.h"
#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace pt2258 {

class PT2258Number : public number::Number, public PollingComponent {
public:
  PT2258Number() : PollingComponent(1000) {}
  void setup() override;
  void dump_config() override;
  void update() override;
  void set_type(uint8_t type) { this->type_ = type; }
  void set_channel_a(uint8_t channel_a) { this->channel_a_ = channel_a; }
  void set_channel_b(uint8_t channel_b) { this->channel_b_ = channel_b; }
  void set_parent(PT2258 *parent) { this->parent_ = parent; }

protected:
  void control(float value) override;
  float get_new_state();

  PT2258 *parent_;
  uint8_t type_{0};
  uint8_t channel_a_{0};
  uint8_t channel_b_{0};
};

} // namespace pt2258
} // namespace esphome