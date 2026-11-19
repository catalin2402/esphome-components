#pragma once

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/defines.h"

namespace esphome::silvercrest {

enum Channel : uint8_t { A, B, C, D, MASTER };

struct SilvercrestData {
  Channel channel{A};
  bool state{};
  uint8_t repeat_times{5};
  bool operator==(const SilvercrestData &rhs) const { return this->channel == rhs.channel && this->state == rhs.state; }
};

class SilvercrestProtocol final : public remote_base::RemoteProtocol<SilvercrestData> {
 public:
  void encode(remote_base::RemoteTransmitData *dst, const SilvercrestData &data);
  optional<SilvercrestData> decode(remote_base::RemoteReceiveData src);
  void dump(const SilvercrestData &data);
};

#ifdef USE_BINARY_SENSOR
using SilvercrestBinarySensor = remote_base::RemoteReceiverBinarySensor<SilvercrestProtocol>;
#endif
using SilvercrestDumper = remote_base::RemoteReceiverDumper<SilvercrestProtocol>;

}  // namespace esphome::silvercrest

// Shared remote action codegen resolves protocol types in remote_base.
namespace esphome::remote_base {
using SilvercrestProtocol = silvercrest::SilvercrestProtocol;
}  // namespace esphome::remote_base
