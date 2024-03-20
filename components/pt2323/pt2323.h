#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"

namespace esphome {
namespace pt2323 {

class PT2323 : public Component, public i2c::I2CDevice {
public:
  void dump_config() override;
  void setup() override;
  void set_defaults();
  void set_input(uint8_t input);
  void set_enhance(bool enhance);
  void set_boost(bool boost);
  void mute_all_channels(bool mute);
  void mute_channel(uint8_t channel, bool mute);

  uint8_t get_input() { return input_; }
  bool get_enhance() { return enhance_; }
  bool get_mute() { return mute_; }
  bool get_boost() { return boost_; }
  bool get_channel_mute(uint8_t channel) { return channels_muted_[channel]; }

private:
  uint8_t input_ = 0;
  bool enhance_ = false;
  bool boost_ = false;
  bool mute_ = false;
  void send_data(uint8_t data);
  bool channels_muted_[6];
};

} // namespace pt2323
} // namespace esphome
