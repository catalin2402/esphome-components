#include "pt2258.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2258 {

static const char *const TAG = "pt2258";

void PT2258::dump_config() {
  ESP_LOGCONFIG(TAG, "PT2258:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Default volume: %d", this->default_volume_);
}

void PT2258::setup() { set_master_volume(this->default_volume_); }

void PT2258::set_default_volume(int volume) {
  this->default_volume_ = volume;
  for (uint8_t i = 0; i < 7; i++)
    this->volumes_[i] = this->default_volume_;
  send_data();
}

int PT2258::get_channel_volume(int channel, bool is_offset) {
  if (is_offset)
    return volumes_[channel];

  int volume = volumes_[channel];
  if (volume <= 0)
    volume = 0;
  if (volume >= 79)
    volume = 79;
  return volume;
}

void PT2258::set_channel_volume(int volume, int channel) {
  this->volumes_[channel] = volume;
  send_data();
}

void PT2258::set_master_volume(int volume) {
  int old_master = this->volumes_[0];
  this->volumes_[0] = volume;
  for (uint8_t i = 1; i < 7; i++) {
    int offset = this->volumes_[i] - old_master;
    this->volumes_[i] = volume + offset;
  }
  send_data();
}

uint8_t PT2258::calculate_x1(int volume) {
  int newVolume = 79 - volume;
  return (newVolume >= 10) ? newVolume % 10 : newVolume;
}

uint8_t PT2258::calculate_x10(int volume) {
  int newVolume = 79 - volume;
  return (newVolume >= 10) ? newVolume / 10 : 0;
}

bool PT2258::send_individual_channels() {
  for (uint8_t i = 1; i < 7; i++)
    if (this->volumes_[i] != this->volumes_[0]) {
      return true;
    }
  return false;
}

void PT2258::send_data() {
  ESP_LOGD(TAG,
           "Sending volumes: Master: %d, Channel 1: %d, Channel 2: %d, "
           "Channel 3: %d, Channel 4: %d, Channel 5: %d, Channel 6: %d",
           this->volumes_[0], this->volumes_[1], this->volumes_[2],
           this->volumes_[3], this->volumes_[4], this->volumes_[5],
           this->volumes_[6]);

  if (send_individual_channels()) {
    for (uint8_t i = 1; i < 7; i++) {
      if (this->volumes_[i] > 79)
        send_channel_volume(79, i);
      else if (this->volumes_[i] < 0)
        send_channel_volume(0, i);
      else
        send_channel_volume(this->volumes_[i], i);
    }
  } else {
    send_channel_volume(this->volumes_[0], 0);
  }
}

void PT2258::send_channel_volume(int volume, int channel) {
  uint8_t x1 = calculate_x1(volume);
  uint8_t x10 = calculate_x10(volume);

  switch (channel) {
  case 0:
    x1 = x1 + 0xE0;
    x10 = x10 + 0xD0;
    break;
  case 1:
    x1 = x1 + 0x90;
    x10 = x10 + 0x80;
    break;
  case 2:
    x1 = x1 + 0x50;
    x10 = x10 + 0x40;
    break;
  case 3:
    x1 = x1 + 0x10;
    x10 = x10 + 0x00;
    break;
  case 4:
    x1 = x1 + 0x30;
    x10 = x10 + 0x20;
    break;
  case 5:
    x1 = x1 + 0x70;
    x10 = x10 + 0x60;
    break;
  case 6:
    x1 = x1 + 0xB0;
    x10 = x10 + 0xA0;
  }
  if (this->is_ready()) {
    for (uint8_t i = 0; i < 2; i++) {
      if (!this->write_byte(x10, x1)) {
        ESP_LOGE(TAG, "Error writing data");
        this->status_set_warning();
      } else {
        this->status_clear_warning();
      }
    }
  }
}

} // namespace pt2258
} // namespace esphome