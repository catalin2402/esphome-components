#include "sound_reactive_lights_select.h"
#ifdef USE_SOUND_REACTIVE_MODE_SELECT
namespace esphome::sound_reactive_lights {
void SoundReactiveLightsSelect::setup() {
  this->parent_->add_mode_callback([this](uint8_t mode) { this->publish_state(static_cast<size_t>(mode)); });
  this->publish_state(static_cast<size_t>(this->parent_->get_mode()));
}
void SoundReactiveLightsSelect::control(size_t index) {
  this->parent_->set_mode(static_cast<uint8_t>(index));
  this->parent_->set_enabled(true);
}
}  // namespace esphome::sound_reactive_lights
#endif
