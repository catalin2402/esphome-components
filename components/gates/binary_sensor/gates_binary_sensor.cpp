
#include "gates_binary_sensor.h"
namespace esphome {
namespace gates {

void GatesBinarySensor::setup() {
  bool state = false;
  if (this->type_ == 0) {
    state = this->parent_->digital_read(14);
  } else {
    state = this->parent_->digital_read(15);
  }
  this->state_ = state;
  this->publish_state(this->state_);
}

void GatesBinarySensor::update() {
  bool state = false;
  if (this->type_ == 0) {
    state = this->parent_->digital_read(14);
  } else {
    state = this->parent_->digital_read(15);
  }

  if (this->state_ != state) {
    this->state_ = state;
    this->publish_state(this->state_);
  }
}

} // namespace gates
} // namespace esphome