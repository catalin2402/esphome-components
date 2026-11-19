#include "unitec.h"

#include <algorithm>
#include <cinttypes>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::unitec {

ESPHOME_LOG_TAG(TAG, "remote.unitec");
// RFLink R51 selects the nibble permutation using bit 1 of the serial ID.
static constexpr uint8_t SUBSTITUTION[2][16] PROGMEM = {
    {0xA, 0x0, 0x4, 0xC, 0x2, 0xE, 0x7, 0x5, 0x1, 0xF, 0xB, 0xD, 0x9, 0x6, 0x3, 0x8},
    {0x2, 0xC, 0x5, 0xE, 0x7, 0x4, 0x1, 0xF, 0xB, 0xD, 0x6, 0x3, 0x8, 0x9, 0xA, 0x0},
};
static constexpr uint8_t NBITS = 24;
static constexpr uint8_t FRAME_REPEATS = 10;

static constexpr uint32_t HEADER_MARK_US = 350;
static constexpr uint32_t HEADER_SPACE_US = 2350;
static constexpr uint32_t BIT_ONE_MARK_US = 1120;
static constexpr uint32_t BIT_ONE_SPACE_US = 440;
static constexpr uint32_t BIT_ZERO_MARK_US = 350;
static constexpr uint32_t BIT_ZERO_SPACE_US = 1210;

void UnitecProtocol::encode(remote_base::RemoteTransmitData *dst, const UnitecData &data) {
  if (data.channel > 4)
    return;

  const uint16_t address = data.address;
  uint8_t table[16];
  progmem_memcpy(table, SUBSTITUTION[(address >> 1) & 1], sizeof(table));
  const uint8_t unit = table[data.channel];
  const uint8_t selector = table[unit ^ (static_cast<uint8_t>(data.state) << 1)];
  const uint8_t high = table[(address >> 12) ^ selector];
  const uint8_t middle = table[((address >> 8) & 0xF) ^ high];
  const uint8_t low = table[((address >> 4) & 0xF) ^ middle];
  const uint32_t group = (address & 0xF) ^ 0x9;
  const uint32_t frame = (group << 20) | (uint32_t{low} << 16) | (uint32_t{middle} << 12) | (uint32_t{high} << 8) |
                         (uint32_t{selector} << 4) | unit;

  dst->set_carrier_frequency(0);
  dst->reserve(FRAME_REPEATS * (2 + 2 * NBITS));
  for (uint8_t repeat = 0; repeat < FRAME_REPEATS; ++repeat) {
    dst->item(HEADER_MARK_US, HEADER_SPACE_US);
    for (uint32_t mask = 1UL << (NBITS - 1); mask != 0; mask >>= 1) {
      if ((frame & mask) != 0)
        dst->item(BIT_ONE_MARK_US, BIT_ONE_SPACE_US);
      else
        dst->item(BIT_ZERO_MARK_US, BIT_ZERO_SPACE_US);
    }
  }
}

static optional<UnitecData> decode_fields(uint32_t code) {
  const uint8_t group = (code >> 20) ^ 0x9;
  uint8_t table[16];
  progmem_memcpy(table, SUBSTITUTION[(group >> 1) & 1], sizeof(table));
  const uint8_t unit = code & 0xF;
  const uint8_t selector = (code >> 4) & 0xF;
  const uint8_t high = (code >> 8) & 0xF;
  const uint8_t middle = (code >> 12) & 0xF;
  const uint8_t low = (code >> 16) & 0xF;
  const auto invert = [&table](uint8_t value) { return std::find(table, table + 16, value) - table; };
  const auto channel = invert(unit);
  const auto command = invert(selector) ^ unit;
  if (channel > 4 || (command & 1) != 0)
    return {};
  const auto address =
      ((invert(high) ^ selector) << 12) | ((invert(middle) ^ high) << 8) | ((invert(low) ^ middle) << 4) | group;
  return UnitecData{static_cast<uint16_t>(address), static_cast<uint8_t>(channel), (command & 2) != 0, code};
}

optional<UnitecData> UnitecProtocol::decode(remote_base::RemoteReceiveData src) {
  for (; src.is_valid(); src.advance()) {
    auto frame = src;
    if (!frame.expect_item(HEADER_MARK_US, HEADER_SPACE_US))
      continue;

    uint32_t code = 0;
    uint8_t bit = 0;
    for (; bit < NBITS; ++bit) {
      code <<= 1;
      if (frame.expect_item(BIT_ONE_MARK_US, BIT_ONE_SPACE_US)) {
        code |= 1;
      } else if (!frame.expect_item(BIT_ZERO_MARK_US, BIT_ZERO_SPACE_US)) {
        // Receiver idle detection can extend or omit the last space.
        if (bit != NBITS - 1)
          break;
        if (frame.expect_mark(BIT_ONE_MARK_US))
          code |= 1;
        else if (!frame.expect_mark(BIT_ZERO_MARK_US))
          break;
      }
    }
    if (bit == NBITS) {
      if (auto data = decode_fields(code); data.has_value())
        return data;
      // Keep unrecognized payloads available to the dumper without matching a sensor.
      return UnitecData{0, 0xFF, false, code};
    }
  }
  return {};
}

void UnitecProtocol::dump(const UnitecData &data) {
  if (data.channel <= 4) {
    ESP_LOGI(TAG, "Received Unitec: address=%04X, channel=%u, command=%s, code=0x%06" PRIX32,
             static_cast<unsigned>(data.address), static_cast<unsigned>(data.channel), ONOFF(data.state), data.code);
    return;
  }
  ESP_LOGI(TAG, "Received Unitec waveform: code=0x%06" PRIX32 ", address=unknown, channel=unknown, command=unknown",
           data.code);
}

}  // namespace esphome::unitec
