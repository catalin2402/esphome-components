
#include "arduino_sensor.h"
namespace esphome {
namespace arduino_expander {

void ArduinoSensor::setup() {
  this->state_ = this->parent_->read_analog(this->pin_);
  this->publish_state(this->state_);
}

void ArduinoSensor::update() {
  uint16_t state = this->parent_->read_analog(this->pin_);
  if (this->state_ != state) {
    this->state_ = state;
    this->publish_state(this->state_);
  }
}

} // namespace arduino_expander
} // namespace esphome