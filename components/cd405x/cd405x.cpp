#include "cd405x.h"

#include "esphome/core/log.h"

namespace esphome::cd405x {

static const char *const TAG = "cd405x";

void CD405x::setup() {
  // Disconnect the signal path while initializing the address pins, when possible.
  if (this->pin_inhibit_ != nullptr) {
    this->pin_inhibit_->setup();
    this->pin_inhibit_->digital_write(true);
  }
  this->pin_a_->setup();
  this->pin_b_->setup();
  if (this->pin_c_ != nullptr)
    this->pin_c_->setup();
  this->initialized_ = true;
  this->write_state_();
}

void CD405x::dump_config() {
  ESP_LOGCONFIG(TAG, "CD405x Multiplexer:");
  LOG_PIN("  A Pin: ", this->pin_a_);
  LOG_PIN("  B Pin: ", this->pin_b_);
  LOG_PIN("  C Pin: ", this->pin_c_);
  LOG_PIN("  Inhibit Pin: ", this->pin_inhibit_);
  ESP_LOGCONFIG(TAG, "  Channels: %u", this->get_channel_count());
  ESP_LOGCONFIG(TAG, "  Selected channel: %u", this->channel_);
  ESP_LOGCONFIG(TAG, "  Inhibited: %s", YESNO(this->inhibited_));
}

void CD405x::select_channel(int channel) {
  if (channel < 0 || channel >= this->get_channel_count()) {
    ESP_LOGW(TAG, "Invalid channel: %d (expected 0-%u)", channel, this->get_channel_count() - 1);
    return;
  }
  const bool changed = this->channel_ != channel;
  this->channel_ = channel;
  this->write_state_();
  if (changed)
    this->channel_callback_.call(channel);
}

void CD405x::inhibit(bool inhibit) {
  if (this->pin_inhibit_ == nullptr) {
    if (inhibit)
      ESP_LOGW(TAG, "Cannot inhibit without pin_inhibit");
    return;
  }
  const bool changed = this->inhibited_ != inhibit;
  this->inhibited_ = inhibit;
  if (this->initialized_)
    this->pin_inhibit_->digital_write(inhibit);
  if (changed)
    this->inhibit_callback_.call(inhibit);
}

void CD405x::write_state_() {
  if (!this->initialized_)
    return;
  // Avoid selecting intermediate channels while changing multiple address bits.
  if (this->pin_inhibit_ != nullptr)
    this->pin_inhibit_->digital_write(true);
  this->pin_a_->digital_write((this->channel_ & 0x01) != 0);
  this->pin_b_->digital_write((this->channel_ & 0x02) != 0);
  if (this->pin_c_ != nullptr)
    this->pin_c_->digital_write((this->channel_ & 0x04) != 0);
  if (this->pin_inhibit_ != nullptr)
    this->pin_inhibit_->digital_write(this->inhibited_);
}

}  // namespace esphome::cd405x
