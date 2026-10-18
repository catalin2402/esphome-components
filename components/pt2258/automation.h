#pragma once

#include "pt2258.h"
#include "esphome/core/automation.h"

namespace esphome::pt2258 {

template<typename... Ts> class ResendDataAction : public Action<Ts...> {
 public:
  explicit ResendDataAction(PT2258 *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->resend_data(); }

 protected:
  PT2258 *parent_;
};

template<typename... Ts> class SetMuteAction : public Action<Ts...> {
 public:
  explicit SetMuteAction(PT2258 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, mute)
  void play(Ts... x) override { this->parent_->set_mute(this->mute_.value(x...)); }

 protected:
  PT2258 *parent_;
};

template<typename... Ts> class ToggleMuteAction : public Action<Ts...> {
 public:
  explicit ToggleMuteAction(PT2258 *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->toggle_mute(); }

 protected:
  PT2258 *parent_;
};

template<typename... Ts> class SetMasterVolumeAction : public Action<Ts...> {
 public:
  explicit SetMasterVolumeAction(PT2258 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, volume)
  void play(Ts... x) override { this->parent_->set_master_volume(this->volume_.value(x...)); }

 protected:
  PT2258 *parent_;
};

template<typename... Ts> class SetChannelVolumeAction : public Action<Ts...> {
 public:
  explicit SetChannelVolumeAction(PT2258 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, channel)
  TEMPLATABLE_VALUE(int, volume)
  void play(Ts... x) override {
    this->parent_->set_channel_volume(this->volume_.value(x...), this->channel_.value(x...));
  }

 protected:
  PT2258 *parent_;
};

template<typename... Ts> class SetChannelOffsetAction : public Action<Ts...> {
 public:
  explicit SetChannelOffsetAction(PT2258 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, channel)
  TEMPLATABLE_VALUE(int, offset)
  void play(Ts... x) override {
    this->parent_->set_channel_offset(this->offset_.value(x...), this->channel_.value(x...));
  }

 protected:
  PT2258 *parent_;
};

template<typename... Ts> class IsMutedCondition : public Condition<Ts...> {
 public:
  explicit IsMutedCondition(PT2258 *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_muted(); }

 protected:
  PT2258 *parent_;
};

template<typename... Ts> class VolumeInRangeCondition : public Condition<Ts...> {
 public:
  explicit VolumeInRangeCondition(PT2258 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, channel)
  TEMPLATABLE_VALUE(int, min)
  TEMPLATABLE_VALUE(int, max)
  bool check(Ts... x) override {
    const int channel = this->channel_.value(x...);
    if (channel < 0 || channel > 6)
      return false;
    const int value = this->parent_->get_channel_volume(channel);
    return value >= this->min_.value(x...) && value <= this->max_.value(x...);
  }

 protected:
  PT2258 *parent_;
};

class VolumeTrigger : public Trigger<int, int> {
 public:
  explicit VolumeTrigger(PT2258 *parent) {
    parent->add_on_volume_callback([this](int channel, int volume) { this->trigger(channel, volume); });
  }
};

class MuteTrigger : public Trigger<bool> {
 public:
  explicit MuteTrigger(PT2258 *parent) {
    parent->add_on_mute_callback([this](bool muted) { this->trigger(muted); });
  }
};

}  // namespace esphome::pt2258
