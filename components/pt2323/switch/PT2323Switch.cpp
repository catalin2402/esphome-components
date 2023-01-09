#include "PT2323Switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *TAG = "PT2323.switch";

void PT2323Switch::setup() {
  switch (this->switch_type_) {
  case 0:
    this->state_ = parent_->isEnhanced();
    break;
  case 1:
    this->state_ = parent_->isBoosted();
    break;
  case 2:
    this->state_ = parent_->isMuted();
    break;
  case 3:
    this->state_ = parent_->isMutedFront();
    break;
  case 4:
    this->state_ = parent_->isMutedRear();
    break;
  case 5:
    this->state_ = parent_->isMutedSubwoofer();
    break;
  case 6:
    this->state_ = parent_->isMutedCenter();
    break;
  case 7:
    this->state_ = parent_->isPowered();
  }

  this->publish_state(this->state_);
  ESP_LOGD(TAG, "PT2323 reported switch is: %s", ONOFF(this->state_));
}

void PT2323Switch::write_state(bool state) {

  if (this->switch_type_ == 7) {
    parent_->setPower(state);
    if (state) {
      parent_->setDefaults();
    }
  }

  if (!parent_->isPowered()) {
    ESP_LOGD(TAG, "PT2323 is turned off");
    return;
  }

  switch (this->switch_type_) {
  case 0:
    parent_->setEnhance(state);
    break;
  case 1:
    parent_->setBoost(state);
    break;
  case 2:
    parent_->setMute(state);
    break;
  case 3:
    parent_->setMuteFront(state);
    break;
  case 4:
    parent_->setMuteRear(state);
    break;
  case 5:
    parent_->setMuteSubwoofer(state);
    break;
  case 6:
    parent_->setMuteCenter(state);
    break;
  }

  ESP_LOGD(TAG, "Setting switch: %s", ONOFF(state));
  this->state_ = state;
  this->publish_state(this->state_);
}

void PT2323Switch::dump_config() {
  LOG_SWITCH("", "PT2323 Switch", this);
  ESP_LOGCONFIG(TAG, "  Switch type: %u", this->switch_type_);
}

void PT2323Switch::set_switch_type(uint8_t switch_type) {
  this->switch_type_ = switch_type;
}

void PT2323Switch::set_parent(PT2323 *parent) { this->parent_ = parent; }

} // namespace pt2323
} // namespace esphome