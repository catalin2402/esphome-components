#include "usb_media_player.h"

#include <algorithm>
#include <cstring>
#include "esp_heap_caps.h"
#include "esphome/core/log.h"
#include "usb_device_uac.h"
#include "usb_media_hid.h"
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
#include "media_source/usb_media_source.h"
#endif

namespace esphome::usb_media_player {

static const char *const TAG = "usb_media_player";

void USBMediaPlayer::set_source(USBMediaSource *source) {
  this->source_ = source;
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
  source->add_on_stream_callback([this](bool streaming) { this->stream_callback_.call(streaming); });
#endif
}

bool USBMediaPlayer::is_streaming() const {
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
  return this->source_ != nullptr && this->source_->is_streaming();
#else
  return false;
#endif
}

void USBMediaPlayer::setup() {
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
  if (this->source_ != nullptr && this->source_->is_failed()) {
    ESP_LOGE(TAG, "USB playback source failed to initialize");
    this->mark_failed();
    return;
  }
#endif
#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
  this->microphone_buffer_ =
      static_cast<uint8_t *>(heap_caps_malloc(this->microphone_buffer_size_, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (this->microphone_buffer_ == nullptr) {
    ESP_LOGE(TAG, "Unable to allocate USB microphone buffer");
    this->mark_failed();
    return;
  }
  this->microphone_->add_data_callback([this](const std::vector<uint8_t> &data) { this->microphone_data_(data); });
#endif
  uac_device_config_t config{};
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
  config.output_cb = USBMediaPlayer::output_cb_;
  config.set_mute_cb = USBMediaPlayer::mute_cb_;
  config.set_volume_cb = USBMediaPlayer::volume_cb_;
#endif
#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
  config.input_cb = USBMediaPlayer::input_cb_;
#endif
  config.cb_ctx = this;
  const esp_err_t result = uac_device_init(&config);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "USB Audio initialization failed: %s", esp_err_to_name(result));
    this->mark_failed();
    return;
  }
  this->initialized_ = true;
#ifdef USE_OTA_STATE_LISTENER
  ota::get_global_ota_callback()->add_global_state_listener(this);
#endif
}

void USBMediaPlayer::loop() {
  const bool connected = usb_audio_hid_mounted();
  if (connected != this->connected_) {
    this->connected_ = connected;
    if (!connected)
      this->clear_media_keys_();
#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
    this->update_microphone_();
#endif
    this->connection_callback_.call(connected);
  }
  if (!this->suspended_.load())
    this->process_media_keys_();
}

void USBMediaPlayer::dump_config() {
  ESP_LOGCONFIG(TAG,
                "USB Media Player:\n"
                "  Sample rate: 48000 Hz\n"
                "  USB playback: %s",
                YESNO(this->source_ != nullptr));
#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
  ESP_LOGCONFIG(TAG,
                "  USB microphone: mono, signed 16-bit PCM\n"
                "  Microphone buffer: %u bytes\n"
                "  Microphone enabled: %s",
                static_cast<unsigned>(this->microphone_buffer_size_), YESNO(this->microphone_enabled_));
#else
  ESP_LOGCONFIG(TAG, "  USB microphone: not configured");
#endif
}

void USBMediaPlayer::set_microphone_enabled(bool enabled) {
#ifndef USE_USB_MEDIA_PLAYER_MICROPHONE
  if (enabled) {
    ESP_LOGW(TAG, "No USB microphone configured");
    return;
  }
#endif
  if (this->microphone_enabled_ == enabled)
    return;
  this->microphone_enabled_ = enabled;
#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
  this->update_microphone_();
#endif
  this->microphone_callback_.call(enabled);
}

#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
void USBMediaPlayer::update_microphone_() {
  const bool active = this->initialized_ && this->connected_ && this->microphone_enabled_ && !this->suspended_.load();
  portENTER_CRITICAL(&this->microphone_lock_);
  const bool changed = active != this->capture_active_;
  this->capture_active_ = active;
  if (changed) {
    this->microphone_head_ = 0;
    this->microphone_count_ = 0;
  }
  portEXIT_CRITICAL(&this->microphone_lock_);
  if (changed) {
    if (active)
      this->microphone_->start();
    else
      this->microphone_->stop();
  }
}

void USBMediaPlayer::microphone_data_(const std::vector<uint8_t> &data) {
  // MicrophoneSource supplies the selected channel as signed 16-bit PCM.
  if (data.empty() || data.size() % 2 != 0)
    return;
  size_t length = std::min(data.size(), this->microphone_buffer_size_);
  const uint8_t *input = data.data() + data.size() - length;
  portENTER_CRITICAL(&this->microphone_lock_);
  if (this->capture_active_ && !this->suspended_.load()) {
    // Keep recent samples when the PC is not reading fast enough.
    const size_t discard = length > this->microphone_buffer_size_ - this->microphone_count_
                               ? length - (this->microphone_buffer_size_ - this->microphone_count_)
                               : 0;
    this->microphone_head_ = (this->microphone_head_ + discard) % this->microphone_buffer_size_;
    this->microphone_count_ -= discard;
    const size_t tail = (this->microphone_head_ + this->microphone_count_) % this->microphone_buffer_size_;
    const size_t first = std::min(length, this->microphone_buffer_size_ - tail);
    if (first != 0)
      std::memcpy(this->microphone_buffer_ + tail, input, first);
    if (length != first)
      std::memcpy(this->microphone_buffer_, input + first, length - first);
    this->microphone_count_ += length;
  }
  portEXIT_CRITICAL(&this->microphone_lock_);
}

void USBMediaPlayer::read_microphone_(uint8_t *data, size_t length) {
  std::memset(data, 0, length);
  portENTER_CRITICAL(&this->microphone_lock_);
  if (this->capture_active_ && !this->suspended_.load()) {
    const size_t available = std::min(length, this->microphone_count_);
    const size_t first = std::min(available, this->microphone_buffer_size_ - this->microphone_head_);
    if (first != 0)
      std::memcpy(data, this->microphone_buffer_ + this->microphone_head_, first);
    if (available != first)
      std::memcpy(data + first, this->microphone_buffer_, available - first);
    this->microphone_head_ = (this->microphone_head_ + available) % this->microphone_buffer_size_;
    this->microphone_count_ -= available;
  }
  portEXIT_CRITICAL(&this->microphone_lock_);
}

esp_err_t USBMediaPlayer::input_cb_(uint8_t *data, size_t length, size_t *bytes_read, void *context) {
  if (data == nullptr || bytes_read == nullptr || length % 2 != 0)
    return ESP_ERR_INVALID_ARG;
  static_cast<USBMediaPlayer *>(context)->read_microphone_(data, length);
  *bytes_read = length;
  return ESP_OK;
}
#endif

esp_err_t USBMediaPlayer::output_cb_(uint8_t *data, size_t length, void *context) {
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
  auto *hub = static_cast<USBMediaPlayer *>(context);
  if (hub->source_ != nullptr)
    return hub->source_->receive_audio(data, length);
#endif
  return ESP_OK;
}

void USBMediaPlayer::mute_cb_(uint32_t mute, void *context) {
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
  auto *hub = static_cast<USBMediaPlayer *>(context);
  if (hub->source_ != nullptr)
    hub->source_->set_host_mute(mute != 0);
#endif
}

void USBMediaPlayer::volume_cb_(uint32_t volume, void *context) {
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
  auto *hub = static_cast<USBMediaPlayer *>(context);
  if (hub->source_ != nullptr)
    hub->source_->set_host_volume(volume);
#endif
}

void USBMediaPlayer::on_shutdown() {
  this->suspended_.store(true);
#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
  this->update_microphone_();
#endif
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
  if (this->source_ != nullptr)
    this->source_->on_shutdown();
#endif
}

#ifdef USE_OTA_STATE_LISTENER
void USBMediaPlayer::on_ota_global_state(ota::OTAState state, float progress, uint8_t error,
                                         ota::OTAComponent *component) {
  if (state != ota::OTA_STARTED && state != ota::OTA_ERROR && state != ota::OTA_ABORT)
    return;
  const bool suspended = state == ota::OTA_STARTED;
  this->suspended_.store(suspended);
#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
  if (suspended) {
    portENTER_CRITICAL(&this->microphone_lock_);
    this->microphone_head_ = 0;
    this->microphone_count_ = 0;
    portEXIT_CRITICAL(&this->microphone_lock_);
  }
#endif
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
  if (this->source_ != nullptr)
    this->source_->set_suspended(suspended);
#endif
#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
  this->defer([this]() { this->update_microphone_(); });
#endif
}
#endif

// HID state is owned by the ESPHome loop; USB callbacks only handle audio controls.
void USBMediaPlayer::send_media_key(MediaKey key) {
  if (!this->connected_)
    return;
  if (this->key_count_ == MEDIA_KEY_QUEUE_SIZE) {
    ESP_LOGW(TAG, "Media-key queue is full");
    return;
  }
  this->media_keys_[this->key_tail_] = key;
  this->key_tail_ = (this->key_tail_ + 1) % MEDIA_KEY_QUEUE_SIZE;
  this->key_count_++;
}

void USBMediaPlayer::press_media_key(MediaKey key) {
  if (this->connected_)
    this->held_key_ = static_cast<uint16_t>(key);
}

void USBMediaPlayer::release_media_key(MediaKey key) {
  if (this->held_key_ == static_cast<uint16_t>(key))
    this->held_key_ = 0;
}

void USBMediaPlayer::clear_media_keys_() {
  this->key_head_ = this->key_tail_ = this->key_count_ = 0;
  this->active_key_ = this->held_key_ = 0;
}

void USBMediaPlayer::process_media_keys_() {
  if (!this->connected_ || !usb_audio_hid_ready())
    return;
  if (this->active_key_ != 0) {
    if (this->active_key_ != this->held_key_ && usb_audio_hid_send_report(0))
      this->active_key_ = 0;
    return;
  }
  if (this->held_key_ != 0) {
    if (usb_audio_hid_send_report(this->held_key_))
      this->active_key_ = this->held_key_;
    return;
  }
  if (this->key_count_ != 0) {
    const auto usage = static_cast<uint16_t>(this->media_keys_[this->key_head_]);
    if (usb_audio_hid_send_report(usage)) {
      this->active_key_ = usage;
      this->key_head_ = (this->key_head_ + 1) % MEDIA_KEY_QUEUE_SIZE;
      this->key_count_--;
    }
  }
}

}  // namespace esphome::usb_media_player
