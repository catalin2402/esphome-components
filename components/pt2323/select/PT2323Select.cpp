#include "PT2323Select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pt2323 {

static const char *const TAG = "PT2323.select";

void PT2323Select::setup() {
  auto value = this->traits.get_options().at(this->parent_->getInput());
  this->publish_state(value);
}

void PT2323Select::control(const std::string &value) {
  auto idx = this->index_of(value);
  this->parent_->setInput(idx.value());
  this->publish_state(value);
}

void PT2323Select::dump_config() { LOG_SELECT("", "PT2323 Select", this); }

} // namespace pt2323
} // namespace esphome