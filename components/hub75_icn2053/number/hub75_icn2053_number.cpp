#include "hub75_icn2053_number.h"

#include <cmath>
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::hub75_icn2053 {

static const char *const TAG = "hub75_icn2053.number";

void HUB75BrightnessNumber::setup() {
  this->parent_->add_on_brightness_callback([this](int brightness) { this->publish_state(brightness); });
  this->publish_state(this->parent_->get_brightness());
}

void HUB75BrightnessNumber::dump_config() { LOG_NUMBER("", "HUB75 Brightness", this); }

void HUB75BrightnessNumber::control(float value) {
  if (!std::isfinite(value))
    return;
  this->parent_->set_brightness(static_cast<int>(std::lround(clamp(value, 0.0f, 255.0f))));
  if (this->state != this->parent_->get_brightness())
    this->publish_state(this->parent_->get_brightness());
}

}  // namespace esphome::hub75_icn2053
