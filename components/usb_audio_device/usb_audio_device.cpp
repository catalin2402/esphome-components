#include "usb_audio_device.h"
#ifdef USE_USB_AUDIO_DEVICE_MICROPHONE
#include "esphome/components/microphone/microphone.h"
#endif
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esp_heap_caps.h"
#include "freertos/task.h"
#include "usb_device_uac.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cinttypes>

extern "C" void usb_audio_device_set_identity(uint16_t vendor, uint16_t product, const char *manufacturer,
                                              const char *name, const char *serial);

namespace esphome::usb_audio_device {
static const char *const TAG = "usb_audio_device";
static std::atomic<USBAudioDevice *> device{nullptr};
static constexpr size_t FRAMES = 480;
bool PCMQueue::allocate(size_t bytes) {
  this->data_ = static_cast<uint8_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!this->data_)
    this->data_ = static_cast<uint8_t *>(heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  this->capacity_ = this->data_ ? bytes : 0;
  return this->data_ != nullptr;
}
void PCMQueue::release() {
  // Only used before the audio worker is enabled and while USB callbacks are gated.
  portENTER_CRITICAL(&this->lock_);
  auto *data = this->data_;
  this->data_ = nullptr;
  this->capacity_ = this->head_ = this->count_ = 0;
  portEXIT_CRITICAL(&this->lock_);
  heap_caps_free(data);
}
void PCMQueue::reset() {
  portENTER_CRITICAL(&this->lock_);
  this->head_ = this->count_ = 0;
  portEXIT_CRITICAL(&this->lock_);
}
size_t PCMQueue::size() {
  portENTER_CRITICAL(&this->lock_);
  const size_t value = this->count_;
  portEXIT_CRITICAL(&this->lock_);
  return value;
}
size_t PCMQueue::push(const uint8_t *data, size_t bytes, size_t frame_bytes) {
  if (!frame_bytes || bytes % frame_bytes || !this->data_)
    return 0;
  portENTER_CRITICAL(&this->lock_);
  const size_t length = std::min(bytes, this->capacity_ - this->count_) / frame_bytes * frame_bytes;
  const size_t tail = (this->head_ + this->count_) % this->capacity_;
  const size_t first = std::min(length, this->capacity_ - tail);
  std::memcpy(this->data_ + tail, data, first);
  std::memcpy(this->data_, data + first, length - first);
  this->count_ += length;
  portEXIT_CRITICAL(&this->lock_);
  return length;
}
bool PCMQueue::pop(uint8_t *data, size_t bytes) {
  if (!this->data_ || !bytes)
    return false;
  portENTER_CRITICAL(&this->lock_);
  if (this->count_ < bytes) {
    portEXIT_CRITICAL(&this->lock_);
    return false;
  }
  const size_t first = std::min(bytes, this->capacity_ - this->head_);
  std::memcpy(data, this->data_ + this->head_, first);
  std::memcpy(data + first, this->data_, bytes - first);
  this->head_ = (this->head_ + bytes) % this->capacity_;
  this->count_ -= bytes;
  portEXIT_CRITICAL(&this->lock_);
  return true;
}
// Endpoint-preserving interpolation, with a bit-exact path for equal counts.
static void resample(const int16_t *input, size_t in_frames, int16_t *output, size_t channels) {
  if (in_frames == FRAMES) {
    std::memcpy(output, input, FRAMES * channels * 2);
    return;
  }
  for (size_t frame = 0; frame < FRAMES; ++frame) {
    const uint32_t position = static_cast<uint32_t>(frame * (in_frames - 1) * 65536ULL / (FRAMES - 1));
    const size_t index = position >> 16;
    for (size_t channel = 0; channel < channels; ++channel) {
      const int32_t a = input[index * channels + channel];
      const int32_t b = input[std::min(index + 1, in_frames - 1) * channels + channel];
      output[frame * channels + channel] = static_cast<int16_t>(a + (int64_t(b - a) * (position & 65535)) / 65536);
    }
  }
}
void USBAudioDevice::setup() {
#ifdef USE_OTA_STATE_LISTENER
  ota::get_global_ota_callback()->add_global_state_listener(this);
#endif
  if (device.load() != nullptr) {
    this->fail_setup_("Only one USB audio device is supported");
    return;
  }
  if ((!this->speaker_ && !this->playback_sink_) || (this->speaker_ && this->playback_sink_)) {
    this->fail_setup_("Exactly one USB playback destination is required");
    return;
  }
#ifdef USE_USB_AUDIO_DEVICE_MICROPHONE
  if (this->microphone_ == nullptr) {
    this->fail_setup_("A capture microphone is required");
    return;
  }
#endif
  if (!this->playback_.allocate(65536)) {
    this->fail_setup_("Could not allocate the playback queue");
    return;
  }
#ifdef USE_USB_AUDIO_DEVICE_MICROPHONE
  if (!this->capture_.allocate(16384)) {
    this->fail_setup_("Could not allocate the capture queue");
    return;
  }
#endif
  // The worker waits for ready_ before touching either queues or audio endpoints.
  if (xTaskCreatePinnedToCore(playback_task_, "usb_pcm", 6144, this, 10, &this->playback_task_handle_, 1) != pdPASS) {
    this->fail_setup_("Could not create the playback task");
    return;
  }
  device.store(this);
  uac_device_config_t config{};
  config.cb_ctx = this;
  config.output_cb = [](uint8_t *data, size_t bytes, void *context) -> esp_err_t {
    static_cast<USBAudioDevice *>(context)->receive_playback(data, bytes);
    return ESP_OK;
  };
#ifdef USE_USB_AUDIO_DEVICE_MICROPHONE
  config.input_cb = [](uint8_t *data, size_t bytes, size_t *read, void *context) -> esp_err_t {
    static_cast<USBAudioDevice *>(context)->read_capture(data, bytes, read);
    return ESP_OK;
  };
#endif
  config.set_volume_cb = [](uint32_t value, void *context) {
    static_cast<USBAudioDevice *>(context)->set_host_volume(value);
  };
  config.set_mute_cb = [](uint32_t value, void *context) {
    static_cast<USBAudioDevice *>(context)->set_host_mute(value != 0);
  };
  usb_audio_device_set_identity(this->vendor_id_, this->product_id_, this->manufacturer_, this->product_,
                                this->serial_);
  const esp_err_t result = uac_device_init(&config);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "USB init failed: %s", esp_err_to_name(result));
    this->fail_setup_("Could not initialize the USB audio device");
    return;
  }
  if (this->speaker_)
    this->speaker_->set_audio_stream_info(audio::AudioStreamInfo(16, 2, 48000));
#ifdef USE_USB_AUDIO_DEVICE_MICROPHONE
  this->microphone_->add_data_callback([this](const std::vector<uint8_t> &data) { this->receive_microphone_(data); });
#endif
  this->ready_.store(true);
}
void USBAudioDevice::fail_setup_(const char *message) {
  ESP_LOGE(TAG, "%s", message);
  // ready_ has never been set, so the worker cannot be inside a speaker call
  // or queue critical section. Any partially started USB callbacks remain gated.
  if (this->playback_task_handle_ != nullptr) {
    vTaskDelete(this->playback_task_handle_);
    this->playback_task_handle_ = nullptr;
  }
  this->playback_.release();
  this->capture_.release();
  if (device.load() == this)
    device.store(nullptr);
  this->mark_failed();
}

void USBAudioDevice::set_playback_active(bool value) {
  this->playback_signal_seen_.store(false);
  this->playback_active_.store(false);
  this->playback_generation_.fetch_add(1);
  this->playback_.reset();
  this->playback_active_.store(value);
}
void USBAudioDevice::set_capture_active(bool value) {
  value = value && this->has_microphone();
  this->capture_active_.store(false);
  this->capture_generation_.fetch_add(1);
  this->capture_.reset();
  this->capture_active_.store(value);
}
void USBAudioDevice::set_microphone_muted(bool value) {
  if (!this->has_microphone())
    return;
  if (this->mic_muted_.exchange(value) == value)
    return;
  this->capture_generation_.fetch_add(1);
  this->capture_.reset();
  this->microphone_mute_callback_.call(value);
}
void USBAudioDevice::set_host_volume(uint32_t percent) {
  percent = std::min<uint32_t>(percent, 100);
  const float amplitude = percent ? std::pow(10.0f, (percent * 0.5f - 50.0f) / 20.0f) : 0.0f;
  this->gain_q16_.store(static_cast<int32_t>(std::lround(amplitude * 65536)));
}
void USBAudioDevice::receive_playback(const uint8_t *data, size_t bytes) {
  if (!this->ready_.load() || this->shutting_down_.load() || this->audio_suspended_.load() ||
      !this->playback_active_.load())
    return;
  if (this->monitor_playback_.load() && !this->host_muted_.load()) {
    const int32_t gain = this->gain_q16_.load();
    for (size_t offset = 0; offset + sizeof(int16_t) <= bytes; offset += sizeof(int16_t)) {
      int16_t sample;
      std::memcpy(&sample, data + offset, sizeof(sample));
      const int64_t magnitude = std::abs(static_cast<int32_t>(sample)) * int64_t(gain);
      if (magnitude >= int64_t(this->activity_threshold_) * 65536) {
        this->last_playback_signal_.store(millis());
        this->playback_signal_seen_.store(true);
        break;
      }
    }
  }
  const size_t accepted = this->playback_.push(data, bytes, 4);
  this->dropped_playback_.fetch_add((bytes - accepted) / 4);
}
#ifdef USE_USB_AUDIO_DEVICE_MICROPHONE
void USBAudioDevice::receive_microphone_(const std::vector<uint8_t> &data) {
  if (!this->ready_.load() || this->shutting_down_.load() || this->audio_suspended_.load() ||
      !this->capture_active_.load() || this->mic_muted_.load())
    return;
  const auto info = this->microphone_->get_audio_stream_info();
  if (info.get_bits_per_sample() != 16 || info.get_channels() != 1 || info.get_sample_rate() != 48000)
    return;
  const size_t accepted = this->capture_.push(data.data(), data.size(), 2);
  this->dropped_capture_.fetch_add((data.size() - accepted) / 2);
}
#endif
void USBAudioDevice::read_capture(uint8_t *data, size_t bytes, size_t *read) {
  *read = bytes;
  std::memset(data, 0, bytes);
  if (!this->ready_.load() || this->shutting_down_.load() || this->audio_suspended_.load() || bytes != FRAMES * 2)
    return;
  const uint32_t generation = this->capture_generation_.load();
  if (generation != this->capture_generation_seen_) {
    this->capture_generation_seen_ = generation;
    this->capture_primed_ = false;
  }
  if (!this->ready_.load() || this->shutting_down_.load() || this->audio_suspended_.load() ||
      !this->capture_active_.load() || this->mic_muted_.load())
    return;
  const size_t queued = this->capture_.size() / 2;
  if (!this->capture_primed_ && queued >= 960)
    this->capture_primed_ = true;
  if (!this->capture_primed_)
    return;
  const size_t consume = queued > 1440 ? 481 : queued < 480 ? 479 : 480;
  int16_t input[481], output[480];
  if (!this->capture_.pop(reinterpret_cast<uint8_t *>(input), consume * 2)) {
    this->capture_primed_ = false;
    return;
  }
  resample(input, consume, output, 1);
  if (this->audio_suspended_.load() || this->shutting_down_.load() || this->mic_muted_.load() ||
      this->capture_generation_.load() != generation)
    return;
  std::memcpy(data, output, bytes);
}
void USBAudioDevice::playback_task_(void *arg) {
  auto *self = static_cast<USBAudioDevice *>(arg);
  int16_t input[482 * 2], output[FRAMES * 2];
  uint32_t generation = UINT32_MAX;
  bool started = false, primed = false;
  bool waiting_for_packet = false;
  uint32_t packet_wait_started = 0;
  while (!self->shutting_down_.load()) {
    if (!self->ready_.load()) {
      vTaskDelay(std::max<TickType_t>(1, pdMS_TO_TICKS(10)));
      continue;
    }
    const uint32_t current = self->playback_generation_.load();
    if (current != generation || !self->playback_active_.load()) {
      if (started && self->speaker_)
        self->speaker_->stop();
      started = primed = false;
      waiting_for_packet = false;
      generation = current;
    }
    if (self->audio_suspended_.load()) {
      vTaskDelay(std::max<TickType_t>(1, pdMS_TO_TICKS(10)));
      continue;
    }
    const size_t queued = self->playback_.size() / 4;
    if (self->playback_sink_ && !self->playback_sink_->is_selected()) {
      // Keep at most 30 ms of live pre-roll. A continuous suppressed PC stream
      // must neither fill the queue nor play stale audio after a source switch.
      if (queued > 1440)
        self->playback_.pop(reinterpret_cast<uint8_t *>(input), FRAMES * 4);
      primed = false;
      waiting_for_packet = false;
      vTaskDelay(std::max<TickType_t>(1, pdMS_TO_TICKS(1)));
      continue;
    }
    if (!self->playback_active_.load() || (!primed && queued < 1440)) {
      vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }
    if (!started) {
      if (self->speaker_) {
        self->speaker_->set_audio_stream_info(audio::AudioStreamInfo(16, 2, 48000));
        self->speaker_->start();
      }
      started = true;
    }
    primed = true;
    // The speaker has its own buffer. Draining our queue into that buffer is
    // normal, so wait for a complete block instead of re-priming every 30 ms.
    // A low queue alone also cannot establish hardware clock drift.
    const size_t consume = queued > 1920 ? 481 : 480;
    if (!self->playback_.pop(reinterpret_cast<uint8_t *>(input), consume * 4)) {
      const uint32_t now = millis();
      if (!waiting_for_packet) {
        waiting_for_packet = true;
        packet_wait_started = now;
      } else if (now - packet_wait_started >= 20) {
        self->playback_underruns_.fetch_add(1);
        primed = false;
        waiting_for_packet = false;
      }
      vTaskDelay(std::max<TickType_t>(1, pdMS_TO_TICKS(1)));
      continue;
    }
    waiting_for_packet = false;
    resample(input, consume, output, 2);
    const int32_t gain = self->host_muted_.load() ? 0 : self->gain_q16_.load();
    for (auto &value : output)
      value = static_cast<int16_t>(int64_t(value) * gain / 65536);
    size_t offset = 0;
    while (offset < sizeof(output) && !self->shutting_down_.load() && !self->audio_suspended_.load() &&
           self->playback_active_.load() && generation == self->playback_generation_.load()) {
      const size_t written = self->playback_sink_
                                 ? self->playback_sink_->write_pcm(reinterpret_cast<uint8_t *>(output) + offset,
                                                                   sizeof(output) - offset, 10)
                                 : self->speaker_->play(reinterpret_cast<uint8_t *>(output) + offset,
                                                        sizeof(output) - offset, pdMS_TO_TICKS(10));
      offset += std::min(written, sizeof(output) - offset);
      if (!written)
        vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
  if (started && self->speaker_)
    self->speaker_->stop();
  vTaskDelete(nullptr);
}
void USBAudioDevice::update_microphone_() {
#ifdef USE_USB_AUDIO_DEVICE_MICROPHONE
  const bool wanted = this->ready_.load() && !this->shutting_down_.load() && !this->audio_suspended_.load() &&
                      this->capture_active_.load();
  if (wanted && !this->microphone_started_) {
    this->microphone_->start();
    this->microphone_started_ = true;
  }
  if (!wanted && this->microphone_started_) {
    this->microphone_->stop();
    this->microphone_started_ = false;
  }
#endif
}
void USBAudioDevice::stop_microphone_() {
#ifdef USE_USB_AUDIO_DEVICE_MICROPHONE
  if (this->microphone_started_) {
    this->microphone_->stop();
    this->microphone_started_ = false;
  }
#endif
}
void USBAudioDevice::loop() {
  // USB tasks only update atomics. Automations run on the main loop.
  const bool connected = this->is_connected();
  if (connected != this->connection_state_) {
    this->connection_state_ = connected;
    this->connection_callback_.call(connected);
  }
  const bool playback = this->is_playback_active();
  if (playback != this->playback_state_) {
    this->playback_state_ = playback;
    this->playback_callback_.call(playback);
  }
  const bool capture = this->is_capture_active();
  if (capture != this->capture_state_) {
    this->capture_state_ = capture;
    this->capture_callback_.call(capture);
  }
  if (this->monitor_playback_.load()) {
    const bool audible = !this->audio_suspended_.load() && this->playback_active_.load() && !this->host_muted_.load() &&
                         this->playback_signal_seen_.load() &&
                         millis() - this->last_playback_signal_.load() < this->activity_timeout_;
    if (audible != this->playback_audible_) {
      this->playback_audible_ = audible;
      this->playback_activity_callback_.call(audible);
    }
  }
  this->update_microphone_();
  if (millis() - this->last_diagnostics_ >= 5000) {
    this->last_diagnostics_ = millis();
    const auto output = this->dropped_playback_.exchange(0), input = this->dropped_capture_.exchange(0);
    const auto underruns = this->playback_underruns_.exchange(0);
    if (output || input || underruns)
      ESP_LOGW(TAG, "Audio queues: dropped playback %lu / capture %lu frames; playback underruns %lu",
               (unsigned long) output, (unsigned long) input, (unsigned long) underruns);
  }
}
#ifdef USE_OTA_STATE_LISTENER
void USBAudioDevice::on_ota_global_state(ota::OTAState state, float progress, uint8_t error,
                                         ota::OTAComponent *component) {
  if (state != ota::OTA_STARTED && state != ota::OTA_ERROR && state != ota::OTA_ABORT)
    return;
  // Gate worker/USB callbacks immediately. Endpoint and source lifecycle calls
  // belong on the main loop, even when the OTA notification comes from a task.
  this->audio_suspended_.store(state == ota::OTA_STARTED);
  this->reset_playback_buffer();
  this->capture_generation_.fetch_add(1);
  this->capture_.reset();
  this->playback_signal_seen_.store(false);
  this->defer("usb_audio_ota", [this]() { this->apply_ota_state_(); });
}
void USBAudioDevice::apply_ota_state_() {
  if (!this->ready_.load() || this->shutting_down_.load())
    return;
  if (this->audio_suspended_.load()) {
    if (this->playback_sink_)
      this->playback_sink_->stop_playback();
    if (this->speaker_)
      this->speaker_->stop();
    this->stop_microphone_();
    if (this->playback_audible_) {
      this->playback_audible_ = false;
      this->playback_activity_callback_.call(false);
    }
    ESP_LOGI(TAG, "USB audio paused for OTA");
  } else {
    // capture_active_ still reflects the host's interface, so loop() restarts
    // capture if requested. Playback can accept fresh audio without reconnecting.
    ESP_LOGI(TAG, "USB audio enabled after OTA failure/abort");
  }
}
#endif
void USBAudioDevice::on_shutdown() {
  this->shutting_down_.store(true);
  this->ready_.store(false);
  this->audio_suspended_.store(true);
  this->set_playback_active(false);
  this->set_capture_active(false);
  if (this->playback_sink_)
    this->playback_sink_->stop_playback();
  this->stop_microphone_();
}

void USBAudioDevice::dump_config() {
  ESP_LOGCONFIG(TAG, "USB Audio Device:");
  ESP_LOGCONFIG(TAG, "  Manufacturer: %s", this->manufacturer_);
  ESP_LOGCONFIG(TAG, "  Product: %s", this->product_);
  ESP_LOGCONFIG(TAG, "  Serial: %s", this->serial_);
  ESP_LOGCONFIG(TAG, "  USB ID: %04X:%04X", this->vendor_id_, this->product_id_);
  ESP_LOGCONFIG(TAG, "  Playback: 48000 Hz, 16-bit, stereo");
  ESP_LOGCONFIG(TAG, "  Microphone capture: %s", this->has_microphone() ? "48000 Hz, 16-bit, mono" : "Disabled");
  ESP_LOGCONFIG(TAG, "  Playback destination: %s", this->speaker_ ? "Speaker" : "Media source");
  ESP_LOGCONFIG(TAG, "  Activity threshold: %u", this->activity_threshold_);
  ESP_LOGCONFIG(TAG, "  Activity timeout: %" PRIu32 " ms", this->activity_timeout_);
}
}  // namespace esphome::usb_audio_device

extern "C" void usb_audio_device_playback_changed(bool active) {
  if (auto *device = esphome::usb_audio_device::device.load())
    device->set_playback_active(active);
}
extern "C" void usb_audio_device_capture_changed(bool active) {
  if (auto *device = esphome::usb_audio_device::device.load())
    device->set_capture_active(active);
}

extern "C" void usb_audio_device_connection_changed(bool connected) {
  if (auto *device = esphome::usb_audio_device::device.load())
    device->set_connected(connected);
}
