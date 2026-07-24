#include "rf_remote.h"

#include "esphome/components/remote_base/rc_switch_protocol.h"

#include <cinttypes>

namespace esphome {
namespace rf_remote {

static const char *const TAG = "rf_remote";

void RFRemoteBase::dump_config() {
  ESP_LOGCONFIG(TAG, "RF Remote:");
  ESP_LOGCONFIG(TAG, "  Remote ID: 0x%05" PRIX32, this->remote_id_);
  ESP_LOGCONFIG(TAG, "  Reset time: %" PRIu32 " ms", this->reset_time_);
}

bool RFRemoteBase::decode_packet_(remote_base::RemoteReceiveData data, uint32_t *remote_id, uint8_t *command,
                                  uint32_t *code) {
  uint64_t decoded_code = 0;
  uint8_t nbits = 0;

  if (!remote_base::RC_SWITCH_PROTOCOLS[1].decode(data, &decoded_code, &nbits))
    return false;

  if (nbits != 24)
    return false;

  uint32_t code32 = static_cast<uint32_t>(decoded_code);
  uint32_t decoded_remote_id = code32 >> 6;
  uint8_t decoded_command = code32 & 0x3F;

  if (this->remote_id_ != 0 && decoded_remote_id != this->remote_id_)
    return false;

  *remote_id = decoded_remote_id;
  *command = decoded_command;
  *code = code32;

  return true;
}

bool RFRemoteSensor::on_receive(remote_base::RemoteReceiveData data) {
  uint32_t decoded_remote_id = 0;
  uint8_t command = 0;
  uint32_t code = 0;

  if (!this->decode_packet_(data, &decoded_remote_id, &command, &code))
    return false;

  ESP_LOGD(TAG, "remote_id=%05" PRIX32 " command=%u code=%06" PRIX32, decoded_remote_id, command, code);

  this->publish_state(command);

  this->set_timeout("reset", this->reset_time_, [this]() { this->publish_state(0); });

  return true;
}

bool RFRemoteBinarySensor::on_receive(remote_base::RemoteReceiveData data) {
  uint32_t decoded_remote_id = 0;
  uint8_t command = 0;
  uint32_t code = 0;

  if (!this->decode_packet_(data, &decoded_remote_id, &command, &code))
    return false;

  if (command != this->remote_button_)
    return false;

  ESP_LOGD(TAG, "remote_id=%05" PRIX32 " button=%u code=%06" PRIX32, decoded_remote_id, command, code);

  this->publish_state(true);

  this->set_timeout("reset", this->reset_time_, [this]() { this->publish_state(false); });

  return true;
}

}  // namespace rf_remote
}  // namespace esphome
