#pragma once

#include "esphome/components/pt2323/pt2323.h"
#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

namespace esphome::pt2323 {

class PT2323Select : public select::Select, public Component {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;
  void update();
  void set_parent(PT2323 *parent) { this->parent_ = parent; }

 protected:
  void control(size_t index) override;
  PT2323 *parent_{nullptr};
};

}  // namespace esphome::pt2323
