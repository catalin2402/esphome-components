
#pragma once

#include "../hub75_icn2053.h"
#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace hub75_icn2053 {

class HUB75_ICN2053Brightness : public number::Number, public PollingComponent {
public:
  HUB75_ICN2053Brightness() : PollingComponent(1000) {}
  void setup() override;
  void dump_config() override;
  void update() override;
  void set_parent(HUB75_ICN2053 *parent) { this->parent_ = parent; }

protected:
  void control(float value) override;
  HUB75_ICN2053 *parent_;
};

} // namespace hub75_icn2053
} // namespace esphome