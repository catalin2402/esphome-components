#pragma once

#include "../arduino_expander.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace arduino_expander {

class ArduinoSensor : public sensor::Sensor, public PollingComponent {
public:
  void dump_config() override;
  void setup() override;
  void update() override;
  void set_parent(ArduinoExpander *parent) { this->parent_ = parent; }
  void set_pin(uint8_t pin) { this->pin_ = pin;}

protected:
  ArduinoExpander *parent_;
  uint16_t state_;
  uint8_t pin_;
};

} // namespace arduino_expander
} // namespace esphome