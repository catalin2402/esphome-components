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

protected:
  int default_volume_ = 40;
  int volumes_[7];
  void send_data();
  void send_channel_volume(int volume, int channel);
  uint8_t calculate_x1(int volume);
  uint8_t calculate_x10(int volume);
  bool send_individual_channels();
};

} // namespace pt2258
} // namespace esphome
