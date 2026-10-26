#pragma once

#include "esphome/components/usb_media_player/usb_media_player.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome::usb_media_player {

class USBMicrophoneSwitch : public switch_::Switch, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_parent(USBMediaPlayer *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  USBMediaPlayer *parent_{nullptr};
};

}  // namespace esphome::usb_media_player
