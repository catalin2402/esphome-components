#include "silvercrest_switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace silvercrest {

static const char *TAG = "silvercrest.switch";

void SilvercrestSwitch::dump_config() {
  LOG_SWITCH("", "SilvercrestSwitch", this);
  ESP_LOGCONFIG(TAG, "  Channel: %u", this->channel_);
}

void SilvercrestSwitch::write_state(bool state) {
  ESP_LOGD(TAG, "Sending command");
  this->parent_->control_channel(this->channel_, state);
  this->publish_state(state);
}

bool SilvercrestSwitch::assumed_state() { return true; }

} // namespace silvercrest
} // namespace esphome