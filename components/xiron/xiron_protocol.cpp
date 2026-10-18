#include "xiron_protocol.h"

#include "esphome/core/log.h"

namespace esphome::xiron {

static const char *const TAG = "remote.xiron";
static constexpr uint8_t NBITS = 36;
// Midpoint and tolerance preserve the original 250-800 us mark window.
static constexpr uint32_t BIT_MARK_US = 525;
static constexpr uint32_t BIT_MARK_TOLERANCE_US = 275;
static constexpr uint32_t BIT_ZERO_SPACE_US = 1000;
static constexpr uint32_t BIT_ONE_SPACE_US = 2000;
static constexpr uint32_t BIT_SPACE_TOLERANCE_US = 250;

void XironProtocol::encode(remote_base::RemoteTransmitData *, const XironData &) {
  ESP_LOGE(TAG, "Xiron transmission is not supported");
}

optional<XironData> XironProtocol::decode(remote_base::RemoteReceiveData src) {
  // Preserve frame discovery in captures containing sync pulses, noise or repeats.
  // Only the candidate position is advanced; pulse decoding uses expect_* helpers.
  while (src.is_valid(NBITS * 2 - 1)) {
    auto decoded = this->decode_frame_(src);
    if (decoded.has_value())
      return decoded;
    src.advance();
  }
  return {};
}

optional<XironData> XironProtocol::decode_frame_(remote_base::RemoteReceiveData src) const {
  uint64_t code = 0;
  for (uint8_t bit = 0; bit < NBITS; bit++) {
    code <<= 1;
    // These are protocol timing windows, matching the working decoder.
    src.set_tolerance(BIT_SPACE_TOLERANCE_US, remote_base::TOLERANCE_MODE_TIME);
    if (src.expect_space(BIT_ZERO_SPACE_US)) {
      // Short space: zero.
    } else if (src.expect_space(BIT_ONE_SPACE_US)) {
      code |= 1;
    } else {
      return {};
    }
    src.set_tolerance(BIT_MARK_TOLERANCE_US, remote_base::TOLERANCE_MODE_TIME);
    if (!src.expect_mark(BIT_MARK_US))
      return {};
  }

  const uint32_t payload = static_cast<uint32_t>(code >> 12);
  if (payload == 0 || ((code >> 8) & 0x0F) != 0x0F)
    return {};

  XironData result{};
  result.id = (payload >> 16) & 0x7F;
  result.battery_ok = (payload & 0x8000) != 0;
  result.status_flag = (payload & 0x4000) != 0;
  result.channel = ((payload >> 12) & 0x03) + 1;
  result.humidity = code & 0xFF;
  if (result.status_flag || result.channel > 3 || result.humidity == 0 || result.humidity > 100)
    return {};

  int16_t temperature = payload & 0x0FFF;
  if ((temperature & 0x0800) != 0)
    temperature -= 0x1000;
  if (temperature < -600 || temperature > 600)
    return {};
  result.temperature_c = temperature * 0.1f;
  return result;
}

void XironProtocol::dump(const XironData &data) {
  ESP_LOGI(TAG,
           "Received Xiron: device_id=0x%04X, id=0x%02X, channel=%u, battery_ok=%s, status_flag=%s, "
           "temperature_C=%.1f°C, humidity=%u%%",
           data.device_id(), data.id, data.channel, TRUEFALSE(data.battery_ok), TRUEFALSE(data.status_flag),
           data.temperature_c, data.humidity);
}

}  // namespace esphome::xiron
