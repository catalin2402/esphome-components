#include "graphic_equalizer_number.h"

#ifdef USE_NUMBER
#include "esphome/core/log.h"

namespace esphome::graphic_equalizer {
static const char *const TAG = "graphic_equalizer.number";
void GraphicEqualizerNumber::setup() {
  this->parent_->add_gain_callback([this](uint8_t control, float gain) {
    if (control == this->control_)
      this->publish_state(gain);
  });
  this->publish_state(this->current_value_());
}
float GraphicEqualizerNumber::current_value_() const {
  return this->control_ == BAND_COUNT ? this->parent_->get_preamp() : this->parent_->get_band_gain(this->control_);
}
void GraphicEqualizerNumber::control(float value) {
  if (this->control_ == BAND_COUNT)
    this->parent_->set_preamp(value);
  else
    this->parent_->set_band_gain(this->control_, value);
  if (!this->has_state() || this->state != this->current_value_())
    this->publish_state(this->current_value_());
}
void GraphicEqualizerNumber::dump_config() { LOG_NUMBER("", "Graphic Equalizer Gain", this); }
}  // namespace esphome::graphic_equalizer
#endif
