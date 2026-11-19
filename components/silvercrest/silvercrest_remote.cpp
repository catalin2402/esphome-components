#include "silvercrest_remote.h"

#include "esphome/core/log.h"

namespace esphome::silvercrest {

ESPHOME_LOG_TAG(TAG, "silvercrest");

void Silvercrest::dump_config() {
  ESP_LOGCONFIG(TAG, "Silvercrest Remote:");
  ESP_LOGCONFIG(TAG, "  Repeat times: %u", this->repeat_times_);
  this->dump_callback_.call(TAG);
#ifdef USE_SILVERCREST_DUMP_PUBLISHER
  if (this->dump_publisher_ != nullptr)
    this->dump_publisher_(TAG);
#endif
}

void Silvercrest::control_channel(Channel channel, bool state) {
  this->transmit_<SilvercrestProtocol>({channel, state, this->repeat_times_});
}

}  // namespace esphome::silvercrest
