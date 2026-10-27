#pragma once

#include "esphome/core/defines.h"

#ifdef USE_SWITCH
#include "esphome/components/usb_audio_device/usb_audio_device.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome::usb_audio_device {
class USBMicrophoneMuteSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(USBAudioDevice *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;

 protected:
  void write_state(bool state) override;
  USBAudioDevice *parent_{nullptr};
};
}  // namespace esphome::usb_audio_device
#endif
