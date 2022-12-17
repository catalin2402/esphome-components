#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "../PT2258.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace pt2258 {

class PT2258Number : public number::Number, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_number_type(uint8_t number_type) { this->number_type_ = number_type; }
  void set_parent(PT2258 *parent) { this->parent_ = parent; }

 protected:
  void control(float value) override;

  PT2258 *parent_;
  uint8_t number_type_{0};
  int state_;
};

}  // namespace pt2258
}  // namespace esphome