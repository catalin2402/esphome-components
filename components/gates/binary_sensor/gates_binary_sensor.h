#pragma once

#include "../gates.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace gates {

enum SensorType { DUAL_GATE, SINGLE_GATE };

class GatesBinarySensor : public binary_sensor::BinarySensor,
                          public PollingComponent {
public:
  GatesBinarySensor() : PollingComponent(100) {}
  void setup() override;
  void update() override;
  void set_parent(Gates *parent) { this->parent_ = parent; }
  void set_type(SensorType type) { this->type_ = type; }

protected:
  bool get_new_state_();
  Gates *parent_;
  SensorType type_{DUAL_GATE};
};

} // namespace gates
} // namespace esphome