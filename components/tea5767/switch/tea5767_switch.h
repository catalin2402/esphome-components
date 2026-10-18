#pragma once

#include "esphome/components/tea5767/tea5767.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome::tea5767 {

enum SwitchType { MUTE, MONO, SOFT_MUTE, STEREO_NOISE_CANCELING, HIGH_CUT_CONTROL };

class TEA5767Switch : public switch_::Switch, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void update();
  void set_type(SwitchType type) { this->type_ = type; }
  void set_parent(TEA5767 *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  bool get_new_state_();

  TEA5767 *parent_{nullptr};
  SwitchType type_{MUTE};
};

}  // namespace esphome::tea5767