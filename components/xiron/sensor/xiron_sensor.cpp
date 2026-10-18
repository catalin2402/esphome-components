#include "xiron_sensor.h"

#include "esphome/core/log.h"

namespace esphome::xiron {

static const char *const TAG = "xiron.sensor";

void XironSensor::dump_config() {
  XironBase::dump_config();
  LOG_SENSOR("  ", "Sensor", this);
}

bool XironSensor::on_receive(remote_base::RemoteReceiveData data) {
  auto decoded = this->decode_packet_(data);
  if (!decoded.has_value())
    return false;
  switch (this->type_) {
    case XironSensorType::TEMPERATURE:
      this->publish_state(decoded->temperature_c);
      break;
    case XironSensorType::HUMIDITY:
      this->publish_state(decoded->humidity);
      break;
  }
  return true;
}

}  // namespace esphome::xiron
