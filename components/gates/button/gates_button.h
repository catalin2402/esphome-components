#pragma once

#include "../gates.h"
#include "esphome/components/button/button.h"
#include "esphome/core/component.h"

namespace esphome {
namespace gates {

class GatesButton : public button::Button, public Component {
public:
  float get_setup_priority() const override;
  void dump_config() override;
  void set_parent(Gates *parent) { this->parent_ = parent; };
  void set_type(int type) { this->type_ = type; };

protected:
  void press_action() override;
  Gates *parent_;
  int type_{0};
};

} // namespace gates
} // namespace esphome