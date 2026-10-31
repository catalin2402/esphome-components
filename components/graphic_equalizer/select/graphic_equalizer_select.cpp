#include "graphic_equalizer_select.h"

#ifdef USE_SELECT
#include "esphome/core/log.h"

namespace esphome::graphic_equalizer {
static const char *const TAG = "graphic_equalizer.select";
void GraphicEqualizerSelect::setup() {
  this->parent_->add_preset_callback([this](uint8_t preset) { this->publish_state(static_cast<size_t>(preset)); });
  this->publish_state(static_cast<size_t>(this->parent_->get_preset()));
}
void GraphicEqualizerSelect::control(size_t index) {
  if (index < PRESET_COUNT)
    this->parent_->select_preset(static_cast<uint8_t>(index));
}
void GraphicEqualizerSelect::dump_config() { LOG_SELECT("", "Graphic Equalizer Preset", this); }
}  // namespace esphome::graphic_equalizer
#endif
