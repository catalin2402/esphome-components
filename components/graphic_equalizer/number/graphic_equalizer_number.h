#pragma once

#include "esphome/core/defines.h"
#ifdef USE_NUMBER
#include "esphome/components/graphic_equalizer/speaker/graphic_equalizer_speaker.h"
#include "esphome/components/number/number.h"

namespace esphome::graphic_equalizer {
class GraphicEqualizerNumber : public number::Number, public Component {
 public:
  void set_parent(GraphicEqualizerSpeaker *parent) { this->parent_ = parent; }
  void set_control(uint8_t control) { this->control_ = control; }
  void setup() override;
  void dump_config() override;

 protected:
  void control(float value) override;
  float current_value_() const;
  GraphicEqualizerSpeaker *parent_{nullptr};
  uint8_t control_{0};
};
}  // namespace esphome::graphic_equalizer
#endif
