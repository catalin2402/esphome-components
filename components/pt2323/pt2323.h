#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"

namespace esphome::pt2323 {

class PT2323 final : public Component, public i2c::I2CDevice {
 public:
#ifdef USE_PT2323_STATE_PUBLISHER
  void set_state_publisher(void (*publisher)()) { this->state_publisher_ = publisher; }

  void set_restore_publisher(void (*publisher)()) { this->restore_publisher_ = publisher; }

  void set_dump_publisher(void (*publisher)(const char *)) { this->dump_publisher_ = publisher; }
#endif
  void dump_config() override;
  void setup() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  template<typename F> void add_on_state_callback(F &&callback) {
    this->state_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_dump_callback(F &&callback) { this->dump_callback_.add(std::forward<F>(callback)); }
  void set_defaults();
  template<typename F> void add_on_input_callback(F &&callback) {
    this->input_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_enhance_callback(F &&callback) {
    this->enhance_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_boost_callback(F &&callback) {
    this->boost_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_mute_callback(F &&callback) { this->mute_callback_.add(std::forward<F>(callback)); }
  template<typename F> void add_on_channel_mute_callback(F &&callback) {
    this->channel_mute_callback_.add(std::forward<F>(callback));
  }
  void set_input(int input);
  void set_enhance(bool enhance);
  void set_boost(bool boost);
  void mute_all_channels(bool mute);
  void mute_channel(int channel, bool mute);
  void resend_data();

  uint8_t get_input() const { return this->input_; }
  bool get_enhance() const { return this->enhance_; }
  bool get_mute() const { return this->mute_; }
  bool get_boost() const { return this->boost_; }
  bool get_channel_mute(uint8_t channel) const {
    return channel >= 1 && channel <= 6 && this->channels_muted_[channel - 1];
  }
  bool is_channel_muted(int channel) const {
    return channel >= 1 && channel <= 6 && (this->mute_ || this->get_channel_mute(channel));
  }

 protected:
#ifdef USE_PT2323_STATE_PUBLISHER
  void (*state_publisher_)(){nullptr};
  void (*restore_publisher_)(){nullptr};
  void (*dump_publisher_)(const char *){nullptr};
#endif
  void publish_state_();
  uint8_t input_{0};
  bool enhance_{false};
  bool boost_{false};
  bool mute_{false};
  void send_data_();
  LazyCallbackManager<void()> state_callback_;
  LazyCallbackManager<void(int)> input_callback_;
  LazyCallbackManager<void(bool)> enhance_callback_;
  LazyCallbackManager<void(bool)> boost_callback_;
  LazyCallbackManager<void(bool)> mute_callback_;
  LazyCallbackManager<void(int, bool)> channel_mute_callback_;
  LazyCallbackManager<void(const char *)> dump_callback_;
  bool initialized_{false};
  bool channels_muted_[6]{false, false, false, false, false, false};
};

}  // namespace esphome::pt2323
