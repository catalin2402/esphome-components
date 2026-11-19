#include "xiron_sensor.h"

#include "esphome/core/log.h"

namespace esphome::xiron {

ESPHOME_LOG_TAG(TAG, "xiron.sensor");

void XironSensor::dump_config() {
  XironBase::dump_config();
  LOG_SENSOR("  ", "Sensor", this);
}

bool XironSensor::on_receive(remote_base::RemoteReceiveData data) {
  auto decoded = this->decode_packet_(data);
  if (!decoded.has_value())
    return false;
  switch (this->type_) {
    case XironSensorType::XIRON_SENSOR_TYPE_TEMPERATURE:
      this->publish_state(decoded->temperature_c);
      break;
    case XironSensorType::XIRON_SENSOR_TYPE_HUMIDITY:
      this->publish_state(decoded->humidity);
      break;
  }
  return true;
}

}  // namespace esphome::xiron
