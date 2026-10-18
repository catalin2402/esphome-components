#pragma once

#include "pt2323.h"
#include "esphome/core/automation.h"

namespace esphome::pt2323 {

template<typename... Ts> class ResendDataAction : public Action<Ts...> {
 public:
  explicit ResendDataAction(PT2323 *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->resend_data(); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class SelectInputAction : public Action<Ts...> {
 public:
  explicit SelectInputAction(PT2323 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, input)
  void play(Ts... x) override { this->parent_->set_input(this->input_.value(x...)); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class SetMuteAction : public Action<Ts...> {
 public:
  explicit SetMuteAction(PT2323 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, mute)
  void play(Ts... x) override { this->parent_->mute_all_channels(this->mute_.value(x...)); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class ToggleMuteAction : public Action<Ts...> {
 public:
  explicit ToggleMuteAction(PT2323 *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->mute_all_channels(!this->parent_->get_mute()); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class SetMuteChannelAction : public Action<Ts...> {
 public:
  explicit SetMuteChannelAction(PT2323 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, channel)
  TEMPLATABLE_VALUE(bool, mute)
  void play(Ts... x) override { this->parent_->mute_channel(this->channel_.value(x...), this->mute_.value(x...)); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class SetEnhanceAction : public Action<Ts...> {
 public:
  explicit SetEnhanceAction(PT2323 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, enhance)
  void play(Ts... x) override { this->parent_->set_enhance(this->enhance_.value(x...)); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class SetBoostAction : public Action<Ts...> {
 public:
  explicit SetBoostAction(PT2323 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, boost)
  void play(Ts... x) override { this->parent_->set_boost(this->boost_.value(x...)); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class IsMutedCondition : public Condition<Ts...> {
 public:
  explicit IsMutedCondition(PT2323 *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->get_mute(); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class SelectedInputCondition : public Condition<Ts...> {
 public:
  explicit SelectedInputCondition(PT2323 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, input)
  bool check(Ts... x) override { return this->parent_->get_input() == this->input_.value(x...); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class IsEnhancedCondition : public Condition<Ts...> {
 public:
  explicit IsEnhancedCondition(PT2323 *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->get_enhance(); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class IsBoostedCondition : public Condition<Ts...> {
 public:
  explicit IsBoostedCondition(PT2323 *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->get_boost(); }

 protected:
  PT2323 *parent_;
};

template<typename... Ts> class IsChannelMutedCondition : public Condition<Ts...> {
 public:
  explicit IsChannelMutedCondition(PT2323 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, channel)
  bool check(Ts... x) override {
    const int channel = this->channel_.value(x...);
    return channel >= 1 && channel <= 6 && (this->parent_->get_mute() || this->parent_->get_channel_mute(channel));
  }

 protected:
  PT2323 *parent_;
};

class InputTrigger : public Trigger<int> {
 public:
  explicit InputTrigger(PT2323 *parent) {
    parent->add_on_input_callback([this](int input) { this->trigger(input); });
  }
};

class EnhanceTrigger : public Trigger<bool> {
 public:
  explicit EnhanceTrigger(PT2323 *parent) {
    parent->add_on_enhance_callback([this](bool enabled) { this->trigger(enabled); });
  }
};

class BoostTrigger : public Trigger<bool> {
 public:
  explicit BoostTrigger(PT2323 *parent) {
    parent->add_on_boost_callback([this](bool enabled) { this->trigger(enabled); });
  }
};

class MuteTrigger : public Trigger<bool> {
 public:
  explicit MuteTrigger(PT2323 *parent) {
    parent->add_on_mute_callback([this](bool muted) { this->trigger(muted); });
  }
};

class ChannelMuteTrigger : public Trigger<int, bool> {
 public:
  explicit ChannelMuteTrigger(PT2323 *parent) {
    parent->add_on_channel_mute_callback([this](int channel, bool muted) { this->trigger(channel, muted); });
  }
};

}  // namespace esphome::pt2323
