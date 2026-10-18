#include "tea5767_sensor.h"

#include "esphome/core/log.h"

namespace esphome::tea5767 {

static const char *const TAG = "tea5767.sensor";

void TEA5767Sensor::setup() {
  this->parent_->add_on_status_callback(
      [this](uint64_t frequency, uint8_t level, bool stereo) { this->publish_status_(frequency, level); });
  if (this->parent_->has_status())
    this->publish_status_(this->parent_->get_frequency(), this->parent_->get_level());
}

void TEA5767Sensor::publish_status_(uint64_t frequency, uint8_t level) {
  switch (this->type_) {
    case SensorType::FREQUENCY:
      this->publish_state(frequency);
      break;
    case SensorType::SIGNAL_LEVEL:
      this->publish_state(level);
      break;
  }
}

void TEA5767Sensor::dump_config() {
  LOG_SENSOR("", "TEA5767 Sensor", this);
  ESP_LOGCONFIG(TAG, "  Type: %s", this->type_ == SensorType::FREQUENCY ? "Frequency" : "Signal level");
}

}  // namespace esphome::tea5767
