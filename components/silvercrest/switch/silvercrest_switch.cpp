#include "silvercrest_switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace silvercrest {

static const char *TAG = "Silvercrest.switch";

void SilvercrestSwitch::write_state(bool state) {
  switch (this->channel_) {
  case 0:
    if (state)
      parent_->SendCode_A_ON();
    else
      parent_->SendCode_A_OFF();
    break;
  case 1:
    if (state)
      parent_->SendCode_B_ON();
    else
      parent_->SendCode_B_OFF();
    break;
  case 2:
    if (state)
      parent_->SendCode_C_ON();
    else
      parent_->SendCode_C_OFF();
    break;
  case 3:
    if (state)
      parent_->SendCode_D_ON();
    else
      parent_->SendCode_D_OFF();
    break;
  case 4:
    if (state)
      parent_->SendCode_M_ON();
    else
      parent_->SendCode_M_OFF();
    break;
  }

  this->publish_state(state);

  ESP_LOGD(TAG, "Sending command");
}

void SilvercrestSwitch::dump_config() {
  LOG_SWITCH("", "SilvercrestSwitch", this);
  ESP_LOGCONFIG(TAG, "  Channel: %u", this->channel_);
}

void SilvercrestSwitch::set_channel(uint8_t channel) {
  this->channel_ = channel;
}

void SilvercrestSwitch::set_parent(Silvercrest *parent) {
  this->parent_ = parent;
}

bool SilvercrestSwitch::assumed_state() { return true; }
} // namespace silvercrest
} // namespace esphome