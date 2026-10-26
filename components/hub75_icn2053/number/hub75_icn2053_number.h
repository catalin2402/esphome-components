#pragma once

#include "esphome/components/hub75_icn2053/hub75_icn2053.h"
#include "esphome/components/number/number.h"
#include "esphome/core/component.h"

namespace esphome::hub75_icn2053 {

class HUB75BrightnessNumber : public number::Number, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_parent(HUB75Display *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;
  HUB75Display *parent_{nullptr};
};

}  // namespace esphome::hub75_icn2053
