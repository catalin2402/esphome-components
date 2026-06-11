#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

namespace esphome {
namespace pt2258 {

class PT2258 : public Component, public i2c::I2CDevice {
public:
  void dump_config() override;
  void setup() override;
  void set_default_volume(int volume);
  void set_master_volume(int volume);
  void set_channel_volume(int volume, int channel);
  int get_channel_volume(int channel, bool is_offset = false);
  void resend_data();

  // Mute support: controls the PT2258 all-channel mute register.
  void set_mute(bool mute);
  bool is_muted() const { return this->muted_; }
  void toggle_mute() { this->set_mute(!this->muted_); }

protected:
  int default_volume_ = 40;
  int volumes_[7];
  bool muted_ = false;

  void send_data();
  void send_channel_volume(int volume, int channel);
  void send_mute_state();
  uint8_t calculate_x1(int volume);
  uint8_t calculate_x10(int volume);
  bool send_individual_channels();
};

} // namespace pt2258
} // namespace esphome
