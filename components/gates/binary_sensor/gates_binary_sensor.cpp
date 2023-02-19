
#include "gates_binary_sensor.h"
namespace esphome {
namespace gates {

void GatesBinarySensor::setup() {
  this->state_ = this->parent_->digital_read(14);
  this->publish_state(this->state_);
}

void GatesBinarySensor::update() {
  bool state = this->parent_->digital_read(14);

  if (this->state_ != state) {
    this->state_ = state;
    this->publish_state(this->state_);
  }
}

} // namespace gates
} // namespace esphome