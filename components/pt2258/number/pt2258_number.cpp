#include "pt2258_number.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2258 {

static const char *const TAG = "PT2258.number";

void PT2258Number::dump_config() {
  LOG_NUMBER("", "PT2258 Number", this);
  ESP_LOGCONFIG(TAG, "  Type: %d", this->type_);
  if (this->type_ != 0) {
    ESP_LOGCONFIG(TAG, "  Channel A: %d", this->channel_a_);
    ESP_LOGCONFIG(TAG, "  Channel B: %d", this->channel_b_);
  }
}

void PT2258Number::setup() {
}

void PT2258Number::update() {
 
  int newValue;
  switch (this->type_) {
  case 0:
    newValue = this->parent_->getChannelVolume(0);
    break;
  case 1:
    if (this->channel_a_ != 0)
      newValue = this->parent_->getChannelVolume(this->channel_a_);
    if (this->channel_b_ != 0)
      newValue = this->parent_->getChannelVolume(this->channel_b_);
    break;
  }
   ESP_LOGD(TAG, "Updating number, got value: %d, actual value: %d", newValue, this->value_);
  if (newValue != this->value_) {
    this->value_ = newValue;
    this->publish_state(this->value_);
  }
}

void PT2258Number::control(float value) {
  switch (this->type_) {
  case 0:
    this->parent_->setMasterVolume(value);
    break;
  case 1:
    if (this->channel_a_ != 0) {
      this->parent_->setChannelVolume(value, this->channel_a_);
    }
    if (this->channel_b_ != 0) {
      this->parent_->setChannelVolume(value, this->channel_b_);
    }
    break;

  case 2:
    if (this->channel_a_ != 0) {
      this->parent_->setOffsetChannelVolume(value, this->channel_a_);
    }
    if (this->channel_b_ != 0) {
      this->parent_->setOffsetChannelVolume(value, this->channel_b_);
    }
    break;
  }
  this->value_ = value;
  this->publish_state(this->value_);
}

} // namespace pt2258
} // namespace esphome