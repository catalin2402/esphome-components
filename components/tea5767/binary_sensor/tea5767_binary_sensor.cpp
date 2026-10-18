#include "tea5767_binary_sensor.h"

#include "esphome/core/log.h"

namespace esphome::tea5767 {

static const char *const TAG = "tea5767.binary_sensor";

void TEA5767BinarySensor::setup() {
  this->parent_->add_on_status_callback(
      [this](uint64_t frequency, uint8_t level, bool stereo) { this->publish_status_(stereo); });
  if (this->parent_->has_status())
    this->publish_status_(this->parent_->is_stereo());
}

void TEA5767BinarySensor::publish_status_(bool stereo) {
  this->publish_state(this->type_ == ReceptionType::STEREO ? stereo : !stereo);
}

void TEA5767BinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "TEA5767 Reception", this);
  ESP_LOGCONFIG(TAG, "  Type: %s", this->type_ == ReceptionType::STEREO ? "Stereo" : "Mono");
}

}  // namespace esphome::tea5767
