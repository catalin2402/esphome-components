#include "PT2258Switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2258 {

static const char *TAG = "PT2258.switch";

void PT2258Switch::setup() {

  this->state_ = parent_->isPowered();

  this->publish_state(this->state_);
  ESP_LOGD(TAG, "PT2258 reported switch is: %s", ONOFF(this->state_));
}

void PT2258Switch::write_state(bool state) {
  parent_->setPower(state);
  if (state) {
    parent_->setDefaults();
  }

  ESP_LOGD(TAG, "Setting switch: %s", ONOFF(state));
  this->state_ = state;
  this->publish_state(this->state_);
}

void PT2258Switch::dump_config() {
  LOG_SWITCH("", "PT2358 Switch", this);
}

void PT2258Switch::set_parent(PT2258 *parent) { this->parent_ = parent; }

} // namespace pt2258
} // namespace esphome