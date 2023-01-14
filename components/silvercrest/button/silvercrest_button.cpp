#include "silvercrest_button.h"
#include "esphome/core/log.h"

namespace esphome {
namespace silvercrest {

static const char *TAG = "Silvercrest.button";

void SilvercrestButton::press_action() {
  switch (this->button_type_) {
  case 0:
    parent_->SendCode_A_ON();
    break;
  case 1:
    parent_->SendCode_B_ON();
    break;
  case 2:
    parent_->SendCode_C_ON();
    break;
  case 3:
    parent_->SendCode_D_ON();
    break;
  case 4:
    parent_->SendCode_M_ON();
    break;
  case 5:
    parent_->SendCode_A_OFF();
    break;
  case 6:
    parent_->SendCode_B_OFF();
    break;
  case 7:
    parent_->SendCode_C_OFF();
    break;
  case 8:
    parent_->SendCode_D_OFF();
    break;
  case 9:
    parent_->SendCode_M_OFF();
    break;
  }

  ESP_LOGD(TAG, "Sending command");
}

void SilvercrestButton::dump_config() {
  LOG_BUTTON("", "SilvercrestButton", this);
  ESP_LOGCONFIG(TAG, "  Button: %u", this->button_type_);
}

void SilvercrestButton::set_button(uint8_t button_type) { this->button_type_ = button_type; }

void SilvercrestButton::set_parent(Silvercrest *parent) { this->parent_ = parent; }

} // namespace silvercrest
} // namespace esphome