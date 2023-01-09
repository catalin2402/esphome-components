#pragma once

#include "PT2258Switch.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome {
namespace pt2258 {

class SwitchTurnOnTrigger : public Trigger<> {
public:
  SwitchTurnOnTrigger(PT2258Switch *a_switch) {
    a_switch->add_on_state_callback([this](bool state) {
      if (state) {
        this->trigger();
      }
    });
  }
};

class SwitchTurnOffTrigger : public Trigger<> {
public:
  SwitchTurnOffTrigger(PT2258Switch *a_switch) {
    a_switch->add_on_state_callback([this](bool state) {
      if (!state) {
        this->trigger();
      }
    });
  }
};

} // namespace pt2258
} // namespace esphome