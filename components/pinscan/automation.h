#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "pinscan.h"

namespace esphome {
namespace pinscan {

template <typename... Ts> class SetInputAction : public Action<Ts...> {
public:
  explicit SetInputAction(Pinscan *a_pinscan) : pinscan_(a_pinscan) {}
  TEMPLATABLE_VALUE(int, pin);
  TEMPLATABLE_VALUE(int, mode);
  void play(Ts... x) override {
    this->pinscan_->set_pin(this->pin_.value(x...));
    this->pinscan_->set_mode(this->mode_.value(x...));
  }

protected:
  Pinscan *pinscan_;
};

} // namespace pinscan
} // namespace esphome