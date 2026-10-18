#include "auriol_protocol.h"

#include <cinttypes>

#include "esphome/core/log.h"

namespace esphome::auriol_4ld5661 {

static const char *const TAG = "remote.auriol";
static constexpr uint8_t NBITS = 52;
// Provisional mark width; upstream documents the PPM spaces only.
static constexpr uint32_t BIT_MARK_US = 500;
static constexpr uint32_t BIT_ZERO_SPACE_US = 1000;
static constexpr uint32_t BIT_ONE_SPACE_US = 2000;
static constexpr uint32_t SYNC_SPACE_US = 2500;

void AuriolProtocol::encode(remote_base::RemoteTransmitData *, const AuriolData &) {
  // The existing capture establishes decoding only, not a complete transmit waveform.
  ESP_LOGE(TAG, "Auriol transmission is not supported");
}

optional<AuriolData> AuriolProtocol::decode(remote_base::RemoteReceiveData src) {
  // Some captures start at the first space, others include its preceding mark.
  src.expect_mark(BIT_MARK_US);
  const int32_t remaining = src.size() - static_cast<int32_t>(src.get_index());
  if (remaining < NBITS * 2 - 1)
    return {};

  std::array<uint8_t, 7> bytes{};
  for (uint8_t bit = 0; bit < NBITS; bit++) {
    if (src.expect_space(BIT_ZERO_SPACE_US)) {
      // Short space: zero in this interpretation.
    } else if (src.expect_space(BIT_ONE_SPACE_US)) {
      bytes[bit / 8] |= 1U << (7 - (bit % 8));
    } else {
      return {};
    }
    if (bit + 1 < NBITS && !src.expect_mark(BIT_MARK_US))
      return {};
  }

  // A row must end here, or at a delimiter mark followed by a long space.
  if (src.is_valid() && !src.expect_pulse_with_gap(BIT_MARK_US, SYNC_SPACE_US)) {
    if (!src.expect_mark(BIT_MARK_US) || src.is_valid())
      return {};
  }

  auto decoded = this->decode_data_(bytes);
  if (decoded.has_value())
    return decoded;
  // Preserve support for captures using the opposite PPM bit interpretation.
  for (auto &byte : bytes)
    byte = static_cast<uint8_t>(~byte);
  bytes.back() &= 0xF0;
  return this->decode_data_(bytes);
}

optional<AuriolData> AuriolProtocol::decode_data_(const std::array<uint8_t, 7> &bytes) const {
  if (bytes[3] != 0xF0)
    return {};

  // Sign-extend the protocol's 12-bit two's-complement temperature explicitly.
  int16_t temperature = ((bytes[1] & 0x0F) << 8) | bytes[2];
  if ((temperature & 0x0800) != 0)
    temperature -= 0x1000;
  if (temperature < -600 || temperature > 800)
    return {};

  AuriolData result{};
  result.id = bytes[0];
  result.battery_ok = (bytes[1] & 0x80) != 0;
  result.channel = (bytes[1] >> 4) & 0x03;
  result.temperature_c = temperature * 0.1f;
  result.rain = (static_cast<uint32_t>(bytes[4]) << 12) | (static_cast<uint32_t>(bytes[5]) << 4) | (bytes[6] >> 4);
  return result;
}

void AuriolProtocol::dump(const AuriolData &data) {
  ESP_LOGI(TAG, "Received Auriol: id=0x%02X, channel=%u, battery_ok=%s, temperature_C=%.1f°C, rain=%" PRIu32 " tips",
           data.id, data.channel, TRUEFALSE(data.battery_ok), data.temperature_c, data.rain);
}

}  // namespace esphome::auriol_4ld5661
