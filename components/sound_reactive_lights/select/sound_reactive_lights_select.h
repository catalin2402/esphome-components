#pragma once
#include "esphome/core/defines.h"
#ifdef USE_SOUND_REACTIVE_MODE_SELECT
#include "esphome/components/sound_reactive_lights/speaker/sound_reactive_lights_speaker.h"
#include "esphome/components/select/select.h"
namespace esphome::sound_reactive_lights {
class SoundReactiveLightsSelect : public select::Select, public Component {
 public:
  void set_parent(SoundReactiveLightsSpeaker *parent) { this->parent_ = parent; }
  void setup() override;

 protected:
  void control(size_t index) override;
  SoundReactiveLightsSpeaker *parent_{nullptr};
};
}  // namespace esphome::sound_reactive_lights
#endif
