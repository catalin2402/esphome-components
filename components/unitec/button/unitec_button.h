#pragma once

#include "esphome/components/button/button.h"
#include "esphome/components/unitec/unitec.h"
#include "esphome/core/component.h"

namespace esphome::unitec {

class UnitecButton : public button::Button, public Component, public remote_base::RemoteTransmittable {
 public:
  void dump_config() override;
  void set_address(uint16_t address) { this->address_ = address; }
  void set_channel(uint8_t channel) { this->channel_ = channel; }
  void set_state(bool state) { this->state_ = state; }

 protected:
  void press_action() override;

  uint16_t address_{};
  uint8_t channel_{};
  bool state_{};
};

}  // namespace esphome::unitec
