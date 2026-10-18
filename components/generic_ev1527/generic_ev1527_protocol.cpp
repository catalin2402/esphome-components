#include "generic_ev1527_protocol.h"

#include <cinttypes>

#include "esphome/core/log.h"

namespace esphome::generic_ev1527 {

static const char *const TAG = "remote.generic_ev1527";
static constexpr uint8_t NBITS = 24;
static constexpr uint8_t DATA_BITS = 4;
static constexpr uint8_t DATA_MASK = 0x0F;
static constexpr uint32_t REMOTE_ID_MAX = 0xFFFFF;
static constexpr uint32_t SYNC_HIGH_US = 350;
static constexpr uint32_t SYNC_LOW_US = 10850;
static constexpr uint32_t BIT_ZERO_HIGH_US = 350;
static constexpr uint32_t BIT_ZERO_LOW_US = 1050;
static constexpr uint32_t BIT_ONE_HIGH_US = 1050;
static constexpr uint32_t BIT_ONE_LOW_US = 350;
// Receiver idle detection may clip the nominal sync gap (e.g. idle: 4ms).
static constexpr uint32_t SYNC_GAP_US = 4000;

void GenericEV1527Protocol::encode(remote_base::RemoteTransmitData *dst, const GenericEV1527Data &data) {
  if (data.id > REMOTE_ID_MAX || data.data > DATA_MASK) {
    ESP_LOGE(TAG, "Remote ID or data is outside the 24-bit frame range");
    return;
  }
  dst->set_carrier_frequency(0);
  dst->reserve((NBITS + 1) * 2);
  const uint32_t code = data.get_code();
  for (uint32_t mask = 1UL << (NBITS - 1); mask != 0; mask >>= 1) {
    if ((code & mask) != 0) {
      dst->item(BIT_ONE_HIGH_US, BIT_ONE_LOW_US);
    } else {
      dst->item(BIT_ZERO_HIGH_US, BIT_ZERO_LOW_US);
    }
  }
  dst->item(SYNC_HIGH_US, SYNC_LOW_US);
}

optional<GenericEV1527Data> GenericEV1527Protocol::decode(remote_base::RemoteReceiveData src) {
  // Accept a leading sync space, whether complete or clipped by receiver idle.
  if (!src.expect_space(SYNC_LOW_US))
    src.expect_space(SYNC_GAP_US);

  // The sync pair is optional when idle detection splits frames at the gap.
  src.expect_pulse_with_gap(SYNC_HIGH_US, SYNC_GAP_US);

  const int32_t remaining = src.size() - static_cast<int32_t>(src.get_index());
  if (remaining < NBITS * 2)
    return {};

  uint32_t code = 0;
  for (uint8_t bit = 0; bit < NBITS; bit++) {
    code <<= 1;
    if (src.expect_item(BIT_ZERO_HIGH_US, BIT_ZERO_LOW_US)) {
      // Zero: short mark, long space.
    } else if (src.expect_item(BIT_ONE_HIGH_US, BIT_ONE_LOW_US)) {
      code |= 1;
    } else {
      return {};
    }
  }

  // Reject a longer payload unless the next item is the sync pair.
  // A final sync mark alone is also valid if the trailing idle was omitted.
  if (src.is_valid() && !src.expect_pulse_with_gap(SYNC_HIGH_US, SYNC_GAP_US)) {
    if (!src.expect_mark(SYNC_HIGH_US) || src.is_valid())
      return {};
  }

  GenericEV1527Data result{};
  result.id = code >> DATA_BITS;
  result.data = static_cast<uint8_t>(code & DATA_MASK);
  return result;
}

void GenericEV1527Protocol::dump(const GenericEV1527Data &data) {
  ESP_LOGI(TAG, "Received Generic EV1527: id=%" PRIu32 " (0x%05" PRIX32 "), data=%u, code=0x%06" PRIX32, data.id,
           data.id, data.data, data.get_code());
}

}  // namespace esphome::generic_ev1527
