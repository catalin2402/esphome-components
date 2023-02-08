#include "PT2323.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *const TAG = "PT2323";

void PT2323::setup() { setDefaults(); }

void PT2323::dump_config() {
  ESP_LOGCONFIG(TAG, "PT2323:");
  ESP_LOGCONFIG(TAG, "  Input: %d", this->input_);
  ESP_LOGCONFIG(TAG, "  Enhance: %s", this->enhance_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Boost: %s", this->boost_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Mute: %s", this->mute_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Mute Front: %s", this->muteFront_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Mute Rear: %s", this->muteRear_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Mute Subwoofer: %s",
                this->muteSubwoofer_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Mute Center: %s", this->muteCenter_ ? "true" : "false");
  LOG_I2C_DEVICE(this);
}

void PT2323::setPower(bool power) { this->power_ = power; }

void PT2323::setInput(int input) {
  ESP_LOGD(TAG, "Setting input to %d", input);
  this->input_ = input;
  sendData(0xC7 + this->input_);
}

void PT2323::setEnhance(bool enhance) {
  this->enhance_ = enhance;
  ESP_LOGD(TAG, "Setting enhance to %s", enhance ? "true" : "false");
  sendData((this->enhance_) ? 0xD0 : 0xD1);
}

void PT2323::setBoost(bool boost) {
  this->boost_ = boost;
  ESP_LOGD(TAG, "Setting boost to %s", boost ? "true" : "false");
  sendData((this->boost_) ? 0x91 : 0x90);
}

void PT2323::setMute(bool mute) {
  this->mute_ = mute;
  ESP_LOGD(TAG, "Setting mute to %s", mute ? "true" : "false");
  sendData((this->mute_) ? 0xFF : 0xFE);
}

void PT2323::setMuteFront(bool mute) {
  this->muteFront_ = mute;
  ESP_LOGD(TAG, "Setting mute front to %s", mute ? "true" : "false");
  sendData((this->muteFront_) ? 0xF1 : 0xF0);
  sendData((this->muteFront_) ? 0xF3 : 0xF2);
}

void PT2323::setMuteRear(bool mute) {
  this->muteRear_ = mute;
  ESP_LOGD(TAG, "Setting mute rear to %s", mute ? "true" : "false");
  sendData((this->muteRear_) ? 0xF9 : 0xF8);
  sendData((this->muteRear_) ? 0xFB : 0xFA);
}

void PT2323::setMuteSubwoofer(bool mute) {
  this->muteSubwoofer_ = mute;
  ESP_LOGD(TAG, "Setting mute subwoofer to %s", mute ? "true" : "false");
  sendData((this->muteSubwoofer_) ? 0xF7 : 0xF6);
}

void PT2323::setMuteCenter(bool mute) {
  this->muteCenter_ = mute;
  ESP_LOGD(TAG, "Setting mute front to %s", mute ? "true" : "false");
  sendData((this->muteCenter_) ? 0xF5 : 0xF4);
}

void PT2323::setDefaults() {
  setInput(this->input_);
  sendData((this->enhance_) ? 0xD0 : 0xD1);
  sendData((this->boost_) ? 0x91 : 0x90);
  sendData((this->mute_) ? 0xFF : 0xFE);
}

bool PT2323::isEnhanced() { return this->enhance_; }

bool PT2323::isBoosted() { return this->boost_; }

bool PT2323::isMuted() { return this->mute_; }

bool PT2323::isMutedFront() { return this->muteFront_; }

bool PT2323::isMutedRear() { return this->muteRear_; }

bool PT2323::isMutedSubwoofer() { return this->muteSubwoofer_; }

bool PT2323::isMutedCenter() { return this->muteCenter_; }

bool PT2323::isPowered() { return this->power_; }

int PT2323::getSelectedInput() { return this->input_; }

void PT2323::sendData(uint8_t data) {
  if (!this->write_bytes(data, nullptr, 0)) {
    ESP_LOGE(TAG, "Error writing data");
    this->status_set_warning();
  } else {
    this->status_clear_warning();
  }
}

} // namespace pt2323
} // namespace esphome
