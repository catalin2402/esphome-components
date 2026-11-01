#include "spool.h"
#include "color_names.h"

#include <cstdio>

namespace esphome::bambu_lab_tags {

static std::string text(const uint8_t *data, size_t length) {
  std::string result;
  for (size_t i = 0; i < length && data[i] != 0; i++) {
    if (data[i] < 32 || data[i] > 126)
      return {};
    result += static_cast<char>(data[i]);
  }
  while (!result.empty() && result.back() == ' ')
    result.pop_back();
  return result;
}

static uint16_t le16(const uint8_t *data) { return uint16_t(data[0]) | (uint16_t(data[1]) << 8); }

bool parse_spool(const uint8_t *data, size_t size, Spool &spool) {
  if (data == nullptr || size < 208)
    return false;
  Spool result{};
  result.material_id = text(data + 24, 8);
  result.variant_id = text(data + 16, 8);
  result.filament_type = text(data + 32, 16);
  result.detailed_filament_type = text(data + 64, 16);
  if (result.material_id.size() < 5 || result.material_id.compare(0, 2, "GF") != 0 || result.filament_type.empty() ||
      result.detailed_filament_type.empty())
    return false;
  char color[9];
  snprintf(color, sizeof(color), "%02X%02X%02X%02X", data[80], data[81], data[82], data[83]);
  result.color = color;
  const char *color_name = get_color_name(result.material_id, result.variant_id);
  result.color_name = color_name != nullptr ? color_name : result.color;
  result.weight = le16(data + 84);
  result.drying_temperature = le16(data + 96);
  result.drying_time = le16(data + 98);
  result.bed_temperature = le16(data + 102);
  result.max_nozzle_temperature = le16(data + 104);
  result.min_nozzle_temperature = le16(data + 106);
  result.production_date = text(data + 192, 16);
  spool = result;
  return true;
}

}  // namespace esphome::bambu_lab_tags
