#pragma once

#include "../silvercrest_remote.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace silvercrest {

class SilvercrestSwitch : public switch_::Switch, public Component {
public:
  void dump_config() override;
  void set_channel(Channel channel) { this->channel_ = channel; };
  void set_parent(Silvercrest *parent) { this->parent_ = parent; };

protected:
  void write_state(bool state) override;
  bool assumed_state() override;

  Silvercrest *parent_;
  Channel channel_{MASTER};
};

} // namespace silvercrest
} // namespace esphome