#include "tinyusb_audio.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <algorithm>

namespace esphome::tinyusb_audio {

static const char *const TAG = "tinyusb_audio";
static constexpr int32_t AUDIBLE_THRESHOLD = 64 << 16;
static constexpr uint32_t SILENCE_TIMEOUT_MS = 750;
TinyUSBAudio *global_tinyusb_audio = nullptr;

static void write_latest_frames(tu_fifo_t &fifo, std::span<const uint8_t> data, size_t frame_bytes) {
  const size_t aligned_bytes = data.size() - data.size() % frame_bytes;
  const size_t bytes = std::min<size_t>(aligned_bytes, tu_fifo_depth(&fifo));
  if (bytes == 0)
    return;

  // The caller holds the lock and FIFO depth, reads and writes are frame-aligned.
  const size_t free = tu_fifo_remaining(&fifo);
  if (bytes > free)
    tu_fifo_advance_read_pointer(&fifo, bytes - free);
  tu_fifo_write_n(&fifo, data.data() + aligned_bytes - bytes, bytes);
}

size_t TinyUSBAudio::playback_buffered_frames() {
  portENTER_CRITICAL(&this->fifo_lock_);
  const size_t frames = tu_fifo_count(&this->playback_fifo_) / PLAYBACK_FRAME_BYTES;
  portEXIT_CRITICAL(&this->fifo_lock_);
  return frames;
}

size_t TinyUSBAudio::read_playback(std::span<uint8_t> data) {
  portENTER_CRITICAL(&this->fifo_lock_);
  const size_t aligned_bytes = data.size() - data.size() % PLAYBACK_FRAME_BYTES;
  const size_t bytes = tu_fifo_read_n(&this->playback_fifo_, data.data(), aligned_bytes);
  portEXIT_CRITICAL(&this->fifo_lock_);
  return bytes;
}

void TinyUSBAudio::clear_playback() {
  portENTER_CRITICAL(&this->fifo_lock_);
  tu_fifo_clear(&this->playback_fifo_);
  portEXIT_CRITICAL(&this->fifo_lock_);
}

void TinyUSBAudio::setup() {
  tu_fifo_config(&this->playback_fifo_, this->playback_storage_.data(), this->playback_storage_.size(), false);
#if TINYUSB_AUDIO_CAPTURE
  tu_fifo_config(&this->capture_fifo_, this->capture_storage_.data(), this->capture_storage_.size(), false);
  this->microphone_source_.emplace(this->microphone_, TINYUSB_AUDIO_CAPTURE_BITS, 1, false);
  for (uint8_t channel = 0; channel < TINYUSB_AUDIO_CAPTURE_CHANNELS; channel++)
    this->microphone_source_->add_channel(channel);
  this->microphone_source_->add_data_callback([this](const microphone::AudioCapture &capture) {
    if (capture.stream_info.get_sample_rate() != TINYUSB_AUDIO_CAPTURE_RATE)
      return;
    portENTER_CRITICAL(&this->fifo_lock_);
    if (this->capture_active_.load(std::memory_order_relaxed))
      write_latest_frames(this->capture_fifo_, capture.data, TINYUSB_AUDIO_CAPTURE_FRAME_BYTES);
    portEXIT_CRITICAL(&this->fifo_lock_);
  });
#endif
  global_tinyusb_audio = this;
}

void TinyUSBAudio::dump_config() {
  ESP_LOGCONFIG(TAG, "TinyUSB Audio:\n  Playback: %u Hz, %u-bit stereo", TINYUSB_AUDIO_PLAYBACK_RATE,
                TINYUSB_AUDIO_PLAYBACK_BITS);
#if TINYUSB_AUDIO_CAPTURE
  ESP_LOGCONFIG(TAG, "  Capture: %u Hz, %u-bit, %u channel(s)", TINYUSB_AUDIO_CAPTURE_RATE, TINYUSB_AUDIO_CAPTURE_BITS,
                TINYUSB_AUDIO_CAPTURE_CHANNELS);
#endif
}

bool TinyUSBAudio::should_auto_start_playback() const {
  return this->is_playback_active() &&
         (this->playback_keep_alive_ ||
          (this->heard_audio_.load() && millis() - this->last_audible_ms_.load() < SILENCE_TIMEOUT_MS));
}

void TinyUSBAudio::receive_playback(std::span<const uint8_t> data) {
  if (!this->is_playback_active())
    return;
  if (!this->playback_keep_alive_) {
    static constexpr size_t SAMPLE_BYTES = TINYUSB_AUDIO_PLAYBACK_BITS / 8;
    for (size_t i = 0; i + SAMPLE_BYTES <= data.size(); i += SAMPLE_BYTES) {
      const int32_t sample = audio::unpack_audio_sample_to_q31(data.data() + i, SAMPLE_BYTES);
      if (sample > AUDIBLE_THRESHOLD || sample < -AUDIBLE_THRESHOLD) {
        this->last_audible_ms_.store(millis());
        this->heard_audio_.store(true);
        break;
      }
    }
  }
  portENTER_CRITICAL(&this->fifo_lock_);
  write_latest_frames(this->playback_fifo_, data, PLAYBACK_FRAME_BYTES);
  portEXIT_CRITICAL(&this->fifo_lock_);
}

void TinyUSBAudio::set_interface(uint8_t interface, bool active) {
  if (interface == PLAYBACK_INTERFACE)
    this->playback_open_ = active;
#if TINYUSB_AUDIO_CAPTURE
  if (interface == CAPTURE_INTERFACE)
    this->capture_open_ = active;
#endif
  this->update_streams_();
}

void TinyUSBAudio::on_usb_event(bool suspended, bool detached) {
  this->suspended_ = suspended;
  if (detached) {
    this->playback_open_ = false;
#if TINYUSB_AUDIO_CAPTURE
    this->capture_open_ = false;
#endif
  }
  this->update_streams_();
}

void TinyUSBAudio::update_streams_() {
  const bool playback_active = this->playback_open_ && !this->suspended_;
  if (!playback_active)
    this->heard_audio_.store(false);
  portENTER_CRITICAL(&this->fifo_lock_);
  if (playback_active != this->playback_active_.load(std::memory_order_relaxed)) {
    tu_fifo_clear(&this->playback_fifo_);
    this->playback_generation_.fetch_add(1);
    this->playback_active_.store(playback_active);
  }
#if TINYUSB_AUDIO_CAPTURE
  const bool capture_active = this->capture_open_ && !this->suspended_;
  if (capture_active != this->capture_active_.load(std::memory_order_relaxed)) {
    this->capture_phase_ = 0;
    this->capture_primed_ = false;
    tu_fifo_clear(&this->capture_fifo_);
    this->capture_active_.store(capture_active);
  }
#endif
  portEXIT_CRITICAL(&this->fifo_lock_);
}

#if TINYUSB_AUDIO_CAPTURE
void TinyUSBAudio::loop() {
  const bool active = this->capture_active_.load();
  if (active == this->microphone_started_)
    return;
  this->microphone_started_ = active;
  if (active)
    this->microphone_source_->start();
  else
    this->microphone_source_->stop();
}

void TinyUSBAudio::fill_capture_packet() {
  static constexpr size_t FRAME_BYTES = TINYUSB_AUDIO_CAPTURE_FRAME_BYTES;
  static constexpr size_t PACKET_BYTES = TINYUSB_AUDIO_CAPTURE_PACKET;
  std::array<uint8_t, PACKET_BYTES> packet{};
  portENTER_CRITICAL(&this->fifo_lock_);
  const size_t buffered = tu_fifo_count(&this->capture_fifo_);
  portEXIT_CRITICAL(&this->fifo_lock_);
  if (buffered >= PACKET_BYTES * 20)
    this->capture_primed_ = true;

  // Use fractional frames per USB millisecond and trim the rate by a few
  // samples/sec when the microphone and USB clocks drift apart.
  uint32_t rate = TINYUSB_AUDIO_CAPTURE_RATE;
  if (buffered > PACKET_BYTES * 32)
    rate += 5;
  else if (buffered < PACKET_BYTES * 10 && rate > 1000)
    rate -= 5;
  if constexpr (TINYUSB_AUDIO_CAPTURE_RATE % 1000 != 0) {
    constexpr uint32_t FLOOR_RATE = TINYUSB_AUDIO_CAPTURE_RATE / 1000 * 1000;
    rate = std::clamp(rate, FLOOR_RATE, FLOOR_RATE + 1000);
  }
  this->capture_phase_ += rate;
  const size_t bytes = this->capture_phase_ / 1000 * FRAME_BYTES;
  this->capture_phase_ %= 1000;
  if (this->capture_active_.load() && this->capture_primed_) {
    portENTER_CRITICAL(&this->fifo_lock_);
    const size_t consumed = tu_fifo_read_n(&this->capture_fifo_, packet.data(), bytes);
    portEXIT_CRITICAL(&this->fifo_lock_);
    this->capture_primed_ = consumed == bytes;
  }
  tud_audio_n_write(1, packet.data(), bytes);
}
#endif

}  // namespace esphome::tinyusb_audio
