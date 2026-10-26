#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "esphome/components/media_source/media_source.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "esphome/components/usb_media_player/usb_media_player.h"

namespace esphome::usb_media_player {

class USBMediaSource : public Component, public media_source::MediaSource {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  bool can_handle(const std::string &uri) const override { return uri == "usb://"; }
  bool play_uri(const std::string &uri) override;
  void handle_command(media_source::MediaSourceCommand command) override;
  // The connected host owns track selection, so NEXT/PREVIOUS must be delegated to it.
  bool has_internal_playlist() const override { return true; }

  void set_prefill_ms(uint32_t prefill_ms) { this->prefill_ms_ = prefill_ms; }
  void set_buffer_size(size_t size) { this->buffer_size_ = size; }
  void set_use_psram(bool use_psram) { this->use_psram_ = use_psram; }
  void set_auto_start(bool auto_start) { this->auto_start_ = auto_start; }
  void set_output_gain_db(float gain) { this->output_gain_db_ = gain; }
  bool is_connected() const { return this->connected_; }
  bool is_streaming() const { return this->streaming_; }
  void set_parent(USBMediaPlayer *parent) { this->parent_ = parent; }
  void send_media_key(MediaKey key) { this->parent_->send_media_key(key); }
  esp_err_t receive_audio(const uint8_t *data, size_t length);
  void set_host_mute(bool mute) { this->host_muted_.store(mute); }
  void set_host_volume(uint32_t volume);
  void set_suspended(bool suspended);

  template<typename F> void add_on_stream_callback(F &&callback) {
    this->stream_callback_.add(std::forward<F>(callback));
  }

 protected:
  static constexpr size_t PACKET_BYTES = 1024;
  struct PacketHeader {
    uint32_t generation;
    uint32_t gain_q16;
  };

  static void audio_task_(void *context);
  void write_audio_();
  void start_audio_();
  void stop_audio_();
  void update_stream_state_();
  USBMediaPlayer *parent_{nullptr};
  RingbufHandle_t buffer_{nullptr};
  TaskHandle_t task_{nullptr};
  std::atomic<bool> task_running_{false};
  std::atomic<bool> playback_enabled_{false};
  std::atomic<bool> prebuffer_enabled_{false};
  std::atomic<bool> suspended_{false};
  std::atomic<bool> host_muted_{false};
  std::atomic<uint32_t> generation_{0};
  std::atomic<uint32_t> host_gain_q16_{65536};
  std::atomic<uint32_t> dropped_frames_{0};
  std::atomic<size_t> queued_bytes_{0};
  size_t buffer_size_{32768};
  uint32_t last_drop_log_{0};
  uint32_t prefill_ms_{100};
  float output_gain_db_{0.0f};
  uint32_t output_gain_q16_{65536};
  bool use_psram_{true};
  bool buffer_in_psram_{false};
  bool auto_start_{true};
  bool connected_{false};
  bool streaming_{false};
  bool auto_start_blocked_{false};
  bool resume_after_ota_{false};
  LazyCallbackManager<void(bool)> stream_callback_;
};

}  // namespace esphome::usb_media_player
