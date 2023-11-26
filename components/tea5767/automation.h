#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "tea5767.h"

namespace esphome {
namespace tea5767 {

template <typename... Ts> class SetFrequencyAction : public Action<Ts...> {
public:
  explicit SetFrequencyAction(TEA5767 *a_tea5767) : tea5767_(a_tea5767) {}
  TEMPLATABLE_VALUE(uint64_t, frequency);
  void play(Ts... x) override {
    this->tea5767_->set_frequency(this->frequency_.value(x...));
  }

protected:
  TEA5767 *tea5767_;
};

template <typename... Ts> class SetMuteAction : public Action<Ts...> {
public:
  explicit SetMuteAction(TEA5767 *a_tea5767) : tea5767_(a_tea5767) {}
  TEMPLATABLE_VALUE(bool, mute);
  void play(Ts... x) override {
    this->tea5767_->set_mute(this->mute_.value(x...));
  }

protected:
  TEA5767 *tea5767_;
};

template <typename... Ts> class SetMonoAction : public Action<Ts...> {
public:
  explicit SetMonoAction(TEA5767 *a_tea5767) : tea5767_(a_tea5767) {}
  TEMPLATABLE_VALUE(bool, mono);
  void play(Ts... x) override {
    this->tea5767_->set_mono(this->mono_.value(x...));
  }

protected:
  TEA5767 *tea5767_;
};

template <typename... Ts> class SetSoftMuteAutomation : public Action<Ts...> {
public:
  explicit SetSoftMuteAutomation(TEA5767 *a_tea5767) : tea5767_(a_tea5767) {}
  TEMPLATABLE_VALUE(bool, soft_mute);
  void play(Ts... x) override {
    this->tea5767_->set_soft_mute(this->soft_mute_.value(x...));
  }

protected:
  TEA5767 *tea5767_;
};

template <typename... Ts>
class SetStereoNoiseCancelingAutomation : public Action<Ts...> {
public:
  explicit SetStereoNoiseCancelingAutomation(TEA5767 *a_tea5767)
      : tea5767_(a_tea5767) {}
  TEMPLATABLE_VALUE(bool, stereo_noise_canceling);
  void play(Ts... x) override {
    this->tea5767_->set_stereo_noise_canceling(
        this->stereo_noise_canceling_.value(x...));
  }

protected:
  TEA5767 *tea5767_;
};

template <typename... Ts> class SetHighCutControlAction : public Action<Ts...> {
public:
  explicit SetHighCutControlAction(TEA5767 *a_tea5767) : tea5767_(a_tea5767) {}
  TEMPLATABLE_VALUE(bool, high_cut_control);
  void play(Ts... x) override {
    this->tea5767_->set_high_cut_control(this->high_cut_control_.value(x...));
  }

protected:
  TEA5767 *tea5767_;
};

} // namespace tea5767
} // namespace esphome