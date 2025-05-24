#pragma once

#include "../pt2323.h"
#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

namespace esphome {
namespace pt2323 {

class PT2323Select : public select::Select, public PollingComponent {
public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;
  void update() override;
  void set_parent(PT2323 *parent) { this->parent_ = parent; }

protected:
  void control(const std::string &value) override;
  PT2323 *parent_;
};

} // namespace pt2323
} // namespace esphome
