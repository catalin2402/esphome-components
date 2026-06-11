#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "pt2258.h"

namespace esphome {
namespace pt2258 {

template <typename... Ts> class ResendDataAction : public Action<Ts...> {
public:
  explicit ResendDataAction(PT2258 *a_pt2258) : pt2258_(a_pt2258) {}
  void play(Ts... x) override { this->pt2258_->resend_data(); }

protected:
  PT2258 *pt2258_;
};

template <typename... Ts> class SetMuteAction : public Action<Ts...> {
public:
  explicit SetMuteAction(PT2258 *a_pt2258) : pt2258_(a_pt2258) {}
  void play(Ts... x) override { this->pt2258_->set_mute(true); }

protected:
  PT2258 *pt2258_;
};

template <typename... Ts> class SetUnmuteAction : public Action<Ts...> {
public:
  explicit SetUnmuteAction(PT2258 *a_pt2258) : pt2258_(a_pt2258) {}
  void play(Ts... x) override { this->pt2258_->set_mute(false); }

protected:
  PT2258 *pt2258_;
};

} // namespace pt2258
} // namespace esphome
