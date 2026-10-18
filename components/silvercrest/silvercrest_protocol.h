#pragma once

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/defines.h"
#include "esphome/core/version.h"

namespace esphome::silvercrest {

enum Channel : uint8_t { A, B, C, D, MASTER };

struct SilvercrestData {
  Channel channel;
  bool state;
  uint8_t repeat_times{5};
  bool operator==(const SilvercrestData &rhs) const { return this->channel == rhs.channel && this->state == rhs.state; }
};

class SilvercrestProtocol : public remote_base::RemoteProtocol<SilvercrestData> {
 public:
#if ESPHOME_VERSION_CODE < VERSION_CODE(2026, 10, 0)
  void encode(remote_base::RemoteTransmitData *dst, const SilvercrestData &data) override;
  optional<SilvercrestData> decode(remote_base::RemoteReceiveData src) override;
  void dump(const SilvercrestData &data) override;
#else
  void encode(remote_base::RemoteTransmitData *dst, const SilvercrestData &data);
  optional<SilvercrestData> decode(remote_base::RemoteReceiveData src);
  void dump(const SilvercrestData &data);
#endif
};

#ifdef USE_BINARY_SENSOR
using SilvercrestBinarySensor = remote_base::RemoteReceiverBinarySensor<SilvercrestProtocol>;
#endif
using SilvercrestDumper = remote_base::RemoteReceiverDumper<SilvercrestProtocol>;

template<typename... Ts> class SilvercrestAction : public remote_base::RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(Channel, channel)
  TEMPLATABLE_VALUE(bool, state)
  TEMPLATABLE_VALUE(uint8_t, repeat_times)

  void encode(remote_base::RemoteTransmitData *dst, Ts... x) override {
    SilvercrestProtocol().encode(
        dst, {this->channel_.value(x...), this->state_.value(x...), this->repeat_times_.value(x...)});
  }
};

}  // namespace esphome::silvercrest
