#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"

namespace esphome::pt2323 {

class PT2323 : public Component, public i2c::I2CDevice {
 public:
  void dump_config() override;
  void setup() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  void add_on_state_callback(std::function<void()> &&callback) { this->state_callback_.add(std::move(callback)); }
  void set_defaults();
  void add_on_input_callback(std::function<void(int)> &&callback) { this->input_callback_.add(std::move(callback)); }
  void add_on_enhance_callback(std::function<void(bool)> &&callback) {
    this->enhance_callback_.add(std::move(callback));
  }
  void add_on_boost_callback(std::function<void(bool)> &&callback) { this->boost_callback_.add(std::move(callback)); }
  void add_on_mute_callback(std::function<void(bool)> &&callback) { this->mute_callback_.add(std::move(callback)); }
  void add_on_channel_mute_callback(std::function<void(int, bool)> &&callback) {
    this->channel_mute_callback_.add(std::move(callback));
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

 protected:
  uint8_t input_{0};
  bool enhance_{false};
  bool boost_{false};
  bool mute_{false};
  void send_data_();
  CallbackManager<void()> state_callback_;
  CallbackManager<void(int)> input_callback_;
  CallbackManager<void(bool)> enhance_callback_;
  CallbackManager<void(bool)> boost_callback_;
  CallbackManager<void(bool)> mute_callback_;
  CallbackManager<void(int, bool)> channel_mute_callback_;
  bool initialized_{false};
  bool channels_muted_[6]{false, false, false, false, false, false};
};

}  // namespace esphome::pt2323
