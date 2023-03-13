#include "pt2323_select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *const TAG = "pt2323.select";

void PT2323Select::dump_config() { LOG_SELECT("", "PT2323 Select", this); }

void PT2323Select::setup() {
  auto value = this->traits.get_options().at(this->parent_->get_input());
  this->state = value;
  this->publish_state(this->state);
}

void PT2323Select::update() {
  auto newState = this->traits.get_options().at(this->parent_->get_input());
  if (this->state != newState) {
    this->state = newState;
    this->publish_state(this->state);
  }
}

void PT2323Select::control(const std::string &value) {
  auto options = this->traits.get_options();
  auto opt_it = std::find(options.cbegin(), options.cend(), value);
  size_t idx = std::distance(options.cbegin(), opt_it);
  this->parent_->set_input(idx);
  this->state = value;
  this->publish_state(this->state);
}

} // namespace pt2323
} // namespace esphome