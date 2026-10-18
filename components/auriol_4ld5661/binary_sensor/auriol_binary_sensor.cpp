#include "auriol_binary_sensor.h"

#include "esphome/core/log.h"

namespace esphome::auriol_4ld5661 {

static const char *const TAG = "auriol_4ld5661.binary_sensor";

void AuriolBatterySensor::dump_config() {
  Auriol4LD5661Base::dump_config();
  LOG_BINARY_SENSOR("  ", "Battery low", this);
}

bool AuriolBatterySensor::on_receive(remote_base::RemoteReceiveData data) {
  auto decoded = this->decode_packet_(data);
  if (!decoded.has_value())
    return false;
  this->publish_state(!decoded->battery_ok);
  return true;
}

}  // namespace esphome::auriol_4ld5661
