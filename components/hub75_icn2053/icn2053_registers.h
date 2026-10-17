#pragma once

#include <cstdint>

namespace esphome::hub75_icn2053 {

using driver_reg_t = std::int16_t;

struct driver_rgb_t {
  driver_reg_t r;
  driver_reg_t g;
  driver_reg_t b;
};

inline constexpr int kIcn2053DriverBits = 16;
inline constexpr int kIcn2053CommandDelay = 16;
inline constexpr int kIcn2053DmaRowBufferCount = 4;
inline constexpr int kIcn2053SubrowPadding = 8;
inline constexpr int kIcn2053RowPadding = 8;
inline constexpr int kIcn2053FramePadding = 16;
inline constexpr int kIcn2053RowAddressBits = 5;

inline constexpr std::uint16_t kIcn2053BitR1 = 1U << 0;
inline constexpr std::uint16_t kIcn2053BitG1 = 1U << 1;
inline constexpr std::uint16_t kIcn2053BitB1 = 1U << 2;
inline constexpr std::uint16_t kIcn2053BitR2 = 1U << 3;
inline constexpr std::uint16_t kIcn2053BitG2 = 1U << 4;
inline constexpr std::uint16_t kIcn2053BitB2 = 1U << 5;
inline constexpr std::uint16_t kIcn2053BitLat = 1U << 6;
inline constexpr std::uint16_t kIcn2053BitOe = 1U << 7;
inline constexpr std::uint16_t kIcn2053BitA = 1U << 8;
inline constexpr std::uint16_t kIcn2053BitB = 1U << 9;
inline constexpr std::uint16_t kIcn2053BitC = 1U << 10;
inline constexpr std::uint16_t kIcn2053BitD = 1U << 11;
inline constexpr std::uint16_t kIcn2053BitE = 1U << 12;
inline constexpr std::uint16_t kIcn2053RgbMask =
    kIcn2053BitR1 | kIcn2053BitG1 | kIcn2053BitB1 | kIcn2053BitR2 | kIcn2053BitG2 | kIcn2053BitB2;

enum Icn2053Command : std::uint8_t {
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

enum Icn2053Register : std::uint8_t {
  ICN2053_CFG1,
  ICN2053_CFG2,
  ICN2053_CFG3,
  ICN2053_CFG4,
  ICN2053_DEBUG,
  ICN2053_REGISTER_COUNT,
};

inline constexpr driver_reg_t kIcn2053Cfg1LineCountMask = 0x1f00;
inline constexpr std::uint8_t kIcn2053Cfg1LineCountOffset = 8;

extern const std::uint8_t kIcn2053RegisterCommands[ICN2053_REGISTER_COUNT];
extern const driver_rgb_t kIcn2053RegisterDefaults[ICN2053_REGISTER_COUNT];

inline constexpr int kIcn2053VsyncLength =
    (kIcn2053CommandDelay + ICN2053_PRE_ACT + kIcn2053CommandDelay + ICN2053_EN_OP + kIcn2053CommandDelay +
     ICN2053_V_SYNC + kIcn2053CommandDelay + 1) /
    2 * 2;
inline constexpr int kIcn2053PrefixStartLength =
    (kIcn2053VsyncLength + ICN2053_PRE_ACT + kIcn2053CommandDelay + 1) / 2 * 2;

enum Icn2053ExtendedDescriptor : std::uint8_t {
  ICN2053_EXT_PREFIX,
  ICN2053_EXT_DATA,
  ICN2053_EXT_COUNT,
};

inline constexpr int kIcn2053PrefixBufferCount = 1;
inline constexpr int kIcn2053SuffixBufferCount = 1;
inline constexpr int kIcn2053SuffixDescriptorSets = 2;
inline constexpr int kIcn2053RowOeCount = 138;
inline constexpr int kIcn2053RowOeLength = kIcn2053RowOeCount * 2 + 2;

void set_icn2053_register_rgb(driver_rgb_t *driver_register, driver_reg_t value, driver_reg_t mask,
                              std::uint8_t offset);

}  // namespace esphome::hub75_icn2053
