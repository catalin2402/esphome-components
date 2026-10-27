#pragma once
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/speaker/speaker.h"
#include "esphome/components/microphone/microphone.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <atomic>
#include <vector>
#ifdef USE_OTA_STATE_LISTENER
#include "esphome/components/ota/ota_backend.h"
#endif

namespace esphome::usb_audio_device {
enum MediaKey : uint16_t {
  MEDIA_KEY_PLAY = 0xB0,
  MEDIA_KEY_PAUSE = 0xB1,
  MEDIA_KEY_FAST_FORWARD = 0xB3,
  MEDIA_KEY_REWIND = 0xB4,
  MEDIA_KEY_NEXT = 0xB5,
  MEDIA_KEY_PREVIOUS = 0xB6,
  MEDIA_KEY_STOP = 0xB7,
  MEDIA_KEY_PLAY_PAUSE = 0xCD,
  MEDIA_KEY_VOLUME_UP = 0xE9,
  MEDIA_KEY_VOLUME_DOWN = 0xEA,
};

// Optional transport registered by the HID component. Audio and media-source
// code depend only on this interface, never on the concrete HID component.
class USBMediaTransport {
 public:
  virtual ~USBMediaTransport() = default;
  virtual void send_media_key(uint16_t usage) = 0;
};
// Alternate playback destination used by the standard media_source adapter.
// Selected state and PCM writes are accessed from the playback worker.
class USBPlaybackSink {
 public:
  virtual ~USBPlaybackSink() = default;
  virtual bool is_selected() const = 0;
  virtual void stop_playback() = 0;
  virtual size_t write_pcm(const uint8_t *data, size_t bytes, uint32_t timeout_ms) = 0;
};
// Frame-aligned bounded queues. Only copies/count updates run under the lock;
// no USB, speaker, microphone, allocation, or light calls are made there.
class PCMQueue {
 public:
  bool allocate(size_t bytes);
  void reset();
  void release();
  size_t size();
  size_t push(const uint8_t *data, size_t bytes, size_t frame_bytes);
  bool pop(uint8_t *data, size_t bytes);

 protected:
  uint8_t *data_{nullptr};
  size_t capacity_{0}, head_{0}, count_{0};
  portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;
};

class USBAudioDevice : public Component
#ifdef USE_OTA_STATE_LISTENER
    ,
                       public ota::OTAGlobalStateListener
#endif
{
 public:
  void set_media_transport(USBMediaTransport *transport) { this->media_transport_ = transport; }
  bool has_media_transport() const { return this->media_transport_ != nullptr; }
  void send_media_key(uint16_t usage) {
    if (this->media_transport_)
      this->media_transport_->send_media_key(usage);
  }
  void set_playback_sink(USBPlaybackSink *sink) { this->playback_sink_ = sink; }
  void reset_playback_buffer() {
    this->playback_generation_.fetch_add(1);
    this->playback_.reset();
  }
  void set_speaker(speaker::Speaker *value) { this->speaker_ = value; }
  void set_microphone(microphone::Microphone *value) { this->microphone_ = value; }
  void set_vendor_id(uint16_t value) { this->vendor_id_ = value; }
  void set_product_id(uint16_t value) { this->product_id_ = value; }
  void set_manufacturer(const char *value) { this->manufacturer_ = value; }
  void set_product(const char *value) { this->product_ = value; }
  void set_serial(const char *value) { this->serial_ = value; }
  void add_playback_activity_callback(std::function<void(bool)> callback) {
    this->playback_activity_callback_.add(std::move(callback));
    this->monitor_playback_.store(true);
  }
  void set_connected(bool connected) { this->connected_.store(connected); }
  bool is_connected() const { return this->ready_.load() && !this->shutting_down_.load() && this->connected_.load(); }
  bool is_capture_active() const {
    return this->ready_.load() && !this->shutting_down_.load() && !this->audio_suspended_.load() &&
           this->capture_active_.load();
  }
  bool is_microphone_muted() const { return this->mic_muted_.load(); }
  template<typename F> void add_connection_callback(F &&callback) {
    this->connection_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_playback_callback(F &&callback) {
    this->playback_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_capture_callback(F &&callback) {
    this->capture_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_microphone_mute_callback(F &&callback) {
    this->microphone_mute_callback_.add(std::forward<F>(callback));
  }
  void set_activity_threshold(uint16_t value) { this->activity_threshold_ = value; }
  void set_activity_timeout(uint32_t value) { this->activity_timeout_ = value; }
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  void on_shutdown() override;
#ifdef USE_OTA_STATE_LISTENER
  void on_ota_global_state(ota::OTAState state, float progress, uint8_t error, ota::OTAComponent *component) override;
#endif
  void set_microphone_muted(bool value);
  void set_playback_active(bool value);
  bool is_playback_active() const {
    return this->ready_.load() && !this->shutting_down_.load() && !this->audio_suspended_.load() &&
           this->playback_active_.load();
  }
  void set_capture_active(bool value);
  void set_host_volume(uint32_t percent);
  void set_host_mute(bool value) { this->host_muted_.store(value); }
  void receive_playback(const uint8_t *data, size_t bytes);
  void read_capture(uint8_t *data, size_t bytes, size_t *read);

 protected:
  static void playback_task_(void *arg);
  void receive_microphone_(const std::vector<uint8_t> &data);
  void fail_setup_(const char *message);
#ifdef USE_OTA_STATE_LISTENER
  void apply_ota_state_();
#endif
  speaker::Speaker *speaker_{nullptr};
  USBPlaybackSink *playback_sink_{nullptr};
  USBMediaTransport *media_transport_{nullptr};
  uint16_t vendor_id_{0x303A}, product_id_{0x8001};
  const char *manufacturer_{"ESPHome"}, *product_{"USB Headset"}, *serial_{"ESPHOME-UAC-1"};
  microphone::Microphone *microphone_{nullptr};
  PCMQueue playback_, capture_;
  std::atomic<bool> playback_active_{false}, capture_active_{false}, mic_muted_{false}, host_muted_{false};
  std::atomic<uint32_t> playback_generation_{0};
  std::atomic<bool> audio_suspended_{false};
  std::atomic<bool> ready_{false}, shutting_down_{false};
  TaskHandle_t playback_task_handle_{nullptr};
  std::atomic<int32_t> gain_q16_{65536};
  std::atomic<uint32_t> dropped_playback_{0}, dropped_capture_{0}, playback_underruns_{0};
  bool microphone_started_{false}, capture_primed_{false};
  uint32_t capture_generation_seen_{0};
  std::atomic<uint32_t> capture_generation_{0};
  uint32_t last_diagnostics_{0};
  LazyCallbackManager<void(bool)> playback_activity_callback_;
  std::atomic<bool> monitor_playback_{false}, playback_signal_seen_{false};
  std::atomic<uint32_t> last_playback_signal_{0};
  uint16_t activity_threshold_{64};
  uint32_t activity_timeout_{750};
  bool playback_audible_{false};
  std::atomic<bool> connected_{false};
  bool connection_state_{false}, playback_state_{false}, capture_state_{false};
  LazyCallbackManager<void(bool)> connection_callback_;
  LazyCallbackManager<void(bool)> playback_callback_;
  LazyCallbackManager<void(bool)> capture_callback_;
  LazyCallbackManager<void(bool)> microphone_mute_callback_;
};
}  // namespace esphome::usb_audio_device
