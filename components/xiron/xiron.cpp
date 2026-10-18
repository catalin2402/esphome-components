#include "xiron.h"

#include "esphome/core/log.h"

namespace esphome::xiron {

static const char *const TAG = "xiron";

void XironBase::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiron:");
  if (this->has_xiron_id_) {
    ESP_LOGCONFIG(TAG, "  Device ID: 0x%04X", this->xiron_id_);
  } else {
    ESP_LOGCONFIG(TAG, "  Device ID: any");
  }
}

optional<XironData> XironBase::decode_packet_(remote_base::RemoteReceiveData data) {
  auto decoded = XironProtocol().decode(data);
  if (!decoded.has_value() || (this->has_xiron_id_ && decoded->device_id() != this->xiron_id_))
    return {};
  return decoded;
}

}  // namespace esphome::xiron
