#pragma once
#include <atomic>
#include <string>
#include "esphome/core/component.h"
#include "esphome/components/media_source/media_source.h"
#ifdef USE_A2DP_SHARED_BLUETOOTH
#include "esphome/components/esp32_ble/ble.h"
#endif
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_gap_bt_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"

namespace esphome::a2dp_sink {
class A2DPSink : public Component, public media_source::MediaSource {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_name(const std::string &name) { this->name_ = name; }
  void set_buffer_size(size_t size) { this->buffer_size_ = size; }
  void set_enabled(bool enabled);
#ifdef USE_A2DP_SHARED_BLUETOOTH
  void set_ble_parent(esp32_ble::ESP32BLE *parent) { this->ble_parent_ = parent; }
  void ble_before_disabled_event_handler();
#endif
  bool can_handle(const std::string &uri) const override { return uri == "a2dp://"; }
  bool play_uri(const std::string &uri) override;
  void handle_command(media_source::MediaSourceCommand command) override;
  bool has_internal_playlist() const override { return true; }

 protected:
  struct Packet { uint32_t generation; uint32_t rate; uint8_t channels; };
  static A2DPSink *instance_;
  static void a2dp_callback_(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
  static void gap_callback_(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
  static void avrc_callback_(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
  static void pcm_callback_(const uint8_t *data, uint32_t length);
  static void audio_task_(void *arg);
  bool check_(esp_err_t error);
  void initialize_profiles_();
  void send_key_(uint8_t key);
  std::string name_;
  size_t buffer_size_{32768};
  RingbufHandle_t buffer_{nullptr};
  std::atomic<bool> enabled_{true}, connected_{false}, streaming_{false}, selected_{false};
  std::atomic<bool> running_{false}, avrc_connected_{false};
  std::atomic<uint32_t> generation_{0}, sample_rate_{44100}, dropped_{0};
  std::atomic<uint8_t> channels_{2};
  esp_bd_addr_t peer_{};
  portMUX_TYPE peer_lock_ = portMUX_INITIALIZER_UNLOCKED;
  bool initialized_{false}, was_streaming_{false}, blocked_{false};
  uint8_t transaction_{0};
  uint32_t last_log_{0};
#ifdef USE_A2DP_SHARED_BLUETOOTH
  esp32_ble::ESP32BLE *ble_parent_{nullptr};
#endif
};
}  // namespace esphome::a2dp_sink
