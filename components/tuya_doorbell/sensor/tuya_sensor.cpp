#include "tuya_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tuya_doorbell {

static const char *TAG = "tuya.sensor";

void Tuya2Sensor::setup() {
  this->parent_->register_listener(
      this->sensor_id_, [this](TuyaDatapoint datapoint) {
        if (datapoint.type == TuyaDatapointType::BOOLEAN) {
          this->publish_state(datapoint.value_bool);
          ESP_LOGD(TAG, "MCU reported sensor is: %s",
                   ONOFF(datapoint.value_bool));
        } else if (datapoint.type == TuyaDatapointType::INTEGER) {
          this->publish_state(datapoint.value_int);
          ESP_LOGD(TAG, "MCU reported sensor is: %d", datapoint.value_int);
        } else if (datapoint.type == TuyaDatapointType::ENUM) {
          this->publish_state(datapoint.value_enum);
          ESP_LOGD(TAG, "MCU reported sensor is: %d", datapoint.value_enum);
        } else if (datapoint.type == TuyaDatapointType::BITMASK) {
          this->publish_state(datapoint.value_bitmask);
          ESP_LOGD(TAG, "MCU reported sensor is: %x", datapoint.value_bitmask);
        }
      });
}

void Tuya2Sensor::dump_config() {
  LOG_SENSOR("", "Tuya Sensor", this);
  ESP_LOGCONFIG(TAG, "  Sensor has datapoint ID %u", this->sensor_id_);
}

} // namespace tuya_doorbell
} // namespace esphome