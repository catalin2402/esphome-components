#pragma once

#include "esphome/components/pt2258/pt2258.h"
#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome::pt2258 {

enum NumberType { MASTER, VOLUME, OFFSET };

class PT2258Number : public number::Number, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void update();
  void set_type(NumberType type) { this->type_ = type; }
  void set_channel_a(uint8_t channel_a) { this->channel_a_ = channel_a; }
  void set_channel_b(uint8_t channel_b) { this->channel_b_ = channel_b; }
  void set_parent(PT2258 *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  float get_new_state_();

  PT2258 *parent_{nullptr};
  NumberType type_{MASTER};
  uint8_t channel_a_{0};
  uint8_t channel_b_{0};
};

}  // namespace esphome::pt2258
