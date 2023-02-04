#include "pt2323_select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *const TAG = "pt2323.select";

void PT2323Select::dump_config() {
  LOG_SELECT("", "PT2323 Select", this);
  ESP_LOGCONFIG(TAG, "  Selected input: %s", this->state_);
}

void PT2323Select::setup() {
  auto value = this->traits.get_options().at(this->parent_->getInput());
  this->state_ = value;
  this->publish_state(this->state_);
}

void PT2323Select::update() {
  auto newState = this->traits.get_options().at(this->parent_->getInput());
  if (this->state_ != newState) {
    this->state_ = newState;
    this->publish_state(this->state_);
  }
}

void PT2323Select::control(const std::string &value) {
  auto options = this->traits.get_options();
  auto opt_it = std::find(options.cbegin(), options.cend(), value);
  size_t idx = std::distance(options.cbegin(), opt_it);
  this->parent_->setInput(idx);
  this->state_ = value;
  this->publish_state(this->state_);
}

} // namespace pt2323
} // namespace esphome