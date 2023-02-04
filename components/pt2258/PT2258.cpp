#include "pt2258.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2258 {

static const char *const TAG = "PT2258";

void PT2258::dump_config() {
  ESP_LOGCONFIG(TAG, "PT2258:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Default volume: %d", this->defaultVolume_);
}

void PT2258::setup() { setMasterVolume(this->defaultVolume_); }

void PT2258::setDefaultVolume(int volume) {
  this->defaultVolume_ = volume;
  for (int i = 0; i < 7; i++)
    this->volumes_[i] = this->defaultVolume_;
}

void PT2258::setChannelVolume(int volume, int channel) {
  this->volumes_[channel] = volume;
  sendData();
}

void PT2258::setOffsetChannelVolume(int offset, int channel) {
  this->volumes_[channel] = this->volumes_[channel] + offset;
  sendData();
}

void PT2258::setMasterVolume(int volume) {
  ESP_LOGD(TAG, "Updating master: %d", volume);
  int initialValue = this->volumes_[0];
  this->volumes_[0] = volume;
  calculateVolumes(initialValue);
}

char PT2258::calculateX1(int volume) {
  int newVolume = 79 - volume;
  return (newVolume >= 10) ? newVolume % 10 : newVolume;
}

char PT2258::calculateX10(int volume) {
  int newVolume = 79 - volume;
  return (newVolume >= 10) ? newVolume / 10 : 0;
}

bool PT2258::shouldSendIndividualChannelsVolume() {
  int masterVolume = this->volumes_[0];
  for (int i = 1; i < 7; i++)
    if (this->volumes_[i] != masterVolume) {
      return true;
    }
  return false;
}

void PT2258::calculateVolumes(int initialValue) {
  for (int i = 1; i < 7; i++) {
    ESP_LOGD(TAG, "Calculate volume for  %d, initial value %d", i,
             this->volumes_[i]);
    this->volumes_[i] = this->volumes_[0] + (initialValue - this->volumes_[i]);
    ESP_LOGD(TAG, "Calculate volume for  %d, updated value %d", i,
             this->volumes_[i]);
  }
  sendData();
}

void PT2258::sendData() {
  if (shouldSendIndividualChannelsVolume()) {
    for (int i = 1; i < 7; i++) {
      if (this->volumes_[i] > 79)
        sendChannelVolume(79, i);
      else if (this->volumes_[i] < 0)
        sendChannelVolume(0, i);
      else
        sendChannelVolume(this->volumes_[i], i);
    }
  } else {
    sendChannelVolume(this->volumes_[0], 0);
  }
}

void PT2258::sendChannelVolume(int volume, int channel) {
  int x1 = calculateX1(volume);
  int x10 = calculateX10(volume);

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

  bool result1 = this->write_byte(x10, x1);
  delay(5);
  bool result2 = this->write_byte(x10, x1);
  delay(5);

  if (!result1 && !result2) {
    ESP_LOGE(TAG, "Error writing data");
    this->status_set_warning();
  } else {
    ESP_LOGD(TAG, "Sent data: %X %X", x10, x1);
    this->status_clear_warning();
  }
}

} // namespace pt2258
} // namespace esphome