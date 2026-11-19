#pragma once

#include <cstddef>
#include <string>

namespace esphome::bambu_lab_tags {

// Maximum catalogue name length, including the RGBA fallback.
size_t max_color_name_length();

// Returns nullptr when the material/variant pair has no known name.
const char *get_color_name(const std::string &material_id, const std::string &variant_id);

}  // namespace esphome::bambu_lab_tags
