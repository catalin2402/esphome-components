#pragma once

#include <array>

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome::tea5767 {

enum class Setting : uint8_t {
  SETTING_MUTE,
  SETTING_MONO,
  SETTING_SOFT_MUTE,
  SETTING_HIGH_CUT_CONTROL,
  SETTING_STEREO_NOISE_CANCELING,
};

class TEA5767 final : public PollingComponent, public i2c::I2CDevice {
 public:
#ifdef USE_TEA5767_STATE_PUBLISHER
  void set_state_publisher(void (*publisher)()) { this->state_publisher_ = publisher; }

  void set_restore_publisher(void (*publisher)()) { this->restore_publisher_ = publisher; }
#endif
#ifdef USE_TEA5767_STATUS_PUBLISHER
  void set_status_publisher(void (*publisher)(uint64_t, uint8_t, bool)) { this->status_publisher_ = publisher; }
#endif
#if defined(USE_TEA5767_STATE_PUBLISHER) || defined(USE_TEA5767_STATUS_PUBLISHER)
  void set_dump_publisher(void (*publisher)(const char *)) { this->dump_publisher_ = publisher; }
#endif
  void dump_config() override;
  void setup() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_in_japan(bool in_japan) { this->in_japan_ = in_japan; }
  void set_frequency(uint64_t frequency);
  void set_mono(bool mono) { this->set_setting_(Setting::SETTING_MONO, mono); }
  void set_mute(bool mute) { this->set_setting_(Setting::SETTING_MUTE, mute); }
  void set_soft_mute(bool soft_mute) { this->set_setting_(Setting::SETTING_SOFT_MUTE, soft_mute); }
  void set_high_cut_control(bool enabled) { this->set_setting_(Setting::SETTING_HIGH_CUT_CONTROL, enabled); }
  void set_stereo_noise_canceling(bool enabled) {
    this->set_setting_(Setting::SETTING_STEREO_NOISE_CANCELING, enabled);
  }
  void toggle_mute() { this->set_mute(!this->is_muted()); }
  void resend_data();

  uint64_t get_requested_frequency() const { return this->frequency_; }
  uint64_t get_frequency() const;
  uint8_t get_level() const { return this->status_[3] >> 4; }
  bool has_status() const { return this->status_valid_; }
  bool is_stereo() const { return this->status_valid_ && (this->status_[2] & 0x80) != 0; }
  bool get_setting(Setting setting) const { return this->settings_[static_cast<uint8_t>(setting)]; }
  bool is_muted() const { return this->get_setting(Setting::SETTING_MUTE); }
  bool is_mono() const { return this->get_setting(Setting::SETTING_MONO); }
  bool is_soft_muted() const { return this->get_setting(Setting::SETTING_SOFT_MUTE); }
  bool get_high_cut_control() const { return this->get_setting(Setting::SETTING_HIGH_CUT_CONTROL); }
  bool get_stereo_noise_canceling() const { return this->get_setting(Setting::SETTING_STEREO_NOISE_CANCELING); }

  template<typename F> void add_on_status_callback(F &&callback) {
    this->status_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_state_callback(F &&callback) {
    this->state_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_frequency_callback(F &&callback) {
    this->frequency_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_setting_callback(F &&callback) {
    this->setting_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_stereo_callback(F &&callback) {
    this->stereo_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_level_callback(F &&callback) {
    this->level_callback_.add(std::forward<F>(callback));
  }

  template<typename F> void add_on_dump_callback(F &&callback) { this->dump_callback_.add(std::forward<F>(callback)); }
  bool frequency_in_range(uint64_t min_frequency, uint64_t max_frequency) const {
    return this->frequency_ >= min_frequency && this->frequency_ <= max_frequency;
  }
  bool signal_level_in_range(int min_level, int max_level) const {
    const uint8_t level = this->get_level();
    return this->has_status() && level >= min_level && level <= max_level;
  }

 protected:
#ifdef USE_TEA5767_STATE_PUBLISHER
  void (*state_publisher_)(){nullptr};
#endif
#ifdef USE_TEA5767_STATE_PUBLISHER
  void (*restore_publisher_)(){nullptr};
#endif
#ifdef USE_TEA5767_STATUS_PUBLISHER
  void (*status_publisher_)(uint64_t, uint8_t, bool){nullptr};
#endif
#if defined(USE_TEA5767_STATE_PUBLISHER) || defined(USE_TEA5767_STATUS_PUBLISHER)
  void (*dump_publisher_)(const char *){nullptr};
#endif
  void publish_state_();
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
  LazyCallbackManager<void(const char *)> dump_callback_;
  uint64_t frequency_{87500000};
  bool in_japan_{false};
  bool initialized_{false};
  bool status_valid_{false};
  bool read_pending_{false};
  bool read_failed_{false};
  bool write_failed_{false};
};

}  // namespace esphome::tea5767
