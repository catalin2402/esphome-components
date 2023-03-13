#include "pt2258_number.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2258 {

static const char *const TAG = "pt2258.number";

void PT2258Number::dump_config() {
  LOG_NUMBER("", "PT2258 Number", this);
  ESP_LOGCONFIG(TAG, "  Type: %d", this->type_);
  if (this->type_ != 0) {
    ESP_LOGCONFIG(TAG, "  Channel A: %d", this->channel_a_);
    ESP_LOGCONFIG(TAG, "  Channel B: %d", this->channel_b_);
  }
}

void PT2258Number::setup() {
  this->state = get_new_state();
  this->publish_state(this->state);
}

void PT2258Number::update() {
  float newState = get_new_state();
  if (newState != this->state) {
    this->state = newState;
    this->publish_state(this->state);
  }
}

void PT2258Number::control(float value) {
  switch (this->type_) {
  case 0:
    this->parent_->set_master_volume(value);
    break;
  case 1:
    if (this->channel_a_ != 0) {
      this->parent_->set_channel_volume(value, this->channel_a_);
    }
    if (this->channel_b_ != 0) {
      this->parent_->set_channel_volume(value, this->channel_b_);
    }
    break;
  case 2:
    int volume = this->parent_->get_channel_volume(0) + value;
    if (this->channel_a_ != 0) {
      this->parent_->set_channel_volume(volume, this->channel_a_);
    }
    if (this->channel_b_ != 0) {
      this->parent_->set_channel_volume(volume, this->channel_b_);
    }
    break;
  }
  this->state = value;
  this->publish_state(this->state);
}

float PT2258Number::get_new_state() {
  float newState;
  switch (this->type_) {
  case 0:
    newState = this->parent_->get_channel_volume(0);
    break;
  case 1:
    if (this->channel_a_ != 0)
      newState = this->parent_->get_channel_volume(this->channel_a_);
    if (this->channel_b_ != 0)
      newState = this->parent_->get_channel_volume(this->channel_b_);
    break;
  case 2:
    if (this->channel_a_ != 0)
      newState = this->parent_->get_channel_volume(this->channel_a_, true) -
                 this->parent_->get_channel_volume(0, true);
    if (this->channel_b_ != 0)
      newState = this->parent_->get_channel_volume(this->channel_b_, true) -
                 this->parent_->get_channel_volume(0, true);
    break;
  }
  return newState;
}

} // namespace pt2258
} // namespace esphome