#include "sound_reactive_lights_switch.h"

#ifdef USE_SWITCH
#include "esphome/core/log.h"

namespace esphome::sound_reactive_lights {
static const char *const TAG = "sound_reactive_lights.switch";
void SoundReactiveLightsSwitch::setup() {
  this->parent_->add_enabled_callback([this](bool state) { this->publish_state(state); });
  auto initial = this->get_initial_state_with_restore_mode();
  if (initial.has_value())
    this->parent_->set_enabled(*initial);
  this->publish_state(this->parent_->is_enabled());
}
void SoundReactiveLightsSwitch::write_state(bool state) {
  this->parent_->set_enabled(state);
  this->publish_state(this->parent_->is_enabled());
}
void SoundReactiveLightsSwitch::dump_config() { LOG_SWITCH("", "Sound Reactive Lights", this); }
}  // namespace esphome::sound_reactive_lights
#endif
