#pragma once

#include <array>

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome::pt2258 {

class PT2258 : public Component, public i2c::I2CDevice {
 public:
  void dump_config() override;
  void setup() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  void set_default_volume(int volume);
  void set_master_volume(int volume);
  void set_channel_volume(int volume, int channel);
  int get_channel_volume(int channel, bool is_offset = false) const;
  void resend_data();
  void set_channel_offset(int offset, int channel);
  void add_on_volume_callback(std::function<void(int, int)> &&callback) {
    this->volume_callback_.add(std::move(callback));
  }
  void add_on_mute_callback(std::function<void(bool)> &&callback) { this->mute_callback_.add(std::move(callback)); }
  void set_mute(bool mute);
  bool is_muted() const { return this->muted_; }
  void toggle_mute() { this->set_mute(!this->muted_); }
  void add_on_state_callback(std::function<void()> &&callback) { this->state_callback_.add(std::move(callback)); }

 protected:
  void send_data_();
  void notify_volume_changes_(const std::array<int, 7> &previous);
  bool send_channel_volume_(int volume, uint8_t channel);
  bool send_mute_state_();
  void update_status_(bool success);

  CallbackManager<void()> state_callback_;
  CallbackManager<void(int, int)> volume_callback_;
  CallbackManager<void(bool)> mute_callback_;
  std::array<int, 7> volumes_{};
  int default_volume_{0};
  bool muted_{false};
  bool initialized_{false};
  bool startup_delay_complete_{false};
};

}  // namespace esphome::pt2258
