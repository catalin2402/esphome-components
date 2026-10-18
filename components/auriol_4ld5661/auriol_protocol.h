#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/version.h"

namespace esphome::auriol_4ld5661 {

struct AuriolData {
  uint8_t id;
  uint8_t channel;  // Raw two-bit channel code (0-3).
  bool battery_ok;
  float temperature_c;
  uint32_t rain;  // Raw cumulative rain tips.

  float rain_mm(float mm_per_tip) const { return this->rain * mm_per_tip; }

  bool operator==(const AuriolData &rhs) const {
    return this->id == rhs.id && this->channel == rhs.channel && this->battery_ok == rhs.battery_ok &&
           this->temperature_c == rhs.temperature_c && this->rain == rhs.rain;
  }
};

class AuriolProtocol : public remote_base::RemoteProtocol<AuriolData> {
 public:
#if ESPHOME_VERSION_CODE < VERSION_CODE(2026, 10, 0)
  void encode(remote_base::RemoteTransmitData *dst, const AuriolData &data) override;
  optional<AuriolData> decode(remote_base::RemoteReceiveData src) override;
  void dump(const AuriolData &data) override;
#else
  void encode(remote_base::RemoteTransmitData *dst, const AuriolData &data);
  optional<AuriolData> decode(remote_base::RemoteReceiveData src);
  void dump(const AuriolData &data);
#endif

 protected:
  optional<AuriolData> decode_data_(const std::array<uint8_t, 7> &bytes) const;
};

using AuriolDumper = remote_base::RemoteReceiverDumper<AuriolProtocol>;

}  // namespace esphome::auriol_4ld5661
