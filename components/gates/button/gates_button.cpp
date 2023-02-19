#include "gates_button.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gates {

static const char *TAG = "gates.button";

void GatesButton::dump_config() {
  LOG_BUTTON("", "Gates Button", this);
  ESP_LOGCONFIG(TAG, "  Type: %s",
             (this->type_ == 0) ? "Send code" : "Retransmit code");
}

void GatesButton::press_action() {
  if (this->type_ == 0) {
    this->parent_->send_code();
  } else {
    this->parent_->retransmit_code();
  }
  ESP_LOGD(TAG, "Sending command");
}

} // namespace gates
} // namespace esphome