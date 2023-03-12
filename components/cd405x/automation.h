#pragma once

#include "cd405x.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome {
namespace cd405x {

template <typename... Ts> class ActivateChannelAction : public Action<Ts...> {
public:
  explicit ActivateChannelAction(CD405x *a_cd405x) : cd405x_(a_cd405x) {}
  TEMPLATABLE_VALUE(int, channel);
  void play(Ts... x) override {
    this->cd405x_->activate_channel(this->channel_.value(x...));
  }

protected:
  CD405x *cd405x_;
};

template <typename... Ts> class InhibitAction : public Action<Ts...> {
public:
  explicit InhibitAction(CD405x *a_cd405x) : cd405x_(a_cd405x) {}
  TEMPLATABLE_VALUE(bool, inhibit);
  void play(Ts... x) override {
    this->cd405x_->inhibit(this->inhibit_.value(x...));
  }

protected:
  CD405x *cd405x_;
};

} // namespace cd405x
} // namespace esphome