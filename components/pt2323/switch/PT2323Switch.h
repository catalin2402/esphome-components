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
  void set_switch_type(uint8_t switch_type);

  void set_parent(PT2323 *parent);

protected:
  void write_state(bool state) override;

  PT2323 *parent_;
  uint8_t switch_type_{0};
  bool state_;
};

} // namespace pt2323
} // namespace esphome