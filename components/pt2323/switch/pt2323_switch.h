#pragma once

#include "esphome/components/pt2323/pt2323.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome::pt2323 {

enum SwitchType { ENHANCE, BOOST, MUTE, MUTE_ALL };

class PT2323Switch : public switch_::Switch, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void update();
  void set_type(SwitchType type) { this->type_ = type; }
  void set_channel_a(uint8_t channel_a) { this->channel_a_ = channel_a; }
  void set_channel_b(uint8_t channel_b) { this->channel_b_ = channel_b; }
  void set_parent(PT2323 *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  bool get_new_state_();

  PT2323 *parent_{nullptr};
  SwitchType type_{ENHANCE};
  uint8_t channel_a_{0};
  uint8_t channel_b_{0};
};

}  // namespace esphome::pt2323
