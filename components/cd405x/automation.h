#pragma once

#include "cd405x.h"
#include "esphome/core/automation.h"

namespace esphome::cd405x {

template<typename... Ts> class SelectChannelAction : public Action<Ts...> {
 public:
  explicit SelectChannelAction(CD405x *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, channel)
  void play(Ts... x) override { this->parent_->select_channel(this->channel_.value(x...)); }

 protected:
  CD405x *parent_;
};

template<typename... Ts> class InhibitAction : public Action<Ts...> {
 public:
  explicit InhibitAction(CD405x *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, inhibit)
  void play(Ts... x) override { this->parent_->inhibit(this->inhibit_.value(x...)); }

 protected:
  CD405x *parent_;
};

template<typename... Ts> class ToggleInhibitAction : public Action<Ts...> {
 public:
  explicit ToggleInhibitAction(CD405x *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->toggle_inhibit(); }

 protected:
  CD405x *parent_;
};

template<typename... Ts> class SelectedChannelCondition : public Condition<Ts...> {
 public:
  explicit SelectedChannelCondition(CD405x *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, channel)
  bool check(Ts... x) override { return this->parent_->get_channel() == this->channel_.value(x...); }

 protected:
  CD405x *parent_;
};

template<typename... Ts> class IsInhibitedCondition : public Condition<Ts...> {
 public:
  explicit IsInhibitedCondition(CD405x *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_inhibited(); }

 protected:
  CD405x *parent_;
};

template<typename... Ts> class IsEnabledCondition : public Condition<Ts...> {
 public:
  explicit IsEnabledCondition(CD405x *parent) : parent_(parent) {}
  bool check(Ts... x) override { return !this->parent_->is_inhibited(); }

 protected:
  CD405x *parent_;
};

class ChannelTrigger : public Trigger<int> {
 public:
  explicit ChannelTrigger(CD405x *parent) {
    parent->add_on_channel_callback([this](int channel) { this->trigger(channel); });
  }
};

class InhibitTrigger : public Trigger<bool> {
 public:
  explicit InhibitTrigger(CD405x *parent) {
    parent->add_on_inhibit_callback([this](bool inhibited) { this->trigger(inhibited); });
  }
};

}  // namespace esphome::cd405x
