#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "driver/i2s_std.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esp_err.h"

#ifdef USE_OTA_STATE_LISTENER
#include "esphome/components/ota/ota_backend.h"
#endif

namespace esphome {
namespace usb_audio_bridge {

// USB HID Consumer Page usages used by Windows, Linux and Android.
enum class MediaKey : uint16_t {
  VOLUME_UP = 0x00E9,
  VOLUME_DOWN = 0x00EA,
  NEXT_TRACK = 0x00B5,
  PREVIOUS_TRACK = 0x00B6,
  PLAY_PAUSE = 0x00CD,
  STOP = 0x00B7,
};

class AudioFrameListener {
 public:
  virtual void on_audio_frames(const int16_t *interleaved_stereo, size_t frames, uint32_t playback_gain_q16,
                               bool muted) = 0;
};

class USBAudioBridge : public Component
#ifdef USE_OTA_STATE_LISTENER
                       , public ota::OTAGlobalStateListener
#endif
{
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

#ifdef USE_OTA_STATE_LISTENER
  void on_ota_global_state(ota::OTAState state, float progress, uint8_t error,
                           ota::OTAComponent *component) override;
#endif

  void set_bclk_pin(int pin) { this->bclk_pin_ = pin; }
  void set_lrclk_pin(int pin) { this->lrclk_pin_ = pin; }
  void set_dac_dout_pin(int pin) { this->dac_dout_pin_ = pin; }
  void set_sample_rate(uint32_t sample_rate) { this->sample_rate_ = sample_rate; }
  void set_output_gain_db(float output_gain_db);
  void add_audio_listener(AudioFrameListener *listener) { this->audio_listeners_.push_back(listener); }
  void send_media_key(MediaKey key);
  void press_media_key(MediaKey key);
  void release_media_key(MediaKey key);

 protected:
  static constexpr size_t CONVERSION_FRAMES = 256;
  static constexpr uint8_t MEDIA_KEY_QUEUE_SIZE = 8;

  esp_err_t initialize_i2s_();
  esp_err_t write_speaker_(uint8_t *buffer, size_t length);
  void update_playback_gain_();
  void suspend_playback_();
  void resume_playback_();
  void process_media_keys_();
  void clear_media_key_queue_();

  static esp_err_t speaker_output_cb_(uint8_t *buffer, size_t length, void *context);
  static void mute_cb_(uint32_t mute, void *context);
  static void volume_cb_(uint32_t volume, void *context);

  int bclk_pin_{5};
  int lrclk_pin_{6};
  int dac_dout_pin_{7};
  uint32_t sample_rate_{48000};

  i2s_chan_handle_t tx_handle_{nullptr};
  int32_t speaker_i2s_[CONVERSION_FRAMES * 2]{};

  volatile bool muted_{false};
  volatile uint32_t volume_percent_{100};
  volatile uint32_t playback_gain_q16_{65536};
  volatile bool playback_suspended_{false};
  float output_gain_db_{0.0f};
  std::vector<AudioFrameListener *> audio_listeners_;

  MediaKey media_key_queue_[MEDIA_KEY_QUEUE_SIZE]{};
  uint8_t media_key_queue_head_{0};
  uint8_t media_key_queue_tail_{0};
  uint8_t media_key_queue_count_{0};
  uint16_t active_media_key_usage_{0};
  uint16_t held_media_key_usage_{0};
};

template<typename... Ts> class MediaKeyAction final : public Action<Ts...> {
 public:
  MediaKeyAction(USBAudioBridge *parent, MediaKey media_key) : parent_(parent), media_key_(media_key) {}

  void play(const Ts &...x) override { this->parent_->send_media_key(this->media_key_); }

 protected:
  USBAudioBridge *parent_;
  MediaKey media_key_;
};

template<typename... Ts> class MediaKeyPressedAction final : public Action<Ts...> {
 public:
  MediaKeyPressedAction(USBAudioBridge *parent, MediaKey media_key) : parent_(parent), media_key_(media_key) {}

  void play(const Ts &...x) override { this->parent_->press_media_key(this->media_key_); }

 protected:
  USBAudioBridge *parent_;
  MediaKey media_key_;
};

template<typename... Ts> class MediaKeyReleasedAction final : public Action<Ts...> {
 public:
  MediaKeyReleasedAction(USBAudioBridge *parent, MediaKey media_key) : parent_(parent), media_key_(media_key) {}

  void play(const Ts &...x) override { this->parent_->release_media_key(this->media_key_); }

 protected:
  USBAudioBridge *parent_;
  MediaKey media_key_;
};

}  // namespace usb_audio_bridge
}  // namespace esphome
