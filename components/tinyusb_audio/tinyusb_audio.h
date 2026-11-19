#pragma once

#include "tusb.h"

#include "esphome/core/component.h"
#include "esphome/components/audio/audio.h"
#if TINYUSB_AUDIO_CAPTURE
#include "esphome/components/microphone/microphone_source.h"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <optional>
#include <span>

#include "freertos/FreeRTOS.h"

namespace esphome::tinyusb_audio {

extern const uint8_t CONFIGURATION_DESCRIPTOR[];
inline constexpr size_t PLAYBACK_FRAME_BYTES = TINYUSB_AUDIO_PLAYBACK_FRAME_BYTES;
// TinyUSB limits a byte FIFO to 32 KiB. Keep both FIFO depths packet- and frame-aligned.
inline constexpr size_t PLAYBACK_FIFO_PACKETS = std::min<size_t>(40, 0x8000 / TINYUSB_AUDIO_PLAYBACK_PACKET);
inline constexpr uint8_t PLAYBACK_INTERFACE = 1;
inline constexpr uint8_t CAPTURE_INTERFACE = 3;
inline constexpr int16_t PLAYBACK_VOLUME_MIN = -50 * 256;  // USB Audio volumes are in 1/256 dB.

class TinyUSBAudio final : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::BUS + 1.0f; }
  bool is_playback_active() const { return this->playback_active_.load(); }
  uint32_t playback_generation() const { return this->playback_generation_.load(); }
  void set_playback_keep_alive(bool keep_alive) { this->playback_keep_alive_ = keep_alive; }
  bool is_playback_keep_alive() const { return this->playback_keep_alive_; }
  bool should_auto_start_playback() const;
  size_t playback_buffered_frames();
  size_t read_playback(std::span<uint8_t> data);
  void clear_playback();
  void receive_playback(std::span<const uint8_t> data);
  void set_interface(uint8_t interface, bool active);
  void on_usb_event(bool suspended, bool detached);
  int16_t host_volume() const { return this->host_volume_.load(); }
  bool host_muted() const { return this->host_muted_.load(); }
  void set_host_volume(int16_t volume) {
    this->host_volume_.store(volume);
    this->host_volume_changed_.store(true);
  }
  void set_host_muted(bool muted) {
    this->host_muted_.store(muted);
    this->host_mute_changed_.store(true);
  }
  bool take_host_volume_change() { return this->host_volume_changed_.exchange(false); }
  bool take_host_mute_change() { return this->host_mute_changed_.exchange(false); }

#if TINYUSB_AUDIO_CAPTURE
  void set_microphone(microphone::Microphone *mic) { this->microphone_ = mic; }
  void loop() override;
  void fill_capture_packet();
#endif

 protected:
  void update_streams_();

  // Interface and suspend state are owned by the TinyUSB task.
  bool playback_open_{false};
  bool playback_keep_alive_{false};
  bool suspended_{false};
  std::array<uint8_t, TINYUSB_AUDIO_PLAYBACK_PACKET * PLAYBACK_FIFO_PACKETS> playback_storage_{};
  tu_fifo_t playback_fifo_{};
  std::atomic<bool> playback_active_{false};
  std::atomic<uint32_t> playback_generation_{0};
  portMUX_TYPE fifo_lock_ = portMUX_INITIALIZER_UNLOCKED;
  std::atomic<bool> heard_audio_{false};
  std::atomic<uint32_t> last_audible_ms_{0};
  std::atomic<int16_t> host_volume_{0};
  std::atomic<bool> host_muted_{false};
  std::atomic<bool> host_volume_changed_{false};
  std::atomic<bool> host_mute_changed_{false};

#if TINYUSB_AUDIO_CAPTURE
  bool capture_open_{false};
  bool capture_primed_{false};
  uint32_t capture_phase_{0};
  // TinyUSB limits a byte FIFO to 32 KiB. Keep its depth a multiple of the packet size.
  static constexpr size_t CAPTURE_FIFO_PACKETS = std::min<size_t>(48, 0x8000 / TINYUSB_AUDIO_CAPTURE_PACKET);
  std::array<uint8_t, TINYUSB_AUDIO_CAPTURE_PACKET * CAPTURE_FIFO_PACKETS> capture_storage_{};
  tu_fifo_t capture_fifo_{};
  std::atomic<bool> capture_active_{false};
  microphone::Microphone *microphone_{nullptr};
  std::optional<microphone::MicrophoneSource> microphone_source_;
  bool microphone_started_{false};
#endif
};

extern TinyUSBAudio *global_tinyusb_audio;

}  // namespace esphome::tinyusb_audio
