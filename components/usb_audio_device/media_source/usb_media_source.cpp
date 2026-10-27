#include "usb_media_source.h"
#ifdef USE_USB_AUDIO_MEDIA_SOURCE
#include "esphome/core/log.h"
namespace esphome::usb_audio_device {
static const char *const TAG = "usb_audio_device.media_source";
void USBMediaSource::setup() {
  this->usb_->add_playback_activity_callback([this](bool active) { this->activity_(active); });
}
void USBMediaSource::loop() {
  // Closing the host USB interface is different from sending silent PCM.
  if (this->selected_.load() && !this->usb_->is_playback_active())
    this->stop_();
}
void USBMediaSource::dump_config() {
  ESP_LOGCONFIG(TAG, "USB media source: usb://current, automatic start on audible PC audio");
  ESP_LOGCONFIG(TAG, "  HID transport registered: %s", this->usb_->has_media_transport() ? "YES" : "NO");
}
bool USBMediaSource::can_handle(const std::string &uri) const { return uri == "usb://current"; }
bool USBMediaSource::play_uri(const std::string &uri) {
  this->pending_start_ = false;
  if (!this->can_handle(uri) || !this->has_listener() || !this->audible_)
    return false;
  this->selected_.store(true);
  this->set_state_(media_source::MediaSourceState::PLAYING);
  // USB host gain/mute is already applied to its PCM. Do not inherit Sendspin's
  // last player volume/mute when returning to PC playback.
  this->request_volume_(1.0f);
  this->request_mute_(false);
  return true;
}
void USBMediaSource::stop_() {
  this->selected_.store(false);
  this->pending_start_ = false;
  this->usb_->reset_playback_buffer();
  this->set_state_(media_source::MediaSourceState::IDLE);
}
void USBMediaSource::handle_command(media_source::MediaSourceCommand command) {
  switch (command) {
    case media_source::MediaSourceCommand::STOP:
      // Internal source switch/OTA lifecycle only: never send HID STOP.
      // Keep audible_ latched: a new Sendspin session must not immediately
      // lose priority to the still-running PC stream. Only a new activity edge
      // automatically requests USB again.
      this->stop_();
      break;
    case media_source::MediaSourceCommand::PAUSE:
      if (this->usb_->has_media_transport()) {
        ESP_LOGD(TAG, "USB pause request -> HID PLAY_PAUSE (0x00CD)");
        this->usb_->send_media_key(MEDIA_KEY_PLAY_PAUSE);
        // Keep PCM flowing, including silence, so pausing the PC does not
        // stop/restart the DAC. PAUSED reflects the requested transport state.
      } else {
        ESP_LOGW(TAG, "USB pause: no usb_media_control configured");
        this->selected_.store(false);
        this->usb_->reset_playback_buffer();
      }
      this->set_state_(media_source::MediaSourceState::PAUSED);
      break;
    case media_source::MediaSourceCommand::PLAY:
      if (this->usb_->has_media_transport()) {
        ESP_LOGD(TAG, "USB play request -> HID PLAY_PAUSE (0x00CD)");
        this->usb_->send_media_key(MEDIA_KEY_PLAY_PAUSE);
      } else {
        ESP_LOGW(TAG, "USB play: no usb_media_control configured");
      }
      if (this->audible_ || (this->usb_->has_media_transport() && this->usb_->is_playback_active())) {
        this->selected_.store(true);
        this->set_state_(media_source::MediaSourceState::PLAYING);
      }
      break;
    case media_source::MediaSourceCommand::NEXT:
      if (this->usb_->has_media_transport())
        this->usb_->send_media_key(MEDIA_KEY_NEXT);
      break;
    case media_source::MediaSourceCommand::PREVIOUS:
      if (this->usb_->has_media_transport())
        this->usb_->send_media_key(MEDIA_KEY_PREVIOUS);
      break;
    default:
      break;
  }
}
void USBMediaSource::activity_(bool active) {
  this->audible_ = active;
  if (active && !this->selected_.load() && !this->pending_start_) {
    this->pending_start_ = true;
    this->request_play_uri_("usb://current");
  }
  // Silence only rearms the next priority edge. Keep rendering silent PCM so
  // the selected source and physical DAC do not restart when sound resumes.
}
size_t USBMediaSource::write_pcm(const uint8_t *data, size_t bytes, uint32_t timeout_ms) {
  if (!this->selected_.load())
    return bytes;  // Discard any in-flight block when the orchestrator switches.
  const auto info = audio::AudioStreamInfo(16, 2, 48000);
  const size_t accepted = this->write_output(data, bytes, timeout_ms, info);
  return this->selected_.load() ? accepted : bytes;
}
}  // namespace esphome::usb_audio_device
#endif
