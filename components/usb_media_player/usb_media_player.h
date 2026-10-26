#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include "esp_err.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
#include <vector>
#include "freertos/FreeRTOS.h"
#include "esphome/components/microphone/microphone_source.h"
#endif
#ifdef USE_OTA_STATE_LISTENER
#include "esphome/components/ota/ota_backend.h"
#endif

namespace esphome::usb_media_player {

class USBMediaSource;

enum class MediaKey : uint16_t {
  PLAY = 0x00B0,
  PAUSE = 0x00B1,
  NEXT_TRACK = 0x00B5,
  PREVIOUS_TRACK = 0x00B6,
  STOP = 0x00B7,
  PLAY_PAUSE = 0x00CD,
  VOLUME_UP = 0x00E9,
  VOLUME_DOWN = 0x00EA,
};

class USBMediaPlayer : public Component
#ifdef USE_OTA_STATE_LISTENER
    ,
                       public ota::OTAGlobalStateListener
#endif
{
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  void set_source(USBMediaSource *source);
  bool is_connected() const { return this->connected_; }
  bool is_streaming() const;
  bool is_microphone_enabled() const { return this->microphone_enabled_; }
  void set_microphone_enabled(bool enabled);
  void send_media_key(MediaKey key);
  void press_media_key(MediaKey key);
  void release_media_key(MediaKey key);

#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
  void set_microphone(microphone::MicrophoneSource *microphone) { this->microphone_ = microphone; }
  void set_microphone_buffer_size(size_t size) { this->microphone_buffer_size_ = size; }
#endif
  template<typename F> void add_on_connection_callback(F &&callback) {
    this->connection_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_stream_callback(F &&callback) {
    this->stream_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_microphone_callback(F &&callback) {
    this->microphone_callback_.add(std::forward<F>(callback));
  }
#ifdef USE_OTA_STATE_LISTENER
  void on_ota_global_state(ota::OTAState state, float progress, uint8_t error, ota::OTAComponent *component) override;
#endif

 protected:
  static constexpr uint8_t MEDIA_KEY_QUEUE_SIZE = 8;
  void process_media_keys_();
  void clear_media_keys_();
  static esp_err_t output_cb_(uint8_t *data, size_t length, void *context);
  static void mute_cb_(uint32_t mute, void *context);
  static void volume_cb_(uint32_t volume, void *context);
#ifdef USE_USB_MEDIA_PLAYER_MICROPHONE
  void update_microphone_();
  void microphone_data_(const std::vector<uint8_t> &data);
  void read_microphone_(uint8_t *data, size_t length);
  static esp_err_t input_cb_(uint8_t *data, size_t length, size_t *bytes_read, void *context);
  microphone::MicrophoneSource *microphone_{nullptr};
  uint8_t *microphone_buffer_{nullptr};
  size_t microphone_buffer_size_{8192};
  size_t microphone_head_{0};
  size_t microphone_count_{0};
  bool capture_active_{false};
  portMUX_TYPE microphone_lock_ = portMUX_INITIALIZER_UNLOCKED;
#endif
  USBMediaSource *source_{nullptr};
  bool connected_{false};
  bool initialized_{false};
  bool microphone_enabled_{false};
  std::atomic<bool> suspended_{false};
  MediaKey media_keys_[MEDIA_KEY_QUEUE_SIZE]{};
  uint8_t key_head_{0};
  uint8_t key_tail_{0};
  uint8_t key_count_{0};
  uint16_t active_key_{0};
  uint16_t held_key_{0};
  LazyCallbackManager<void(bool)> connection_callback_;
  LazyCallbackManager<void(bool)> stream_callback_;
  LazyCallbackManager<void(bool)> microphone_callback_;
};

}  // namespace esphome::usb_media_player
