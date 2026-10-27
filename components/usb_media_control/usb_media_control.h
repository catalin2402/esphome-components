#pragma once
#include "esphome/core/component.h"
#include "esphome/components/usb_audio_device/usb_audio_device.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace esphome::usb_media_control {
class USBMediaControl : public Component, public usb_audio_device::USBMediaTransport {
 public:
  void set_parent(usb_audio_device::USBAudioDevice *parent) { this->parent_ = parent; }
  void setup() override;
  void loop() override;
  void on_shutdown() override;
  void dump_config() override;
  bool is_ready() const;
  void send_media_key(uint16_t usage) override;
  void press_media_key(uint16_t usage);
  void release_media_key(uint16_t usage);
  void release_all_media_keys();

 protected:
  usb_audio_device::USBAudioDevice *parent_{nullptr};
  // Media key state is owned by the main loop and its automations.
  bool shutting_down_{false};
  QueueHandle_t media_keys_{nullptr};
  uint16_t media_current_{0}, media_waiting_{0};
  uint16_t media_held_{0};
  bool media_pulse_active_{false};
  uint32_t media_started_{0};
};
}  // namespace esphome::usb_media_control
