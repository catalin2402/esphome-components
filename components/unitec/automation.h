#pragma once

#include "unitec.h"
#include "esphome/core/automation.h"

namespace esphome::unitec {

template<typename... Ts> class UnitecAction : public remote_base::RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint16_t, address)
  TEMPLATABLE_VALUE(uint8_t, channel)
  TEMPLATABLE_VALUE(bool, state)

  void encode(remote_base::RemoteTransmitData *dst, Ts... x) override {
    UnitecProtocol().encode(dst, {this->address_.value(x...), this->channel_.value(x...), this->state_.value(x...)});
  }
};

}  // namespace esphome::unitec
