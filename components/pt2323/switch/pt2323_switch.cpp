#include "pt2323_switch.h"
#include "esphome/core/log.h"

namespace esphome::pt2323 {

static const char *const TAG = "pt2323.switch";

void PT2323Switch::dump_config() {
  LOG_SWITCH("", "PT2323 Switch", this);
  ESP_LOGCONFIG(TAG, "  Type: %u", this->type_);
  if (this->type_ == MUTE) {
    ESP_LOGCONFIG(TAG, "  Channel A: %u", this->channel_a_);
    ESP_LOGCONFIG(TAG, "  Channel B: %u", this->channel_b_);
  }
  ESP_LOGCONFIG(TAG, "  State: %s", ONOFF(this->state));
}

void PT2323Switch::setup() {
  this->parent_->add_on_state_callback([this]() { this->update(); });
  this->publish_state(this->get_new_state_());
}

void PT2323Switch::update() {
  bool new_state = get_new_state_();
  if (this->state != new_state) {
    this->publish_state(new_state);
  }
}

void PT2323Switch::write_state(bool state) {
  switch (this->type_) {
    case ENHANCE:
      parent_->set_enhance(state);
      break;
    case BOOST:
      parent_->set_boost(state);
      break;
    case MUTE:
      if (this->channel_a_ != 0)
        parent_->mute_channel(this->channel_a_, state);
      if (this->channel_b_ != 0)
        parent_->mute_channel(this->channel_b_, state);
      break;
    case MUTE_ALL:
      parent_->mute_all_channels(state);
      break;
  }

  ESP_LOGD(TAG, "Setting switch: %s", ONOFF(state));
  this->update();
}

bool PT2323Switch::get_new_state_() {
  switch (this->type_) {
    case ENHANCE:
      return this->parent_->get_enhance();
    case BOOST:
      return this->parent_->get_boost();
    case MUTE:
      if (this->channel_a_ != 0)
        return this->parent_->get_channel_mute(this->channel_a_);
      if (this->channel_b_ != 0)
        return this->parent_->get_channel_mute(this->channel_b_);
      break;
    case MUTE_ALL:
      return this->parent_->get_mute();
  }
  return false;
}

}  // namespace esphome::pt2323
