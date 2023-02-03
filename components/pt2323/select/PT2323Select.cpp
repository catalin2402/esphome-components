#include "PT2323Select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *const TAG = "PT2323.select";

void PT2323Select::setup() {
  auto value = this->at(0);
  this->publish_state(value.value());
}

void PT2323Select::control(const std::string &value) {
  auto idx = this->index_of(value);
  if (idx.has_value()) {
    uint8_t mapping = this->mappings_.at(idx.value());
    ESP_LOGV(TAG, "Setting  value to %u:%s", mapping, value.c_str());
    this->parent_->setInput(mapping);
    this->publish_state(value);
    return;
  }

  ESP_LOGW(TAG, "Invalid value %s", value.c_str());
}

void PT2323Select::dump_config() {
  LOG_SELECT("", "PT2323 Select", this);
  ESP_LOGCONFIG(TAG, "  Options are:");
  auto options = this->traits.get_options();
  for (auto i = 0; i < this->mappings_.size(); i++) {
    ESP_LOGCONFIG(TAG, "    %i: %s", this->mappings_.at(i),
                  options.at(i).c_str());
  }
}

} // namespace pt2323
} // namespace esphome