#include "usb_media_source.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstring>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esp_heap_caps.h"

namespace esphome::usb_media_player {

static const char *const TAG = "usb_media_player.source";
using media_source::MediaSourceCommand;
using media_source::MediaSourceState;

void USBMediaSource::setup() {
  this->output_gain_q16_ =
      static_cast<uint32_t>(std::lround(std::pow(10.0f, this->output_gain_db_ / 20.0f) * 65536.0f));
  // USB callbacks run in task context; packet storage does not need DMA memory.
  if (this->use_psram_) {
    this->buffer_ =
        xRingbufferCreateWithCaps(this->buffer_size_, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    this->buffer_in_psram_ = this->buffer_ != nullptr;
  }
  if (this->buffer_ == nullptr)
    this->buffer_ =
        xRingbufferCreateWithCaps(this->buffer_size_, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "Unable to allocate USB audio buffer");
    this->mark_failed();
    return;
  }
  this->prebuffer_enabled_.store(this->auto_start_);
  this->task_running_.store(true);
  if (xTaskCreate(USBMediaSource::audio_task_, "usb_audio_out", 6144, this, 5, &this->task_) != pdPASS) {
    this->task_running_.store(false);
    vRingbufferDeleteWithCaps(this->buffer_);
    this->buffer_ = nullptr;
    ESP_LOGE(TAG, "Unable to create USB audio output task");
    this->mark_failed();
    return;
  }
}

void USBMediaSource::dump_config() {
  ESP_LOGCONFIG(TAG,
                "USB Media Source:\n"
                "  Audio: 48000 Hz, stereo, signed 16-bit PCM\n"
                "  Buffer: %u bytes (%s)\n"
                "  Prefill: %u ms\n"
                "  Auto start: %s\n"
                "  Maximum output gain: %.1f dB",
                static_cast<unsigned>(this->buffer_size_), this->buffer_in_psram_ ? "PSRAM" : "internal RAM",
                static_cast<unsigned>(this->prefill_ms_), YESNO(this->auto_start_), this->output_gain_db_);
}

void USBMediaSource::loop() {
  const uint32_t now = millis();
  const bool connected = this->parent_->is_connected();
  if (connected != this->connected_) {
    this->connected_ = connected;
    if (connected) {
      this->auto_start_blocked_ = false;
      if (this->auto_start_ && !this->suspended_.load()) {
        this->prebuffer_enabled_.store(true);
        this->request_play_uri_("usb://");
      }
    } else {
      this->stop_audio_();
      this->set_state_(MediaSourceState::IDLE);
    }
  }
  this->update_stream_state_();
  if (now - this->last_drop_log_ >= 5000) {
    this->last_drop_log_ = now;
    const uint32_t dropped = this->dropped_frames_.exchange(0);
    if (dropped != 0)
      ESP_LOGW(TAG, "USB audio output fell behind; dropped %" PRIu32 " frames", dropped);
  }
}

bool USBMediaSource::play_uri(const std::string &uri) {
  if (!this->can_handle(uri) || this->is_failed() || !this->connected_ || this->suspended_.load())
    return false;
  this->start_audio_();
  return true;
}

void USBMediaSource::start_audio_() {
  this->auto_start_blocked_ = false;
  if (!this->prebuffer_enabled_.load())
    this->generation_.fetch_add(1);
  this->playback_enabled_.store(true);
  this->prebuffer_enabled_.store(false);
  this->set_state_(MediaSourceState::PLAYING);
  this->update_stream_state_();
}

void USBMediaSource::update_stream_state_() {
  const bool streaming = this->connected_ && !this->suspended_.load() && this->get_state() == MediaSourceState::PLAYING;
  if (this->streaming_ != streaming) {
    this->streaming_ = streaming;
    this->stream_callback_.call(streaming);
  }
}

void USBMediaSource::stop_audio_() {
  this->prebuffer_enabled_.store(false);
  this->playback_enabled_.store(false);
  this->generation_.fetch_add(1);
}

void USBMediaSource::handle_command(MediaSourceCommand command) {
  switch (command) {
    case MediaSourceCommand::PLAY:
      if (!this->connected_ || this->suspended_.load())
        return;
      this->send_media_key(MediaKey::PLAY);
      if (this->get_state() != MediaSourceState::PLAYING)
        this->start_audio_();
      break;
    case MediaSourceCommand::PAUSE:
      // Pause controls the PC. The USB headset route remains ready for all
      // audio, including notifications and audio from other applications.
      this->send_media_key(MediaKey::PAUSE);
      break;
    case MediaSourceCommand::STOP:
      // The orchestrator also uses STOP when selecting another source. Do not
      // let a connected headset immediately take the pipeline back.
      this->auto_start_blocked_ = true;
      this->send_media_key(MediaKey::STOP);
      this->stop_audio_();
      this->set_state_(MediaSourceState::IDLE);
      this->update_stream_state_();
      break;
    case MediaSourceCommand::NEXT:
      this->send_media_key(MediaKey::NEXT_TRACK);
      break;
    case MediaSourceCommand::PREVIOUS:
      this->send_media_key(MediaKey::PREVIOUS_TRACK);
      break;
    default:
      break;
  }
}

esp_err_t USBMediaSource::receive_audio(const uint8_t *data, size_t length) {
  if (length % 4 != 0 || (data == nullptr && length != 0))
    return ESP_ERR_INVALID_SIZE;
  if (length == 0)
    return ESP_OK;
  if ((!this->playback_enabled_.load() && !this->prebuffer_enabled_.load()) || this->suspended_.load())
    return ESP_OK;

  const uint32_t generation = this->generation_.load();
  const uint32_t gain = this->host_muted_.load() ? 0 : this->host_gain_q16_.load();
  while (length != 0) {
    const size_t bytes = std::min(length, PACKET_BYTES);
    void *item = nullptr;
    // The USB callback never waits for a busy speaker or allocates memory.
    if (xRingbufferSendAcquire(this->buffer_, &item, sizeof(PacketHeader) + bytes, 0) != pdTRUE) {
      this->dropped_frames_.fetch_add(length / 4);
      return ESP_OK;
    }
    auto *header = static_cast<PacketHeader *>(item);
    header->generation = generation;
    header->gain_q16 = gain;
    std::memcpy(header + 1, data, bytes);
    this->queued_bytes_.fetch_add(bytes);
    xRingbufferSendComplete(this->buffer_, item);
    data += bytes;
    length -= bytes;
  }
  return ESP_OK;
}

void USBMediaSource::audio_task_(void *context) {
  static_cast<USBMediaSource *>(context)->write_audio_();
  vTaskDelete(nullptr);
}

void USBMediaSource::write_audio_() {
  const audio::AudioStreamInfo stream_info(16, 2, 48000);
  int16_t pcm[PACKET_BYTES / sizeof(int16_t)];
  const TickType_t wait = std::max<TickType_t>(1, pdMS_TO_TICKS(20));
  bool needs_prefill = true;
  uint32_t generation = this->generation_.load();
  // Retain a small reserve before feeding the speaker after startup or a gap.
  const size_t prefill_bytes = 192 * this->prefill_ms_;
  while (this->task_running_.load()) {
    size_t size = 0;
    auto *item = static_cast<PacketHeader *>(xRingbufferReceive(this->buffer_, &size, wait));
    if (item == nullptr) {
      needs_prefill = true;
      continue;
    }
    const size_t length = size - sizeof(PacketHeader);
    if (generation != item->generation) {
      generation = item->generation;
      needs_prefill = true;
    }
    if (needs_prefill && item->generation == this->generation_.load()) {
      const uint32_t start = millis();
      while (this->queued_bytes_.load() < prefill_bytes && millis() - start < this->prefill_ms_ &&
             this->task_running_.load() && item->generation == this->generation_.load() &&
             !this->suspended_.load())
        vTaskDelay(1);
      needs_prefill = false;
    }
    this->queued_bytes_.fetch_sub(length);
    // Retain the first USB packets while the main loop selects this source.
    while (this->task_running_.load() && item->generation == this->generation_.load() &&
           this->prebuffer_enabled_.load() && !this->playback_enabled_.load() && !this->suspended_.load())
      vTaskDelay(1);
    if (item->generation == this->generation_.load() && this->playback_enabled_.load() && !this->suspended_.load()) {
      const auto *input = reinterpret_cast<const int16_t *>(item + 1);
      const uint32_t gain =
          static_cast<uint32_t>((static_cast<uint64_t>(item->gain_q16) * this->output_gain_q16_) >> 16);
      for (size_t i = 0; i < length / sizeof(int16_t); i++)
        pcm[i] = static_cast<int16_t>((static_cast<int64_t>(input[i]) * gain) >> 16);
      size_t offset = 0;
      uint32_t last_progress = millis();
      while (offset < length && this->task_running_.load() && this->playback_enabled_.load() &&
             !this->suspended_.load() && item->generation == this->generation_.load()) {
        const size_t written =
            this->write_output(reinterpret_cast<const uint8_t *>(pcm) + offset, length - offset, 20, stream_info);
        if (written != 0) {
          offset += written;
          last_progress = millis();
        } else {
          if (millis() - last_progress >= 200)
            break;
          vTaskDelay(1);
        }
      }
      if (offset < length && item->generation == this->generation_.load() && this->playback_enabled_.load())
        this->dropped_frames_.fetch_add((length - offset) / 4);
    }
    vRingbufferReturnItem(this->buffer_, item);
  }
}

void USBMediaSource::set_host_volume(uint32_t volume) {
  // usb_device_uac maps its advertised -50..0 dB range to 0..100.
  const uint32_t percent = std::min<uint32_t>(volume, 100);
  const float gain = percent == 0 ? 0.0f : std::pow(10.0f, (percent * 0.5f - 50.0f) / 20.0f);
  this->host_gain_q16_.store(static_cast<uint32_t>(std::lround(gain * 65536.0f)));
}

void USBMediaSource::on_shutdown() {
  this->suspended_.store(true);
  this->stop_audio_();
  this->task_running_.store(false);
}

void USBMediaSource::set_suspended(bool suspended) {
  if (suspended) {
    if (this->suspended_.exchange(true))
      return;
    this->stop_audio_();
    this->defer([this]() {
      this->resume_after_ota_ = this->get_state() == MediaSourceState::PLAYING;
      if (this->resume_after_ota_)
        this->set_state_(MediaSourceState::PAUSED);
      this->update_stream_state_();
    });
  } else {
    this->defer([this]() {
      this->suspended_.store(false);
      if (this->resume_after_ota_) {
        this->resume_after_ota_ = false;
        if (this->connected_)
          this->start_audio_();
      } else if (this->connected_ && this->get_state() == MediaSourceState::IDLE && !this->auto_start_blocked_) {
        if (this->auto_start_) {
          this->prebuffer_enabled_.store(true);
          this->request_play_uri_("usb://");
        }
      }
    });
  }
}

}  // namespace esphome::usb_media_player
