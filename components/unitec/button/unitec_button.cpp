#include "unitec_button.h"

#include "esphome/core/log.h"

namespace esphome::unitec {

ESPHOME_LOG_TAG(TAG, "unitec.button");

void UnitecButton::dump_config() {
  LOG_BUTTON("", "Unitec Button", this);
  ESP_LOGCONFIG(TAG, "  Address: %04X, Channel: %u", static_cast<unsigned>(this->address_),
                static_cast<unsigned>(this->channel_));
}

void UnitecButton::press_action() { this->transmit_<UnitecProtocol>({this->address_, this->channel_, this->state_}); }

}  // namespace esphome::unitec
