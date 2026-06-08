#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace rf_remote {

class RFRemoteBase : public remote_base::RemoteReceiverListener, public Component {
 public:
  void set_remote_id(uint32_t id) { remote_id_ = id; }
  void set_reset_time(uint32_t reset_time) { reset_time_ = reset_time; }

 protected:
  uint32_t remote_id_{0};
  uint32_t reset_time_{200};

  bool decode_packet_(remote_base::RemoteReceiveData data, uint32_t *remote_id, uint8_t *command, uint32_t *code);
};

class RFRemoteSensor : public RFRemoteBase, public sensor::Sensor {
 public:
  bool on_receive(remote_base::RemoteReceiveData data) override;
};

class RFRemoteBinarySensor : public RFRemoteBase, public binary_sensor::BinarySensor {
 public:
  void set_remote_button(uint8_t button) { remote_button_ = button; }
  bool on_receive(remote_base::RemoteReceiveData data) override;

 protected:
  uint8_t remote_button_{0};
};

}  // namespace rf_remote
}  // namespace esphome