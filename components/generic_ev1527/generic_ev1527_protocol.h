#pragma once

#include <cstdint>

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/defines.h"

namespace esphome::generic_ev1527 {

struct GenericEV1527Data {
  uint32_t id{};
  uint8_t data{};

  uint32_t get_code() const { return (this->id << 4) | this->data; }
  bool operator==(const GenericEV1527Data &rhs) const { return this->id == rhs.id && this->data == rhs.data; }
};

class GenericEV1527Protocol final : public remote_base::RemoteProtocol<GenericEV1527Data> {
 public:
  void encode(remote_base::RemoteTransmitData *dst, const GenericEV1527Data &data);
  optional<GenericEV1527Data> decode(remote_base::RemoteReceiveData src);
  void dump(const GenericEV1527Data &data);
};

#ifdef USE_BINARY_SENSOR
using GenericEV1527BinarySensor = remote_base::RemoteReceiverBinarySensor<GenericEV1527Protocol>;
#endif
using GenericEV1527Dumper = remote_base::RemoteReceiverDumper<GenericEV1527Protocol>;

}  // namespace esphome::generic_ev1527
