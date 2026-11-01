#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome::bambu_lab_tags {

struct Spool {
  std::string uid;
  std::string material_id;
  std::string variant_id;
  std::string filament_type;
  std::string detailed_filament_type;
  std::string color;
  std::string color_name;
  std::string production_date;
  uint16_t weight{0};
  uint16_t drying_temperature{0};
  uint16_t drying_time{0};
  uint16_t bed_temperature{0};
  uint16_t min_nozzle_temperature{0};
  uint16_t max_nozzle_temperature{0};
};

// Input retains physical block offsets; sector trailers are ignored.
bool parse_spool(const uint8_t *data, size_t size, Spool &spool);

}  // namespace esphome::bambu_lab_tags
