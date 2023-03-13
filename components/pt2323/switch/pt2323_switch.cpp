#include "pt2323_switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *TAG = "pt2323.switch";

void PT2323Switch::dump_config() {
  LOG_SWITCH("", "PT2323 Switch", this);
  ESP_LOGCONFIG(TAG, "  Type: %u", this->type_);
  if (this->type_ == 2) {
    ESP_LOGCONFIG(TAG, "  Channel A: %u", this->channel_a_);
    ESP_LOGCONFIG(TAG, "  Channel B: %u", this->channel_b_);
  }
  ESP_LOGCONFIG(TAG, "  State: %s", ONOFF(this->state_));
}

void PT2323Switch::setup() {
  switch (this->type_) {
  case 0:
    this->state_ = this->parent_->getEnhance();
    break;
  case 1:
    this->state_ = this->parent_->getBoost();
    break;
  case 2:
    if (this->channel_a_ != 0)
      this->state_ = this->parent_->getChannelMute(this->channel_a_);
    if (this->channel_b_ != 0)
      this->state_ = this->parent_->getChannelMute(this->channel_b_);
    break;
  case 3:
    this->state_ = this->parent_->getMute();
    break;
  }
  this->publish_state(this->state_);
}

void PT2323Switch::update() {
  bool newState;
  switch (this->type_) {
  case 0:
    newState = this->parent_->getEnhance();
    break;
  case 1:
    newState = this->parent_->getBoost();
    break;
  case 2:
    if (this->channel_a_ != 0)
      newState = this->parent_->getChannelMute(this->channel_a_);
    if (this->channel_b_ != 0)
      newState = this->parent_->getChannelMute(this->channel_b_);
    break;
  case 3:
    newState = this->parent_->getMute();
    break;
  }
  if (this->state_ != newState) {
    this->state_ = newState;
    this->publish_state(this->state_);
  }
}

void PT2323Switch::write_state(bool state) {
  switch (this->type_) {
  case 0:
    parent_->setEnhance(state);
    break;
  case 1:
    parent_->setBoost(state);
    break;
  case 2:
    if (this->channel_a_ != 0)
      parent_->muteChannel(this->channel_a_, state);
    if (this->channel_b_ != 0)
      parent_->muteChannel(this->channel_b_, state);
    break;
  case 3:
    parent_->muteAllChannels(state);
    break;
  }

  ESP_LOGD(TAG, "Setting switch: %s", ONOFF(state));
  this->state_ = state;
  this->publish_state(this->state_);
}

} // namespace pt2323
} // namespace esphome