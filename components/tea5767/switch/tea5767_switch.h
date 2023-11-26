#pragma once

#include "../tea5767.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace tea5767 {

enum SwitchType {
  MUTE,
  MONO,
  SOFT_MUTE,
  STEREO_NOISE_CANCELING,
  HIGH_CUT_CONTROL
};

class TEA5767Switch : public switch_::Switch, public PollingComponent {
public:
  TEA5767Switch() : PollingComponent(1000) {}
  void setup() override;
  void dump_config() override;
  void update() override;
  void set_type(SwitchType type) { this->type_ = type; }
  void set_parent(TEA5767 *parent) { this->parent_ = parent; }

protected:
  void write_state(bool state) override;
  bool get_new_state_();

  TEA5767 *parent_;
  SwitchType type_{MUTE};
};

} // namespace tea5767
} // namespace esphome