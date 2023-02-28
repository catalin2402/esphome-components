#pragma once

#include "../gates.h"
#include "esphome/components/button/button.h"
#include "esphome/core/component.h"

namespace esphome {
namespace gates {

class GatesButton : public button::Button, public Component {
public:
  void dump_config() override;
  void set_parent(Gates *parent) { this->parent_ = parent; };

protected:
  void press_action() override;
  Gates *parent_; 
};

} // namespace gates
} // namespace esphome