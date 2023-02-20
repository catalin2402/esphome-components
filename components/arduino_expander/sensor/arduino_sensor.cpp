
#include "arduino_sensor.h"
namespace esphome {
namespace arduino_expander {

static const char *TAG = "arduino_expander.sensor";

void ArduinoSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "ADC Sensor '%s'", this->name_.c_str());
  ESP_LOGCONFIG(TAG, "  Pin: A%u via ArduinoExpander", this->pin_);
}

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