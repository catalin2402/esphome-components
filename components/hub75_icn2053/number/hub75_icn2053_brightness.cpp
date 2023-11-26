
#include "hub75_icn2053_brightness.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hub75_icn2053 {

static const char *const TAG = "hub75_icn2053.number";

void HUB75_ICN2053Brightness::dump_config() {
  LOG_NUMBER("", "HUB75 ICN2053 Display Brightness", this);
}

void HUB75_ICN2053Brightness::setup() {
  this->state = this->parent_->dma_display_->brightness;
  this->publish_state(this->state);
}

void HUB75_ICN2053Brightness::update() {
  float new_state = this->parent_->dma_display_->brightness;
  if (new_state != this->state) {
    this->state = new_state;
    this->publish_state(this->state);
  }
}

void HUB75_ICN2053Brightness::control(float value) {
  this->state = value;
  this->parent_->dma_display_->setPanelBrightness(value);
  this->publish_state(this->state);
}

} // namespace hub75_icn2053
} // namespace esphome