#pragma once

#include "esphome/core/defines.h"
#ifdef USE_SWITCH
#include "esphome/components/graphic_equalizer/speaker/graphic_equalizer_speaker.h"
#include "esphome/components/switch/switch.h"

namespace esphome::graphic_equalizer {
class GraphicEqualizerSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(GraphicEqualizerSpeaker *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;

 protected:
  void write_state(bool state) override;
  GraphicEqualizerSpeaker *parent_{nullptr};
};
}  // namespace esphome::graphic_equalizer
#endif
