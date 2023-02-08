#include "gates_button.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gates {

static const char *TAG = "gates.button";

float GatesButton::get_setup_priority() const {
  return setup_priority::AFTER_WIFI;
}


void GatesButton::press_action() {
  this->parent_->send_code();
  ESP_LOGD(TAG, "Sending command");
}

void GatesButton::dump_config() { LOG_BUTTON("", "Gates Button", this); }

} // namespace gates
} // namespace esphome