#pragma once

#include "../gates.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace gates {

class GatesSwitch : public switch_::Switch, public Component {
public:
  float get_setup_priority() const override;
  void setup() override;
  void dump_config() override;
  void set_parent(Gates *parent) { this->parent_ = parent; };
  void set_type(int type) { this->type_ = type; };

protected:
  void write_state(bool state) override;
  Gates *parent_;
  bool state_;
  int type_;
};

} // namespace gates
} // namespace esphome