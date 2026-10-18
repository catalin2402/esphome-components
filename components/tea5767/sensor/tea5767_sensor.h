#pragma once

#include "esphome/components/tea5767/tea5767.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome::tea5767 {

enum class SensorType : uint8_t { FREQUENCY, SIGNAL_LEVEL };

class TEA5767Sensor : public sensor::Sensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_parent(TEA5767 *parent) { this->parent_ = parent; }
  void set_type(SensorType type) { this->type_ = type; }

 protected:
  void publish_status_(uint64_t frequency, uint8_t level);

  TEA5767 *parent_{nullptr};
  SensorType type_{SensorType::FREQUENCY};
};

}  // namespace esphome::tea5767
