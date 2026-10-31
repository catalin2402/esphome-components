#pragma once

#include "esphome/components/graphic_equalizer/speaker/graphic_equalizer_speaker.h"
#include "esphome/core/automation.h"

namespace esphome::graphic_equalizer {
class GainTrigger : public Trigger<uint8_t, float> {
 public:
  explicit GainTrigger(GraphicEqualizerSpeaker *parent) {
    parent->add_gain_callback([this](uint8_t band, float gain) { this->trigger(band, gain); });
  }
};
class PresetTrigger : public Trigger<uint8_t> {
 public:
  explicit PresetTrigger(GraphicEqualizerSpeaker *parent) {
    parent->add_preset_callback([this](uint8_t preset) { this->trigger(preset); });
  }
};
class EnabledTrigger : public Trigger<bool> {
 public:
  explicit EnabledTrigger(GraphicEqualizerSpeaker *parent) {
    parent->add_enabled_callback([this](bool enabled) { this->trigger(enabled); });
  }
};
}  // namespace esphome::graphic_equalizer
