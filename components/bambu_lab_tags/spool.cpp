#include "spool.h"
#include "color_names.h"
#include "esphome/core/helpers.h"

#include <string_view>

namespace esphome::bambu_lab_tags {

// Views only refer to the input while parsing; Spool owns the published strings.
static std::string_view text(const uint8_t *data, size_t length) {
  size_t size = 0;
  while (size < length && data[size] != 0) {
    if (data[size] < 32 || data[size] > 126)
      return {};
    size++;
  }
  while (size > 0 && data[size - 1] == ' ')
    size--;
  return {reinterpret_cast<const char *>(data), size};
}

void Spool::reserve() {
  this->uid.reserve(8);
  this->material_id.reserve(8);
  this->variant_id.reserve(8);
  this->filament_type.reserve(16);
  this->detailed_filament_type.reserve(16);
  this->color.reserve(8);
  this->color_name.reserve(max_color_name_length());
  this->production_date.reserve(16);
}

bool parse_spool(const uint8_t *data, size_t size, Spool &spool) {
  if (data == nullptr || size < 208)
    return false;
  const auto material_id = text(data + 24, 8);
  const auto variant_id = text(data + 16, 8);
  const auto filament_type = text(data + 32, 16);
  const auto detailed_filament_type = text(data + 64, 16);
  if (material_id.size() < 5 || material_id.substr(0, 2) != "GF" || filament_type.empty() ||
      detailed_filament_type.empty())
    return false;
  // Validate the mandatory fields before changing the previous successful reading.
  spool.material_id.assign(material_id);
  spool.variant_id.assign(variant_id);
  spool.filament_type.assign(filament_type);
  spool.detailed_filament_type.assign(detailed_filament_type);
  char color[format_hex_size(4)];
  format_hex_pretty_to(color, data + 80, 4, '\0');
  spool.color = color;
  const char *color_name = get_color_name(spool.material_id, spool.variant_id);
  spool.color_name = color_name != nullptr ? color_name : color;
  spool.weight = encode_value_le<uint16_t>(data + 84);
  spool.drying_temperature = encode_value_le<uint16_t>(data + 96);
  spool.drying_time = encode_value_le<uint16_t>(data + 98);
  spool.bed_temperature = encode_value_le<uint16_t>(data + 102);
  spool.max_nozzle_temperature = encode_value_le<uint16_t>(data + 104);
  spool.min_nozzle_temperature = encode_value_le<uint16_t>(data + 106);
  spool.production_date.assign(text(data + 192, 16));
  return true;
}

}  // namespace esphome::bambu_lab_tags
