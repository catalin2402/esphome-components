#include "usb_microphone_switch.h"
#include "esphome/core/log.h"

namespace esphome::usb_media_player {

static const char *const TAG = "usb_media_player.switch";

void USBMicrophoneSwitch::setup() {
  this->parent_->add_on_microphone_callback([this](bool enabled) { this->publish_state(enabled); });
  this->write_state(this->get_initial_state_with_restore_mode().value_or(false));
}

void USBMicrophoneSwitch::dump_config() { LOG_SWITCH("", "USB Microphone", this); }

void USBMicrophoneSwitch::write_state(bool state) {
  this->parent_->set_microphone_enabled(state);
  if (!this->has_state() || this->state != this->parent_->is_microphone_enabled())
    this->publish_state(this->parent_->is_microphone_enabled());
}

}  // namespace esphome::usb_media_player
