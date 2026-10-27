#pragma once

#include "esphome/core/defines.h"

#ifdef USE_SWITCH
#include "esphome/components/sound_reactive_lights/speaker/sound_reactive_lights_speaker.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome::sound_reactive_lights {
class SoundReactiveLightsSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(SoundReactiveLightsSpeaker *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;

 protected:
  void write_state(bool state) override;
  SoundReactiveLightsSpeaker *parent_{nullptr};
};
}  // namespace esphome::sound_reactive_lights
#endif
