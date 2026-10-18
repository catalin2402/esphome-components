#pragma once

#include <array>

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome::tea5767 {

enum class Setting : uint8_t { MUTE, MONO, SOFT_MUTE, HIGH_CUT_CONTROL, STEREO_NOISE_CANCELING };

class TEA5767 : public PollingComponent, public i2c::I2CDevice {
 public:
  void dump_config() override;
  void setup() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_in_japan(bool in_japan) { this->in_japan_ = in_japan; }
  void set_frequency(uint64_t frequency);
  void set_mono(bool mono) { this->set_setting_(Setting::MONO, mono); }
  void set_mute(bool mute) { this->set_setting_(Setting::MUTE, mute); }
  void set_soft_mute(bool soft_mute) { this->set_setting_(Setting::SOFT_MUTE, soft_mute); }
  void set_high_cut_control(bool enabled) { this->set_setting_(Setting::HIGH_CUT_CONTROL, enabled); }
  void set_stereo_noise_canceling(bool enabled) { this->set_setting_(Setting::STEREO_NOISE_CANCELING, enabled); }
  void toggle_mute() { this->set_mute(!this->is_muted()); }
  void resend_data();

  uint64_t get_requested_frequency() const { return this->frequency_; }
  uint64_t get_frequency() const;
  uint8_t get_level() const { return this->status_[3] >> 4; }
  bool has_status() const { return this->status_valid_; }
  bool is_stereo() const { return this->status_valid_ && (this->status_[2] & 0x80) != 0; }
  bool get_setting(Setting setting) const { return this->settings_[static_cast<uint8_t>(setting)]; }
  bool is_muted() const { return this->get_setting(Setting::MUTE); }
  bool is_mono() const { return this->get_setting(Setting::MONO); }
  bool is_soft_muted() const { return this->get_setting(Setting::SOFT_MUTE); }
  bool get_high_cut_control() const { return this->get_setting(Setting::HIGH_CUT_CONTROL); }
  bool get_stereo_noise_canceling() const { return this->get_setting(Setting::STEREO_NOISE_CANCELING); }

  void add_on_status_callback(std::function<void(uint64_t, uint8_t, bool)> &&callback) {
    this->status_callback_.add(std::move(callback));
  }
  void add_on_state_callback(std::function<void()> &&callback) { this->state_callback_.add(std::move(callback)); }
  void add_on_frequency_callback(std::function<void(uint64_t)> &&callback) {
    this->frequency_callback_.add(std::move(callback));
  }
  void add_on_setting_callback(std::function<void(Setting, bool)> &&callback) {
    this->setting_callback_.add(std::move(callback));
  }
  void add_on_stereo_callback(std::function<void(bool)> &&callback) { this->stereo_callback_.add(std::move(callback)); }
  void add_on_level_callback(std::function<void(uint8_t)> &&callback) {
    this->level_callback_.add(std::move(callback));
  }

 protected:
  void set_setting_(Setting setting, bool enabled);
  bool save_registers_();
  void update_status_();
  void schedule_read_();

  std::array<uint8_t, 5> status_{};
  std::array<bool, 5> settings_{};
  LazyCallbackManager<void(uint64_t, uint8_t, bool)> status_callback_;
  LazyCallbackManager<void()> state_callback_;
  LazyCallbackManager<void(uint64_t)> frequency_callback_;
  LazyCallbackManager<void(Setting, bool)> setting_callback_;
  LazyCallbackManager<void(bool)> stereo_callback_;
  LazyCallbackManager<void(uint8_t)> level_callback_;
  uint64_t frequency_{87500000};
  bool in_japan_{false};
  bool initialized_{false};
  bool status_valid_{false};
  bool read_pending_{false};
  bool read_failed_{false};
  bool write_failed_{false};
};

}  // namespace esphome::tea5767
