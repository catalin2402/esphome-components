#pragma once

#include <cstdint>

namespace esphome::hub75_icn2053 {

using ICN2053RegisterValue = int16_t;

struct ICN2053RegisterRGB {
  ICN2053RegisterValue r;
  ICN2053RegisterValue g;
  ICN2053RegisterValue b;
};

inline constexpr int ICN2053_DRIVER_BITS = 16;
inline constexpr int ICN2053_COMMAND_DELAY = 16;
inline constexpr int ICN2053_DMA_ROW_BUFFER_COUNT = 4;
inline constexpr int ICN2053_SUBROW_PADDING = 8;
inline constexpr int ICN2053_ROW_PADDING = 8;
inline constexpr int ICN2053_FRAME_PADDING = 16;
inline constexpr int ICN2053_ROW_ADDRESS_BITS = 5;

inline constexpr uint16_t ICN2053_BIT_R1 = 1U << 0;
inline constexpr uint16_t ICN2053_BIT_G1 = 1U << 1;
inline constexpr uint16_t ICN2053_BIT_B1 = 1U << 2;
inline constexpr uint16_t ICN2053_BIT_R2 = 1U << 3;
inline constexpr uint16_t ICN2053_BIT_G2 = 1U << 4;
inline constexpr uint16_t ICN2053_BIT_B2 = 1U << 5;
inline constexpr uint16_t ICN2053_BIT_LAT = 1U << 6;
inline constexpr uint16_t ICN2053_BIT_OE = 1U << 7;
inline constexpr uint16_t ICN2053_BIT_A = 1U << 8;
inline constexpr uint16_t ICN2053_BIT_B = 1U << 9;
inline constexpr uint16_t ICN2053_BIT_C = 1U << 10;
inline constexpr uint16_t ICN2053_BIT_D = 1U << 11;
inline constexpr uint16_t ICN2053_BIT_E = 1U << 12;
inline constexpr uint16_t ICN2053_RGB_MASK =
    ICN2053_BIT_R1 | ICN2053_BIT_G1 | ICN2053_BIT_B1 | ICN2053_BIT_R2 | ICN2053_BIT_G2 | ICN2053_BIT_B2;

enum Icn2053Command : uint8_t {
  ICN2053_WR_DEBUG = 2,
  ICN2053_V_SYNC = 3,
  ICN2053_WR_CFG1 = 4,
  ICN2053_WR_CFG2 = 6,
  ICN2053_WR_CFG3 = 8,
  ICN2053_WR_CFG4 = 10,
  ICN2053_EN_OP = 12,
  ICN2053_DIS_OP = 13,
  ICN2053_PRE_ACT = 14,
};

enum Icn2053Register : uint8_t {
  ICN2053_CFG1,
  ICN2053_CFG2,
  ICN2053_CFG3,
  ICN2053_CFG4,
  ICN2053_DEBUG,
  ICN2053_REGISTER_COUNT,
};

inline constexpr ICN2053RegisterValue ICN2053_CFG1_LINE_COUNT_MASK = 0x1f00;
inline constexpr uint8_t ICN2053_CFG1_LINE_COUNT_OFFSET = 8;

extern const uint8_t ICN2053_REGISTER_COMMANDS[ICN2053_REGISTER_COUNT];
extern const ICN2053RegisterRGB ICN2053_REGISTER_DEFAULTS[ICN2053_REGISTER_COUNT];

inline constexpr int ICN2053_VSYNC_LENGTH =
    (ICN2053_COMMAND_DELAY + ICN2053_PRE_ACT + ICN2053_COMMAND_DELAY + ICN2053_EN_OP + ICN2053_COMMAND_DELAY +
     ICN2053_V_SYNC + ICN2053_COMMAND_DELAY + 1) /
    2 * 2;
inline constexpr int ICN2053_PREFIX_START_LENGTH =
    (ICN2053_VSYNC_LENGTH + ICN2053_PRE_ACT + ICN2053_COMMAND_DELAY + 1) / 2 * 2;

enum Icn2053ExtendedDescriptor : uint8_t {
  ICN2053_EXT_PREFIX,
  ICN2053_EXT_DATA,
  ICN2053_EXT_COUNT,
};

inline constexpr int ICN2053_PREFIX_BUFFER_COUNT = 1;
inline constexpr int ICN2053_SUFFIX_BUFFER_COUNT = 1;
inline constexpr int ICN2053_SUFFIX_DESCRIPTOR_SETS = 2;
inline constexpr int ICN2053_ROW_OE_COUNT = 138;
inline constexpr int ICN2053_ROW_OE_LENGTH = ICN2053_ROW_OE_COUNT * 2 + 2;

void set_icn2053_register_rgb(ICN2053RegisterRGB *driver_register, ICN2053RegisterValue value,
                              ICN2053RegisterValue mask, uint8_t offset);

}  // namespace esphome::hub75_icn2053
