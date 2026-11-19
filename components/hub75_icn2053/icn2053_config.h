#pragma once

#include <cstdint>

namespace esphome::hub75_icn2053 {

inline constexpr uint8_t ICN2053_ROWS_IN_PARALLEL = 2;
inline constexpr uint8_t ICN2053_DEFAULT_BRIGHTNESS = 255;

struct Hub75Pins {
  int8_t r1;
  int8_t g1;
  int8_t b1;
  int8_t r2;
  int8_t g2;
  int8_t b2;
  int8_t a;
  int8_t b;
  int8_t c;
  int8_t d;
  int8_t e;
  int8_t lat;
  int8_t oe;
  int8_t clk;
};

struct Hub75Config {
  uint16_t panel_width;
  uint16_t panel_height;
  uint16_t chain_length_x;
  uint16_t chain_length_y;
  Hub75Pins gpio;
  uint32_t clock_hz;
  bool clock_phase_inverted;
};

}  // namespace esphome::hub75_icn2053
