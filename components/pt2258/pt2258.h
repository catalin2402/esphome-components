#pragma once

#include <array>

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif

namespace esphome::pt2258 {

class PT2258 final : public Component, public i2c::I2CDevice {
 public:
#ifdef USE_PT2258_STATE_PUBLISHER
  void set_state_publisher(void (*publisher)()) { this->state_publisher_ = publisher; }

  void set_dump_publisher(void (*publisher)(const char *)) { this->dump_publisher_ = publisher; }
#endif
  void dump_config() override;
  void setup() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  void set_default_volume(int volume);
  void set_master_volume(int volume);
  void set_channel_volume(int volume, int channel);
  int get_channel_volume(int channel, bool is_offset = false) const;
  void resend_data();
  void set_channel_offset(int offset, int channel);
  template<typename F> void add_on_volume_callback(F &&callback) {
    this->volume_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_mute_callback(F &&callback) { this->mute_callback_.add(std::forward<F>(callback)); }
  void set_mute(bool mute);
  bool is_muted() const { return this->muted_; }
  void toggle_mute() { this->set_mute(!this->muted_); }
  template<typename F> void add_on_state_callback(F &&callback) {
    this->state_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_dump_callback(F &&callback) { this->dump_callback_.add(std::forward<F>(callback)); }
  bool volume_in_range(int channel, int min_volume, int max_volume) const {
    if (channel < 0 || channel > 6)
      return false;
    const int volume = this->get_channel_volume(channel);
    return volume >= min_volume && volume <= max_volume;
  }
#ifdef USE_NUMBER
  void control_number(number::Number *entity, float value, uint8_t channel_a, uint8_t channel_b, bool is_offset);
#endif

 protected:
#ifdef USE_PT2258_STATE_PUBLISHER
  void (*state_publisher_)(){nullptr};
  void (*dump_publisher_)(const char *){nullptr};
#endif
  void publish_state_();
  void send_data_();
  void notify_volume_changes_(const std::array<int, 7> &previous);
  bool send_channel_volume_(int volume, uint8_t channel);
  bool send_mute_state_();
  void update_status_(bool success);

  LazyCallbackManager<void()> state_callback_;
  LazyCallbackManager<void(int, int)> volume_callback_;
  LazyCallbackManager<void(bool)> mute_callback_;
  LazyCallbackManager<void(const char *)> dump_callback_;
  std::array<int, 7> volumes_{};
  int default_volume_{0};
  bool muted_{false};
  bool initialized_{false};
  bool startup_delay_complete_{false};
};

}  // namespace esphome::pt2258
