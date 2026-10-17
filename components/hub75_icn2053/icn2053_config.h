#pragma once

#include <cstdint>

namespace esphome::hub75_icn2053 {

inline constexpr std::uint8_t kIcn2053RowsInParallel = 2;
inline constexpr std::uint8_t kIcn2053DefaultBrightness = 255;

struct hub75_pins_t {
  std::int8_t r1;
  std::int8_t g1;
  std::int8_t b1;
  std::int8_t r2;
  std::int8_t g2;
  std::int8_t b2;
  std::int8_t a;
  std::int8_t b;
  std::int8_t c;
  std::int8_t d;
  std::int8_t e;
  std::int8_t lat;
  std::int8_t oe;
  std::int8_t clk;
};

struct hub75_i2s_cfg_t {
  std::uint16_t panel_width;
  std::uint16_t panel_height;
  std::uint16_t chain_length_x;
  std::uint16_t chain_length_y;
  hub75_pins_t gpio;
  std::uint32_t clock_hz;
  bool clock_phase_inverted;
};

}  // namespace esphome::hub75_icn2053
