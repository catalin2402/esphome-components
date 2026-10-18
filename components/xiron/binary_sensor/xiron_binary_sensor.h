#pragma once

#include "esphome/components/xiron/xiron.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome::xiron {

class XironBatterySensor : public XironBase, public binary_sensor::BinarySensor {
 public:
  bool on_receive(remote_base::RemoteReceiveData data) override;
  void dump_config() override;
};

}  // namespace esphome::xiron
