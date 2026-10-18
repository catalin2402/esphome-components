#include "auriol_4ld5661.h"

#include "esphome/core/log.h"

namespace esphome::auriol_4ld5661 {

static const char *const TAG = "auriol_4ld5661";

void Auriol4LD5661Base::dump_config() {
  ESP_LOGCONFIG(TAG, "Auriol 4LD5661:");
  if (this->has_auriol_id_) {
    ESP_LOGCONFIG(TAG, "  Device ID: 0x%02X", this->auriol_id_);
  } else {
    ESP_LOGCONFIG(TAG, "  Device ID: any");
  }
}

optional<AuriolData> Auriol4LD5661Base::decode_packet_(remote_base::RemoteReceiveData data) {
  auto decoded = AuriolProtocol().decode(data);
  if (!decoded.has_value() || (this->has_auriol_id_ && decoded->id != this->auriol_id_))
    return {};
  return decoded;
}

#ifdef USE_SENSOR
void AuriolSensor::dump_config() {
  Auriol4LD5661Base::dump_config();
  LOG_SENSOR("  ", "Sensor", this);
  if (this->type_ == AuriolSensorType::RAIN)
    ESP_LOGCONFIG(TAG, "  Rain calibration: %.3f mm/tip", this->mm_per_tip_);
}

bool AuriolSensor::on_receive(remote_base::RemoteReceiveData data) {
  auto decoded = this->decode_packet_(data);
  if (!decoded.has_value())
    return false;
  switch (this->type_) {
    case AuriolSensorType::TEMPERATURE:
      this->publish_state(decoded->temperature_c);
      break;
    case AuriolSensorType::RAIN:
      this->publish_state(decoded->rain_mm(this->mm_per_tip_));
      break;
    case AuriolSensorType::RAIN_TIPS:
      this->publish_state(decoded->rain);
      break;
  }
  return true;
}
#endif

}  // namespace esphome::auriol_4ld5661
