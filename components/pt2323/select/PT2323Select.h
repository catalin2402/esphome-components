#pragma once

#include "../PT2323.h"
#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

namespace esphome {
namespace pt2323 {

class PT2323Select : public select::Select, public Component {
public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override {
    return esphome::setup_priority::AFTER_WIFI;
  }

  void set_parent(PT2323 *parent) { this->parent_ = parent; }

protected:
  void control(const std::string &value) override;

  PT2323 *parent_;
};

} // namespace pt2323
} // namespace esphome