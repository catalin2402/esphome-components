#include "status_light_light.h"

namespace esphome::status_light {

void StatusLightLightOutput::write_state(light::LightState *state) {
  if (this->initialized_ && this->get_status_() == Status::OK)
    this->write_idle_();
}

void StatusLightLightOutput::write_idle_() {
  if (this->control_state_ == nullptr)
    return;
  auto call = this->light_->make_call();
  call.from_light_color_values(this->control_state_->current_values);
  call.set_transition_length_if_supported(0);
  call.set_effect(uint32_t{0});
  call.set_publish(false);
  call.set_save(false);
  call.perform();
}

}  // namespace esphome::status_light
