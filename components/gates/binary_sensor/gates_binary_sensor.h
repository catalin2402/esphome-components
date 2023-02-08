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
  float get_setup_priority() const override;
  void setup() override;
  void dump_config() override;
  void update() override;
  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_parent(Gates *parent) { this->parent_ = parent; }

protected:
  Gates *parent_;
  uint8_t pin_{0};
  bool state_;
};

} // namespace gates
} // namespace esphome