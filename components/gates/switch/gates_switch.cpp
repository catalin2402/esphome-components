#include "gates_switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gates {

static const char *TAG = "gates.switch";

void GatesSwitch::setup() {
  this->state_ = this->parent_->read_passthrough_state();
  this->publish_state(this->state_);
  ESP_LOGD(TAG, "Gates switch is: %s", ONOFF(this->state_));
}

void GatesSwitch::write_state(bool state) {
  this->parent_->enable_passthrough(state);
  ESP_LOGD(TAG, "Setting switch: %s", ONOFF(state));
  this->state_ = state;
  this->publish_state(this->state_);
}

void GatesSwitch::dump_config() {
  LOG_SWITCH("", "Gates Switch", this);
  ESP_LOGCONFIG(TAG, "  Type: Passthrough");
}

} // namespace gates
} // namespace esphome