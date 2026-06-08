#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace xiron_decoder {

class XironDecoder : public remote_base::RemoteReceiverListener, public Component {
 public:
  void set_temperature_sensor(sensor::Sensor *s) { temp_sensor_ = s; }
  void set_humidity_sensor(sensor::Sensor *s) { hum_sensor_ = s; }
  void set_battery_sensor(binary_sensor::BinarySensor *s) { battery_sensor_ = s; }
  void set_xiron_id(uint16_t id) { xiron_id_ = id; }

  bool on_receive(remote_base::RemoteReceiveData data) override;

 protected:
  sensor::Sensor *temp_sensor_{nullptr};
  sensor::Sensor *hum_sensor_{nullptr};
  binary_sensor::BinarySensor *battery_sensor_{nullptr};
  uint16_t xiron_id_{0};
};

}  // namespace xiron_decoder
}  // namespace esphome