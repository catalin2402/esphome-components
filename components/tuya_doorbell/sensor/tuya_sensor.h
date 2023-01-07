#pragma once

#include "../tuya_doorbell.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace tuya_doorbell {

class Tuya2Sensor : public sensor::Sensor, public Component {
public:
  void setup() override;
  void dump_config() override;
  void set_sensor_id(uint8_t sensor_id) { this->sensor_id_ = sensor_id; }

  void set_tuya_parent(TuyaDoorbell *parent) { this->parent_ = parent; }

protected:
  TuyaDoorbell *parent_;
  uint8_t sensor_id_{0};
};

} // namespace tuya_doorbell
} // namespace esphome