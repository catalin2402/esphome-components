#pragma once

#include "esphome/core/defines.h"

#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#include "esphome/components/usb_media_control/usb_media_control.h"

namespace esphome::usb_media_control {
class USBMediaControlButton : public button::Button {
 public:
  void set_parent(USBMediaControl *parent) { this->parent_ = parent; }
  void set_key(uint16_t key) { this->key_ = key; }

 protected:
  void press_action() override { this->parent_->send_media_key(this->key_); }
  USBMediaControl *parent_{nullptr};
  uint16_t key_{0};
};
}  // namespace esphome::usb_media_control
#endif
