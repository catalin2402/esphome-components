#pragma once

#include "../PT2258.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace pt2258 {

class PT2258Switch : public switch_::Switch, public Component {
public:
  void setup() override;
  void dump_config() override;

  void set_parent(PT2258 *parent);

protected:
  void write_state(bool state) override;

  PT2258 *parent_;
  bool state_;
};

} // namespace pt2258
} // namespace esphome