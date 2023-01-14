#pragma once

#include "../silvercrest_remote.h"
#include "esphome/components/button/button.h"
#include "esphome/core/component.h"

namespace esphome {
namespace silvercrest {

class SilvercrestButton :  public button::Button, public Component {
public:
  void dump_config() override;
  void set_button(uint8_t button_type);
  void set_parent(Silvercrest *parent);

protected:
  void press_action() override;

  Silvercrest *parent_;
  uint8_t button_type_{0};
};

} // namespace silvercrest
} // namespace esphome