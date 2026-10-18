#include "silvercrest_button.h"
#include "esphome/core/log.h"

namespace esphome::silvercrest {

static const char *const TAG = "silvercrest.button";

void SilvercrestButton::press_action() {
  const auto channel = static_cast<Channel>(this->button_type_ % 5);
  this->parent_->control_channel(channel, this->button_type_ < 5);

  ESP_LOGD(TAG, "Sending command");
}

void SilvercrestButton::dump_config() {
  LOG_BUTTON("", "Silvercrest Button", this);
  ESP_LOGCONFIG(TAG, "  Button: %u", this->button_type_);
}

void SilvercrestButton::set_button(uint8_t button_type) { this->button_type_ = button_type; }

void SilvercrestButton::set_parent(Silvercrest *parent) { this->parent_ = parent; }

}  // namespace esphome::silvercrest
