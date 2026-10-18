#include "silvercrest_protocol.h"

#include "esphome/core/log.h"

namespace esphome::silvercrest {

static const char *const TAG = "remote.silvercrest";
static constexpr uint8_t NBITS = 24;
static constexpr uint32_t HEADER_MARK_US = 288;
static constexpr uint32_t HEADER_SPACE_US = 2232;
static constexpr uint32_t BIT_ONE_MARK_US = 288;
static constexpr uint32_t BIT_ONE_SPACE_US = 1236;
static constexpr uint32_t BIT_ZERO_MARK_US = 1056;
static constexpr uint32_t BIT_ZERO_SPACE_US = 468;
static constexpr uint32_t FOOTER_MARK_US = 2952;

// Rows are A, B, C, D, master; columns are off, on.
static constexpr uint32_t CODES[5][2] = {
    {0xE759DF, 0xE976CF}, {0xE759DB, 0xE976CB}, {0xE21CE3, 0xEADD43}, {0xE5AE8D, 0xEDE15D}, {0xE976C5, 0xE1F865},
};

void SilvercrestProtocol::encode(remote_base::RemoteTransmitData *dst, const SilvercrestData &data) {
  if (data.channel > MASTER || data.repeat_times == 0)
    return;
  const uint32_t code = CODES[data.channel][data.state];
  dst->set_carrier_frequency(0);
  dst->reserve(data.repeat_times * (2 + NBITS * 2) + 1);
  for (uint8_t repeat = 0; repeat < data.repeat_times; repeat++) {
    dst->item(HEADER_MARK_US, HEADER_SPACE_US);
    for (uint32_t mask = 1UL << (NBITS - 1); mask != 0; mask >>= 1) {
      if ((code & mask) != 0)
        dst->item(BIT_ONE_MARK_US, BIT_ONE_SPACE_US);
      else
        dst->item(BIT_ZERO_MARK_US, BIT_ZERO_SPACE_US);
    }
  }
  dst->mark(FOOTER_MARK_US);
}

optional<SilvercrestData> SilvercrestProtocol::decode(remote_base::RemoteReceiveData src) {
  if (!src.expect_item(HEADER_MARK_US, HEADER_SPACE_US))
    return {};
  if (src.size() - src.get_index() < NBITS * 2)
    return {};
  uint32_t code = 0;
  for (uint8_t bit = 0; bit < NBITS; bit++) {
    code <<= 1;
    if (src.expect_item(BIT_ONE_MARK_US, BIT_ONE_SPACE_US))
      code |= 1;
    else if (!src.expect_item(BIT_ZERO_MARK_US, BIT_ZERO_SPACE_US))
      return {};
  }
  // A capture may end after one packet, contain more repetitions, or include the final mark.
  for (uint8_t channel = A; channel <= MASTER; channel++) {
    for (uint8_t state = 0; state < 2; state++) {
      if (code == CODES[channel][state])
        return SilvercrestData{static_cast<Channel>(channel), state != 0};
    }
  }
  return {};
}

void SilvercrestProtocol::dump(const SilvercrestData &data) {
  ESP_LOGI(TAG, "Received Silvercrest: channel=%s, state=%s",
           data.channel == MASTER ? "master"
                                  : (data.channel == A   ? "A"
                                     : data.channel == B ? "B"
                                     : data.channel == C ? "C"
                                                         : "D"),
           ONOFF(data.state));
}

}  // namespace esphome::silvercrest
