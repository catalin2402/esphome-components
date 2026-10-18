#pragma once

#include <cstdint>

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/version.h"

namespace esphome::xiron {

struct XironData {
  uint8_t id;
  uint8_t channel;
  bool battery_ok;
  bool status_flag;
  float temperature_c;
  uint8_t humidity;

  uint16_t device_id() const { return (static_cast<uint16_t>(this->id) << 8) | this->channel; }
  bool operator==(const XironData &rhs) const {
    return this->id == rhs.id && this->channel == rhs.channel && this->battery_ok == rhs.battery_ok &&
           this->status_flag == rhs.status_flag && this->temperature_c == rhs.temperature_c &&
           this->humidity == rhs.humidity;
  }
};

class XironProtocol : public remote_base::RemoteProtocol<XironData> {
 public:
#if ESPHOME_VERSION_CODE < VERSION_CODE(2026, 10, 0)
  void encode(remote_base::RemoteTransmitData *dst, const XironData &data) override;
  optional<XironData> decode(remote_base::RemoteReceiveData src) override;
  void dump(const XironData &data) override;
#else
  void encode(remote_base::RemoteTransmitData *dst, const XironData &data);
  optional<XironData> decode(remote_base::RemoteReceiveData src);
  void dump(const XironData &data);
#endif

 protected:
  optional<XironData> decode_frame_(remote_base::RemoteReceiveData src) const;
};

using XironDumper = remote_base::RemoteReceiverDumper<XironProtocol>;

}  // namespace esphome::xiron
