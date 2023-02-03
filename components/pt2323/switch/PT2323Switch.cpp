#include "PT2323Switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *TAG = "PT2323.switch";

void PT2323Switch::dump_config() {
  LOG_SWITCH("", "PT2323 Switch", this);
  ESP_LOGCONFIG(TAG, "  Type: %u", this->type_);
  ESP_LOGCONFIG(TAG, "  Channel A: %u", this->channel_a_);
  ESP_LOGCONFIG(TAG, "  Channel B: %u", this->channel_b_);
  ESP_LOGCONFIG(TAG, "  State: %s", ONOFF(this->state_));
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