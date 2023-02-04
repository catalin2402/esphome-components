#include "esphome/core/log.h"
#include "PT2258Number.h"

namespace esphome {
namespace pt2258 {

static const char *const TAG = "PT2258.number";

void PT2258Number::setup() {
  switch (this->number_type_) {
    case 0:
      this->state_ = parent_->getMasterVolume();
      break;
    case 1:
      this->state_ = parent_->getOffsetFront();
      break;
    case 2:
      this->state_ = parent_->getOffsetRear();
      break;
    case 3:
      this->state_ = parent_->getOffsetCenter();
      break;
    case 4:
      this->state_ = parent_->getOffsetSubwoofer();
      break;
  }
  this->publish_state(this->state_);
  ESP_LOGD(TAG, "PT2258 reported value is: %d", this->state_);
}

void PT2258Number::control(float value) {
  if (!parent_->isPowered()) {
    ESP_LOGD(TAG, "PT2258 is turned off");
    return;
  }

  switch (this->number_type_) {
    case 0:
      parent_->setMasterVolume(value);
      break;
    case 1:
      parent_->setFrontVolume(value);
      break;
    case 2:
      parent_->setRearVolume(value);
      break;
    case 3:
      parent_->setCenterVolume(value);
      break;
    case 4:
      parent_->setSubwooferVolume(value);
      break;
  }

  this->state_ = value;
  this->publish_state(this->state_);
}

void PT2258Number::dump_config() { LOG_NUMBER("", "PT2258 Number", this); }

}  // namespace pt2258
}  // namespace esphome