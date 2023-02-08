#include "gates_switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gates {

static const char *TAG = "gates.switch";

float GatesSwitch::get_setup_priority() const {
  return setup_priority::AFTER_WIFI;
}

void GatesSwitch::setup() {
  if (this->type_ == 0) {
    this->state_ = this->parent_->read_passthrough_state();
  } else {
    this->state_ = this->parent_->read_relay_state();
  }
  this->publish_state(this->state_);
  ESP_LOGD(TAG, "Gates switch is: %s", ONOFF(this->state_));
}

void GatesSwitch::update() {
  int newState = 0;
  if (this->type_ == 0) {
    newState = this->parent_->read_passthrough_state();
  } else {
    newState = this->parent_->read_relay_state();
  }

  if (this->state_ != newState) {
    this->state_ = newState;
    this->publish_state(this->state_);
  }
}

void GatesSwitch::write_state(bool state) {
  if (this->type_ == 0) {
    this->parent_->enable_passthrough(state);
  } else {
    this->parent_->enable_relay(state);
  }
  ESP_LOGD(TAG, "Setting switch: %s", ONOFF(state));
  this->state_ = state;
  this->publish_state(this->state_);
}

void GatesSwitch::dump_config() {
  LOG_SWITCH("", "Gates Switch", this);
  ESP_LOGCONFIG(TAG, "  Type: %d", this->type_);
}

} // namespace gates
} // namespace esphome