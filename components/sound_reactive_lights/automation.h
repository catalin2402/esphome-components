#pragma once

#include "esphome/components/sound_reactive_lights/speaker/sound_reactive_lights_speaker.h"
#include "esphome/core/automation.h"

namespace esphome::sound_reactive_lights {
class ModeTrigger : public Trigger<uint8_t> {
 public:
  explicit ModeTrigger(SoundReactiveLightsSpeaker *parent) {
    parent->add_mode_callback([this](uint8_t mode) { this->trigger(mode); });
  }
};
class EnabledTrigger : public Trigger<bool> {
 public:
  explicit EnabledTrigger(SoundReactiveLightsSpeaker *parent) {
    parent->add_enabled_callback([this](bool enabled) { this->trigger(enabled); });
  }
};
}  // namespace esphome::sound_reactive_lights
