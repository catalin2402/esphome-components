#pragma once

#include "esphome/components/auriol_4ld5661/auriol_4ld5661.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome::auriol_4ld5661 {

class AuriolBatterySensor : public Auriol4LD5661Base, public binary_sensor::BinarySensor {
 public:
  bool on_receive(remote_base::RemoteReceiveData data) override;
  void dump_config() override;
};

}  // namespace esphome::auriol_4ld5661
