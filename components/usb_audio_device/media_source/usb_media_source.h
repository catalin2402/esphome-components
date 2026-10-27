#pragma once
#include "esphome/core/defines.h"
#ifdef USE_USB_AUDIO_MEDIA_SOURCE
#include "esphome/components/usb_audio_device/usb_audio_device.h"
#include "esphome/components/media_source/media_source.h"
namespace esphome::usb_audio_device {
class USBMediaSource : public Component, public media_source::MediaSource, public USBPlaybackSink {
 public:
  void set_usb_audio_device(USBAudioDevice *value) { this->usb_ = value; }
  void setup() override;
  void loop() override;
  void dump_config() override;
  bool can_handle(const std::string &uri) const override;
  bool play_uri(const std::string &uri) override;
  void handle_command(media_source::MediaSourceCommand command) override;
  bool has_internal_playlist() const override { return true; }
  void stop_playback() override { this->stop_(); }
  bool is_selected() const override { return this->selected_.load(); }
  size_t write_pcm(const uint8_t *data, size_t bytes, uint32_t timeout_ms) override;

 protected:
  void activity_(bool active);
  void stop_();
  USBAudioDevice *usb_{nullptr};
  std::atomic<bool> selected_{false};
  bool audible_{false}, pending_start_{false};
};
}  // namespace esphome::usb_audio_device
#endif
