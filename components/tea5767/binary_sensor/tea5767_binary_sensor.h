#pragma once

#include "esphome/components/tea5767/tea5767.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/component.h"

namespace esphome::tea5767 {

enum class ReceptionType : uint8_t { MONO, STEREO };

class TEA5767BinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_parent(TEA5767 *parent) { this->parent_ = parent; }
  void set_type(ReceptionType type) { this->type_ = type; }

 protected:
  void publish_status_(bool stereo);

  TEA5767 *parent_{nullptr};
  ReceptionType type_{ReceptionType::MONO};
};

}  // namespace esphome::tea5767
