#pragma once

#include "esphome/core/component.h"
#include "../PT2323.h"
#include "esphome/components/select/select.h"

namespace esphome {
namespace pt2323 {

class PT2323Select : public select::Select, public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_parent(PT2323 *parent) { this->parent_ = parent; }
  void set_select_mappings(std::vector<uint8_t> mappings) { this->mappings_ = std::move(mappings); }

 protected:
  void control(const std::string &value) override;

  PT2323 *parent_;
  std::vector<uint8_t> mappings_;
};

}  // namespace pt2323
}  // namespace esphome