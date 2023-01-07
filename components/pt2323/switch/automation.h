#pragma once

#include "PT2323Switch.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"


namespace esphome {
namespace pt2323 {

class SwitchTurnOnTrigger : public Trigger<> {
public:
  SwitchTurnOnTrigger(PT2323Switch *a_switch) {
    a_switch->add_on_state_callback([this](bool state) {
      if (state) {
        this->trigger();
      }
    });
  }
};

class SwitchTurnOffTrigger : public Trigger<> {
public:
  SwitchTurnOffTrigger(PT2323Switch *a_switch) {
    a_switch->add_on_state_callback([this](bool state) {
      if (!state) {
        this->trigger();
      }
    });
  }
};

} // namespace pt2323
} // namespace esphome