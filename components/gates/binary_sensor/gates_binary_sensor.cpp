
#include "gates_binary_sensor.h"
namespace esphome {
namespace gates {

float GatesBinarySensor::get_setup_priority() const {
  return setup_priority::AFTER_WIFI;
}

void GatesBinarySensor::setup() {
  this->state_ = this->parent_->read_pin(this->pin_);
  this->publish_state(this->state_);
}
void GatesBinarySensor::dump_config() {}
void GatesBinarySensor::update() {
  bool state = this->parent_->read_pin(this->pin_);

  if (this->state_ != state) {
    this->state_ = state;
    this->publish_state(this->state_);
  }
}

} // namespace gates
} // namespace esphome