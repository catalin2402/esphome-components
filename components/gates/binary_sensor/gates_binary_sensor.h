#pragma once

#include "../gates.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace gates {

class GatesBinarySensor : public binary_sensor::BinarySensor,
                          public PollingComponent {
public:
  GatesBinarySensor() : PollingComponent(100) {}
  void setup() override;
  void update() override;
  void set_parent(Gates *parent) { this->parent_ = parent; }
  void set_type(uint8_t type) { this->type_ = type; }

protected:
  Gates *parent_;
  bool state_;
  uint8_t type_;
};

} // namespace gates
} // namespace esphome