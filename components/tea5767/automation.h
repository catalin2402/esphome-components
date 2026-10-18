#pragma once

#include "tea5767.h"
#include "esphome/core/automation.h"

namespace esphome::tea5767 {

template<typename... Ts> class SetFrequencyAction : public Action<Ts...> {
 public:
  explicit SetFrequencyAction(TEA5767 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint64_t, frequency)
  void play(Ts... x) override { this->parent_->set_frequency(this->frequency_.value(x...)); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class SetMuteAction : public Action<Ts...> {
 public:
  explicit SetMuteAction(TEA5767 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, mute)
  void play(Ts... x) override { this->parent_->set_mute(this->mute_.value(x...)); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class SetMonoAction : public Action<Ts...> {
 public:
  explicit SetMonoAction(TEA5767 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, mono)
  void play(Ts... x) override { this->parent_->set_mono(this->mono_.value(x...)); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class SetSoftMuteAction : public Action<Ts...> {
 public:
  explicit SetSoftMuteAction(TEA5767 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, soft_mute)
  void play(Ts... x) override { this->parent_->set_soft_mute(this->soft_mute_.value(x...)); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class SetStereoNoiseCancelingAction : public Action<Ts...> {
 public:
  explicit SetStereoNoiseCancelingAction(TEA5767 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, stereo_noise_canceling)
  void play(Ts... x) override { this->parent_->set_stereo_noise_canceling(this->stereo_noise_canceling_.value(x...)); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class SetHighCutControlAction : public Action<Ts...> {
 public:
  explicit SetHighCutControlAction(TEA5767 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, high_cut_control)
  void play(Ts... x) override { this->parent_->set_high_cut_control(this->high_cut_control_.value(x...)); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class ToggleMuteAction : public Action<Ts...> {
 public:
  explicit ToggleMuteAction(TEA5767 *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->toggle_mute(); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class ResendDataAction : public Action<Ts...> {
 public:
  explicit ResendDataAction(TEA5767 *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->resend_data(); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class IsMutedCondition : public Condition<Ts...> {
 public:
  explicit IsMutedCondition(TEA5767 *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_muted(); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class IsMonoCondition : public Condition<Ts...> {
 public:
  explicit IsMonoCondition(TEA5767 *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_mono(); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class IsSoftMutedCondition : public Condition<Ts...> {
 public:
  explicit IsSoftMutedCondition(TEA5767 *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_soft_muted(); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class IsStereoNoiseCancelingCondition : public Condition<Ts...> {
 public:
  explicit IsStereoNoiseCancelingCondition(TEA5767 *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->get_stereo_noise_canceling(); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class IsHighCutControlCondition : public Condition<Ts...> {
 public:
  explicit IsHighCutControlCondition(TEA5767 *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->get_high_cut_control(); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class IsStereoCondition : public Condition<Ts...> {
 public:
  explicit IsStereoCondition(TEA5767 *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_stereo(); }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class FrequencyInRangeCondition : public Condition<Ts...> {
 public:
  explicit FrequencyInRangeCondition(TEA5767 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint64_t, min)
  TEMPLATABLE_VALUE(uint64_t, max)
  bool check(Ts... x) override {
    const auto value = this->parent_->get_requested_frequency();
    return value >= this->min_.value(x...) && value <= this->max_.value(x...);
  }

 protected:
  TEA5767 *parent_;
};

template<typename... Ts> class SignalLevelInRangeCondition : public Condition<Ts...> {
 public:
  explicit SignalLevelInRangeCondition(TEA5767 *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, min)
  TEMPLATABLE_VALUE(int, max)
  bool check(Ts... x) override {
    if (!this->parent_->has_status())
      return false;
    const auto value = this->parent_->get_level();
    return value >= this->min_.value(x...) && value <= this->max_.value(x...);
  }

 protected:
  TEA5767 *parent_;
};

class MuteTrigger : public Trigger<bool> {
 public:
  explicit MuteTrigger(TEA5767 *parent) {
    parent->add_on_setting_callback([this](Setting setting, bool enabled) {
      if (setting == Setting::MUTE)
        this->trigger(enabled);
    });
  }
};

class MonoTrigger : public Trigger<bool> {
 public:
  explicit MonoTrigger(TEA5767 *parent) {
    parent->add_on_setting_callback([this](Setting setting, bool enabled) {
      if (setting == Setting::MONO)
        this->trigger(enabled);
    });
  }
};

class SoftMuteTrigger : public Trigger<bool> {
 public:
  explicit SoftMuteTrigger(TEA5767 *parent) {
    parent->add_on_setting_callback([this](Setting setting, bool enabled) {
      if (setting == Setting::SOFT_MUTE)
        this->trigger(enabled);
    });
  }
};

class StereoNoiseCancelingTrigger : public Trigger<bool> {
 public:
  explicit StereoNoiseCancelingTrigger(TEA5767 *parent) {
    parent->add_on_setting_callback([this](Setting setting, bool enabled) {
      if (setting == Setting::STEREO_NOISE_CANCELING)
        this->trigger(enabled);
    });
  }
};

class HighCutControlTrigger : public Trigger<bool> {
 public:
  explicit HighCutControlTrigger(TEA5767 *parent) {
    parent->add_on_setting_callback([this](Setting setting, bool enabled) {
      if (setting == Setting::HIGH_CUT_CONTROL)
        this->trigger(enabled);
    });
  }
};

class FrequencyTrigger : public Trigger<uint64_t> {
 public:
  explicit FrequencyTrigger(TEA5767 *parent) {
    parent->add_on_frequency_callback([this](uint64_t frequency) { this->trigger(frequency); });
  }
};

class StereoTrigger : public Trigger<bool> {
 public:
  explicit StereoTrigger(TEA5767 *parent) {
    parent->add_on_stereo_callback([this](bool stereo) { this->trigger(stereo); });
  }
};

class LevelTrigger : public Trigger<uint8_t> {
 public:
  explicit LevelTrigger(TEA5767 *parent) {
    parent->add_on_level_callback([this](uint8_t level) { this->trigger(level); });
  }
};

}  // namespace esphome::tea5767
