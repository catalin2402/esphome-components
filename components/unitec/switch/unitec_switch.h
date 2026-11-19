#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/components/unitec/unitec.h"
#include "esphome/core/component.h"

namespace esphome::unitec {

class UnitecSwitch final : public switch_::Switch, public Component, public remote_base::RemoteTransmittable {
 public:
  void dump_config() override;
  void set_address(uint16_t address) { this->address_ = address; }
  void set_channel(uint8_t channel) { this->channel_ = channel; }

 protected:
  void write_state(bool state) override;
  bool assumed_state() override { return true; }

  uint16_t address_{};
  uint8_t channel_{};
};

}  // namespace esphome::unitec
