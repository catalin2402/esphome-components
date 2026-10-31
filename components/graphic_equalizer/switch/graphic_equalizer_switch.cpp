#include "graphic_equalizer_switch.h"

#ifdef USE_SWITCH
#include "esphome/core/log.h"

namespace esphome::graphic_equalizer {
static const char *const TAG = "graphic_equalizer.switch";
void GraphicEqualizerSwitch::setup() {
  this->parent_->add_enabled_callback([this](bool enabled) { this->publish_state(enabled); });
  const auto initial = this->get_initial_state_with_restore_mode();
  if (initial.has_value())
    this->parent_->set_enabled(*initial);
  this->publish_state(this->parent_->is_enabled());
}
void GraphicEqualizerSwitch::write_state(bool state) {
  this->parent_->set_enabled(state);
  this->publish_state(this->parent_->is_enabled());
}
void GraphicEqualizerSwitch::dump_config() { LOG_SWITCH("", "Graphic Equalizer", this); }
}  // namespace esphome::graphic_equalizer
#endif
