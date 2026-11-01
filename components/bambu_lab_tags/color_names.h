#pragma once

#include <string>

namespace esphome::bambu_lab_tags {

// Returns nullptr when the material/variant pair has no known name.
const char *get_color_name(const std::string &material_id, const std::string &variant_id);

}  // namespace esphome::bambu_lab_tags
