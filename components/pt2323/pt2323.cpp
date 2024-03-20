#include "pt2323.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *const TAG = "pt2323";

void PT2323::dump_config() {
  ESP_LOGCONFIG(TAG, "PT2323:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Input: %d", this->input_);
  ESP_LOGCONFIG(TAG, "  Enhance: %s", ONOFF(this->enhance_));
  ESP_LOGCONFIG(TAG, "  Boost: %s", ONOFF(this->boost_));
  ESP_LOGCONFIG(TAG, "  Mute: %s", ONOFF(this->mute_));
}

void PT2323::setup() { set_defaults(); }

void PT2323::set_input(uint8_t input) {
  this->input_ = input;
  send_data(0xC7 + this->input_);
}

void PT2323::set_enhance(bool enhance) {
  this->enhance_ = enhance;
  send_data((this->enhance_) ? 0xD0 : 0xD1);
}

void PT2323::set_boost(bool boost) {
  this->boost_ = boost;
  send_data((this->boost_) ? 0x91 : 0x90);
}

void PT2323::mute_all_channels(bool mute) {
  this->mute_ = mute;
  send_data((this->mute_) ? 0xFF : 0xFE);
}

void PT2323::mute_channel(uint8_t channel, bool mute) {
  this->channels_muted_[channel] = mute;
  switch (channel) {
  case 1:
    send_data(mute ? 0xF1 : 0xF0);
    break;
  case 2:
    send_data(mute ? 0xF3 : 0xF2);
    break;
  case 3:
    send_data(mute ? 0xF9 : 0xF8);
    break;
  case 4:
    send_data(mute ? 0xFB : 0xFA);
    break;
  case 5:
    send_data(mute ? 0xF7 : 0xF6);
    break;
  case 6:
    send_data(mute ? 0xF5 : 0xF4);
    break;
  }
}

void PT2323::set_defaults() {
  set_input(this->input_);
  set_enhance(this->enhance_);
  set_boost(this->boost_);
  mute_all_channels(this->mute_);
}

void PT2323::send_data(uint8_t data) {
  if (this->is_ready()) {
    if (!this->write_bytes(data, nullptr, 0)) {
      ESP_LOGE(TAG, "Error writing data");
      this->status_set_warning();
    } else {
      ESP_LOGD(TAG, "Sent data: %X", data);
      this->status_clear_warning();
    }
  }
}

} // namespace pt2323
} // namespace esphome
