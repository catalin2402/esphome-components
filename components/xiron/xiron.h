#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "xiron_protocol.h"

namespace esphome::xiron {

class XironBase : public remote_base::RemoteReceiverListener, public Component {
 public:
  void set_xiron_id(uint16_t id) {
    this->xiron_id_ = id;
    this->has_xiron_id_ = true;
  }
  void dump_config() override;

 protected:
  optional<XironData> decode_packet_(remote_base::RemoteReceiveData data);
  uint16_t xiron_id_{0};
  bool has_xiron_id_{false};
};

}  // namespace esphome::xiron
