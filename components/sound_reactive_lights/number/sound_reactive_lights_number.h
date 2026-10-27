#pragma once

#include "esphome/core/defines.h"

#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#include "esphome/components/sound_reactive_lights/speaker/sound_reactive_lights_speaker.h"
#include "esphome/core/component.h"

namespace esphome::sound_reactive_lights {
enum ReactiveNumberType : uint8_t { NUMBER_SENSITIVITY, NUMBER_MAX_BRIGHTNESS };
class SoundReactiveLightsNumber : public number::Number, public Component {
 public:
  void set_parent(SoundReactiveLightsSpeaker *parent) { this->parent_ = parent; }
  void set_type(ReactiveNumberType type) { this->type_ = type; }
  void setup() override;
  void dump_config() override;

 protected:
  void control(float value) override;
  SoundReactiveLightsSpeaker *parent_{nullptr};
  ReactiveNumberType type_{NUMBER_SENSITIVITY};
};
}  // namespace esphome::sound_reactive_lights
#endif
