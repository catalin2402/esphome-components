#pragma once

#include "silvercrest_protocol.h"
#include "esphome/core/component.h"

namespace esphome::silvercrest {

class Silvercrest : public Component, public remote_base::RemoteTransmittable {
 public:
  void dump_config() override;
  void set_repeat_times(uint8_t repeat_times) { this->repeat_times_ = repeat_times; }
  void control_channel(Channel channel, bool state);

 protected:
  uint8_t repeat_times_{5};
};

}  // namespace esphome::silvercrest
