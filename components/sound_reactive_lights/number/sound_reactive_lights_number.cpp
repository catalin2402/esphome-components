#include "sound_reactive_lights_number.h"

#ifdef USE_NUMBER
#include "esphome/core/log.h"

namespace esphome::sound_reactive_lights {
static const char *const TAG = "sound_reactive_lights.number";
void SoundReactiveLightsNumber::setup() {
  if (this->type_ == NUMBER_SENSITIVITY) {
    this->parent_->add_sensitivity_callback([this](float value) { this->publish_state(value); });
    this->publish_state(this->parent_->get_sensitivity());
  } else {
    this->parent_->add_max_brightness_callback([this](float value) { this->publish_state(value * 100.0f); });
    this->publish_state(this->parent_->get_max_brightness() * 100.0f);
  }
}
void SoundReactiveLightsNumber::control(float value) {
  if (this->type_ == NUMBER_SENSITIVITY) {
    this->parent_->set_sensitivity(value);
    if (!this->has_state() || this->state != this->parent_->get_sensitivity())
      this->publish_state(this->parent_->get_sensitivity());
  } else {
    this->parent_->set_max_brightness(value / 100.0f);
    if (!this->has_state() || this->state != this->parent_->get_max_brightness() * 100.0f)
      this->publish_state(this->parent_->get_max_brightness() * 100.0f);
  }
}
void SoundReactiveLightsNumber::dump_config() { LOG_NUMBER("", "Sound Reactive Lights", this); }
}  // namespace esphome::sound_reactive_lights
#endif
