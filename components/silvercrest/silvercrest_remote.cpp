#include "silvercrest_remote.h"

#include "esphome/core/log.h"

namespace esphome::silvercrest {

static const char *const TAG = "silvercrest";

void Silvercrest::dump_config() {
  ESP_LOGCONFIG(TAG, "Silvercrest Remote:");
  ESP_LOGCONFIG(TAG, "  Repeat times: %u", this->repeat_times_);
}

void Silvercrest::control_channel(Channel channel, bool state) {
  this->transmit_<SilvercrestProtocol>({channel, state, this->repeat_times_});
}

}  // namespace esphome::silvercrest
