#include "pt2323.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *const TAG = "PT2323";

void PT2323::setup() { setDefaults(); }

void PT2323::dump_config() {
  ESP_LOGCONFIG(TAG, "PT2323:");
  ESP_LOGCONFIG(TAG, "  Input: %d", this->input_);
  ESP_LOGCONFIG(TAG, "  Enhance: %s", ONOFF(this->enhance_));
  ESP_LOGCONFIG(TAG, "  Boost: %s", ONOFF(this->boost_));
  ESP_LOGCONFIG(TAG, "  Mute: %s", ONOFF(this->mute_));
  LOG_I2C_DEVICE(this);
}

void PT2323::setInput(int input) {
  this->input_ = input;
  sendData(0xC7 + this->input_);
}

void PT2323::setEnhance(bool enhance) {
  this->enhance_ = enhance;
  sendData((this->enhance_) ? 0xD0 : 0xD1);
}

void PT2323::setBoost(bool boost) {
  this->boost_ = boost;
  sendData((this->boost_) ? 0x91 : 0x90);
}

void PT2323::muteAllChannels(bool mute) {
  this->mute_ = mute;
  sendData((this->mute_) ? 0xFF : 0xFE);
}

void PT2323::muteChannel(int channel, bool mute) {
  this->channelsMuted_[channel] = mute;
  switch (channel) {
  case 1:
    sendData(mute ? 0xF1 : 0xF0);
    break;
  case 2:
    sendData(mute ? 0xF3 : 0xF2);
    break;
  case 3:
    sendData(mute ? 0xF9 : 0xF8);
    break;
  case 4:
    sendData(mute ? 0xFB : 0xFA);
    break;
  case 5:
    sendData(mute ? 0xF7 : 0xF6);
    break;
  case 6:
    sendData(mute ? 0xF5 : 0xF4);
    break;
  }
}

void PT2323::setDefaults() {
  setInput(this->input_);
  setEnhance(this->enhance_);
  setBoost(this->boost_);
  muteAllChannels(this->mute_);
}

void PT2323::sendData(uint8_t data) {
  if (!this->write_bytes(data, nullptr, 0)) {
    ESP_LOGE(TAG, "Error writing data");
    this->status_set_warning();
  } else {
    ESP_LOGD(TAG, "Sent data: %X", data);
    this->status_clear_warning();
  }
}

} // namespace pt2323
} // namespace esphome