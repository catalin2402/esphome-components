#include "unitec_switch.h"

#include "esphome/core/log.h"

namespace esphome::unitec {

ESPHOME_LOG_TAG(TAG, "unitec.switch");

void UnitecSwitch::dump_config() {
  LOG_SWITCH("", "Unitec Switch", this);
  ESP_LOGCONFIG(TAG, "  Address: %04X, Channel: %u", static_cast<unsigned>(this->address_),
                static_cast<unsigned>(this->channel_));
}

void UnitecSwitch::write_state(bool state) {
  this->transmit_<UnitecProtocol>({this->address_, this->channel_, state});
  this->publish_state(state);
}

}  // namespace esphome::unitec
