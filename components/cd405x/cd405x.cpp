
#include "cd405x.h"

namespace esphome {
namespace cd405x {

static const char *const TAG = "cd405x";

void CD405x::setup() {
  ESP_LOGCONFIG(TAG, "Setting up CD405x...");

  this->pin_a_->setup();
  this->pin_a_->digital_write(false);

  this->pin_b_->setup();
  this->pin_b_->digital_write(false);

  if (this->pin_c_ != nullptr) {
    this->pin_c_->setup();
    this->pin_c_->digital_write(false);
  }

  if (this->pin_inhibit_ != nullptr) {
    this->pin_inhibit_->setup();
    this->pin_inhibit_->digital_write(false);
  }
}

void CD405x::dump_config() {
  ESP_LOGCONFIG(TAG, "CD405x Multiplexer:");
  LOG_PIN("  A Pin: ", this->pin_a_);
  LOG_PIN("  B Pin: ", this->pin_b_);
  if (this->pin_c_ != nullptr)
    LOG_PIN("  C Pin: ", this->pin_c_);
  if (this->pin_inhibit_ != nullptr)
    LOG_PIN("  Inhibit Pin: ", this->pin_inhibit_);
}

void CD405x::activate_channel(uint8_t channel) {
  ESP_LOGD(TAG, "CD405x activating channel: %d", channel);
  this->pin_a_->digital_write(channel % 2);
  this->pin_b_->digital_write(channel / 2 % 2);

  if (this->pin_c_ != nullptr)
    this->pin_c_->digital_write(channel / 4 % 2);
}

void CD405x::inhibit(bool inhibit) {
  if (this->pin_inhibit_ != nullptr) {
    ESP_LOGD(TAG, "CD405x setting inhibit: %s", TRUEFALSE(inhibit));
    this->pin_inhibit_->digital_write(inhibit);
  }
}
} // namespace cd405x
} // namespace esphome