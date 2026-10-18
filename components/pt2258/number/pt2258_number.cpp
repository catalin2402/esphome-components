#include "pt2258_number.h"
#include <cmath>
#include "esphome/core/log.h"

namespace esphome::pt2258 {

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
  this->parent_->add_on_state_callback([this]() { this->update(); });
  this->publish_state(this->get_new_state_());
}

void PT2258Number::update() {
  float new_state = get_new_state_();
  if (new_state != this->state) {
    this->publish_state(new_state);
  }
}

void PT2258Number::control(float value) {
  if (!std::isfinite(value))
    return;
  value = roundf(clamp(value, this->traits.get_min_value(), this->traits.get_max_value()));
  switch (this->type_) {
    case MASTER:
      this->parent_->set_master_volume(value);
      break;
    case VOLUME:
      if (this->channel_a_ != 0) {
        this->parent_->set_channel_volume(value, this->channel_a_);
      }
      if (this->channel_b_ != 0) {
        this->parent_->set_channel_volume(value, this->channel_b_);
      }
      break;
    case OFFSET:
      int volume = this->parent_->get_channel_volume(0) + value;
      if (this->channel_a_ != 0) {
        this->parent_->set_channel_volume(volume, this->channel_a_);
      }
      if (this->channel_b_ != 0) {
        this->parent_->set_channel_volume(volume, this->channel_b_);
      }
      break;
  }
  this->update();
}

float PT2258Number::get_new_state_() {
  switch (this->type_) {
    case MASTER:
      return this->parent_->get_channel_volume(0);
    case VOLUME:
      if (this->channel_a_ != 0)
        return this->parent_->get_channel_volume(this->channel_a_);
      if (this->channel_b_ != 0)
        return this->parent_->get_channel_volume(this->channel_b_);
      break;
    case OFFSET:
      if (this->channel_a_ != 0)
        return this->parent_->get_channel_volume(this->channel_a_, true) - this->parent_->get_channel_volume(0, true);
      if (this->channel_b_ != 0)
        return this->parent_->get_channel_volume(this->channel_b_, true) - this->parent_->get_channel_volume(0, true);
  }
  return 0;
}

}  // namespace esphome::pt2258
