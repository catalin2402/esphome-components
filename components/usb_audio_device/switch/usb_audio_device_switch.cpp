#include "usb_audio_device_switch.h"

#ifdef USE_SWITCH
#include "esphome/core/log.h"

namespace esphome::usb_audio_device {
static const char *const TAG = "usb_audio_device.switch";
void USBMicrophoneMuteSwitch::setup() {
  this->parent_->add_microphone_mute_callback([this](bool state) { this->publish_state(state); });
  auto initial = this->get_initial_state_with_restore_mode();
  if (initial.has_value())
    this->parent_->set_microphone_muted(*initial);
  this->publish_state(this->parent_->is_microphone_muted());
}
void USBMicrophoneMuteSwitch::write_state(bool state) {
  this->parent_->set_microphone_muted(state);
  this->publish_state(this->parent_->is_microphone_muted());
}
void USBMicrophoneMuteSwitch::dump_config() { LOG_SWITCH("", "Microphone Mute", this); }
}  // namespace esphome::usb_audio_device
#endif
