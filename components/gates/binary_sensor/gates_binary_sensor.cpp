#include "gates_binary_sensor.h"
namespace esphome {
namespace gates {

void GatesBinarySensor::setup() {
  this->state = get_new_state_();
  this->publish_state(this->state);
}

void GatesBinarySensor::update() {
  bool new_state = get_new_state_();
  if (this->state != new_state) {
    this->state = new_state;
    this->publish_state(this->state);
  }
}

bool GatesBinarySensor::get_new_state_() {
  return (this->type_ == DUAL_GATE) ? this->parent_->digital_read(14)
                                    : this->parent_->digital_read(15);
}

} // namespace gates
} // namespace esphome