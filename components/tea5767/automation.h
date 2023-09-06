#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "tea5767.h"

namespace esphome {
namespace tea5767 {

template <typename... Ts> class SetFrequencyAction : public Action<Ts...> {
public:
  explicit SetFrequencyAction(TEA5767 *a_tea5767) : tea5767_(a_tea5767) {}
  TEMPLATABLE_VALUE(uint16_t, frequency);
  void play(Ts... x) override {
    this->tea5767_->setFrequency(this->frequency_.value(x...));
  }

protected:
  TEA5767 *tea5767_;
};

} // namespace tea5767
} // namespace esphome