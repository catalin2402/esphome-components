#pragma once

#include "../silvercrest_remote.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace silvercrest {

class SilvercrestSwitch : public switch_::Switch, public Component {
public:
  void dump_config() override;
  void set_channel(uint8_t channel);
  void set_parent(Silvercrest *parent);

protected:
  void write_state(bool state) override;
  bool assumed_state() override;

  Silvercrest *parent_;
  uint8_t channel_{0};
};

} // namespace silvercrest
} // namespace esphome