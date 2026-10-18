#pragma once

#include "esphome/components/status_light/status_light.h"
#include "esphome/components/light/light_output.h"

namespace esphome::status_light {

class StatusLightLightOutput : public light::LightOutput, public StatusLight {
 public:
  light::LightTraits get_traits() override { return this->light_->get_traits(); }
  void setup_state(light::LightState *state) override { this->control_state_ = state; }
  void write_state(light::LightState *state) override;

 protected:
  void write_idle_() override;

  light::LightState *control_state_{nullptr};
};

}  // namespace esphome::status_light
