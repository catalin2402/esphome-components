#include "pt2323_select.h"
#include "esphome/core/log.h"

namespace esphome::pt2323 {

static const char *const TAG = "pt2323.select";

void PT2323Select::dump_config() { LOG_SELECT("", "PT2323 Select", this); }

void PT2323Select::setup() {
  this->parent_->add_on_state_callback([this]() { this->update(); });
  this->update();
}

void PT2323Select::update() {
  const size_t index = this->parent_->get_input();
  if (index >= this->traits.get_options().size()) {
    ESP_LOGW(TAG, "Input %u has no configured option", static_cast<unsigned>(index));
    return;
  }
  if (!this->has_state() || this->active_index() != index)
    this->publish_state(index);
}

void PT2323Select::control(size_t index) {
  if (index < this->size())
    this->parent_->set_input(index);
}

}  // namespace esphome::pt2323
