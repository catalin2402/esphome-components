#include "pt2323_select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *const TAG = "pt2323.select";

void PT2323Select::dump_config() { LOG_SELECT("", "PT2323 Select", this); }

void PT2323Select::setup() {
  size_t idx = this->parent_->get_input();
  auto &options = this->traits.get_options();

  if (idx < options.size()) {
    this->publish_state(options[idx]);
  } else {
    ESP_LOGW(TAG, "Invalid initial index %u", (unsigned)idx);
  }
}

void PT2323Select::update() {
  size_t idx = this->parent_->get_input();
  auto &options = this->traits.get_options();

  if (idx < options.size()) {
    std::string new_state = options[idx];
    if (this->current_option() != new_state) {
      this->publish_state(new_state);
    }
  }
}

void PT2323Select::control(const std::string &value) {
  auto &options = this->traits.get_options();
  auto it = std::find(options.begin(), options.end(), value);
  if (it == options.end()) {
    ESP_LOGW(TAG, "Invalid option selected: %s", value.c_str());
    return;
  }
  size_t idx = std::distance(options.begin(), it);
  this->parent_->set_input(idx);
  this->publish_state(value);
}

} // namespace pt2323
} // namespace esphome
