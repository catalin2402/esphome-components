#pragma once

#include "esphome/components/xiron/xiron.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome::xiron {

enum class XironSensorType : uint8_t { XIRON_SENSOR_TYPE_TEMPERATURE, XIRON_SENSOR_TYPE_HUMIDITY };

class XironSensor final : public XironBase, public sensor::Sensor {
 public:
  void set_type(XironSensorType type) { this->type_ = type; }
  bool on_receive(remote_base::RemoteReceiveData data) override;
  void dump_config() override;

 protected:
  XironSensorType type_{XironSensorType::XIRON_SENSOR_TYPE_TEMPERATURE};
};

}  // namespace esphome::xiron
