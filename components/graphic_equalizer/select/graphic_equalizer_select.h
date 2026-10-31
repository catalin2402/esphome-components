#pragma once

#include "esphome/core/defines.h"
#ifdef USE_SELECT
#include "esphome/components/graphic_equalizer/speaker/graphic_equalizer_speaker.h"
#include "esphome/components/select/select.h"

namespace esphome::graphic_equalizer {
class GraphicEqualizerSelect : public select::Select, public Component {
 public:
  void set_parent(GraphicEqualizerSpeaker *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;

 protected:
  void control(size_t index) override;
  GraphicEqualizerSpeaker *parent_{nullptr};
};
}  // namespace esphome::graphic_equalizer
#endif
