#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "pt2323.h"

namespace esphome {
namespace pt2323 {

template <typename... Ts> class SetInputAction : public Action<Ts...> {
public:
  explicit SetInputAction(PT2323 *a_pt2323) : pt2323_(a_pt2323) {}
  TEMPLATABLE_VALUE(int, input);
  void play(Ts... x) override {
    this->pt2323_->set_input(this->input_.value(x...));
  }

protected:
  PT2323 *pt2323_;
};

template <typename... Ts> class SetMuteAction : public Action<Ts...> {
public:
  explicit SetMuteAction(PT2323 *a_pt2323) : pt2323_(a_pt2323) {}
  void play(Ts... x) override { this->pt2323_->mute_all_channels(true); }

protected:
  PT2323 *pt2323_;
};

template <typename... Ts> class SetUnmuteAction : public Action<Ts...> {
public:
  explicit SetUnmuteAction(PT2323 *a_pt2323) : pt2323_(a_pt2323) {}
  void play(Ts... x) override { this->pt2323_->mute_all_channels(false); }

protected:
  PT2323 *pt2323_;
};

template <typename... Ts> class SetMuteChannelAction : public Action<Ts...> {
public:
  explicit SetMuteChannelAction(PT2323 *a_pt2323) : pt2323_(a_pt2323) {}
  TEMPLATABLE_VALUE(int, channel);
  void play(Ts... x) override {
    this->pt2323_->mute_channel(this->channel_.value(x...), true);
  }

protected:
  PT2323 *pt2323_;
};

template <typename... Ts> class SetUnmuteChannelAction : public Action<Ts...> {
public:
  explicit SetUnmuteChannelAction(PT2323 *a_pt2323) : pt2323_(a_pt2323) {}
  TEMPLATABLE_VALUE(int, channel);
  void play(Ts... x) override {
    this->pt2323_->mute_channel(this->channel_.value(x...), false);
  }

protected:
  PT2323 *pt2323_;
};

template <typename... Ts> class SetEnhanceAction : public Action<Ts...> {
public:
  explicit SetEnhanceAction(PT2323 *a_pt2323) : pt2323_(a_pt2323) {}
  TEMPLATABLE_VALUE(bool, enhance);
  void play(Ts... x) override {
    this->pt2323_->set_enhance(this->enhance_.value(x...));
  }

protected:
  PT2323 *pt2323_;
};

template <typename... Ts> class SetBoostAction : public Action<Ts...> {
public:
  explicit SetBoostAction(PT2323 *a_pt2323) : pt2323_(a_pt2323) {}
  TEMPLATABLE_VALUE(bool, boost);
  void play(Ts... x) override {
    this->pt2323_->set_boost(this->boost_.value(x...));
  }

protected:
  PT2323 *pt2323_;
};

} // namespace pt2323
} // namespace esphome