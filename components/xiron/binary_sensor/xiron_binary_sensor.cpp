#include "xiron_binary_sensor.h"

#include "esphome/core/log.h"

namespace esphome::xiron {

static const char *const TAG = "xiron.binary_sensor";

void XironBatterySensor::dump_config() {
  XironBase::dump_config();
  LOG_BINARY_SENSOR("  ", "Battery low", this);
}

bool XironBatterySensor::on_receive(remote_base::RemoteReceiveData data) {
  auto decoded = this->decode_packet_(data);
  if (!decoded.has_value())
    return false;
  this->publish_state(!decoded->battery_ok);
  return true;
}

}  // namespace esphome::xiron
