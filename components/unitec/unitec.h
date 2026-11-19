#pragma once

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/defines.h"

namespace esphome::unitec {

struct UnitecData {
  uint16_t address{};
  uint8_t channel{};
  bool state{};
  uint32_t code{};
  bool operator==(const UnitecData &rhs) const {
    return this->address == rhs.address && this->channel == rhs.channel && this->state == rhs.state;
  }
};

class UnitecProtocol final : public remote_base::RemoteProtocol<UnitecData> {
 public:
  void encode(remote_base::RemoteTransmitData *dst, const UnitecData &data);
  optional<UnitecData> decode(remote_base::RemoteReceiveData src);
  void dump(const UnitecData &data);
};

#ifdef USE_BINARY_SENSOR
using UnitecBinarySensor = remote_base::RemoteReceiverBinarySensor<UnitecProtocol>;
#endif
using UnitecDumper = remote_base::RemoteReceiverDumper<UnitecProtocol>;

}  // namespace esphome::unitec

// Shared remote action codegen resolves protocol types in remote_base.
namespace esphome::remote_base {
using UnitecProtocol = unitec::UnitecProtocol;
}  // namespace esphome::remote_base
