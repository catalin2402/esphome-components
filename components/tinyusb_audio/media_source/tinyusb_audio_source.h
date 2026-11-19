#pragma once

#include "esphome/components/media_source/media_source.h"
#include "esphome/components/tinyusb_audio/tinyusb_audio.h"
#include "esphome/core/defines.h"

#ifdef USE_TINYUSB_AUDIO_KEYBOARD
#include "esphome/components/tinyusb_keyboard/keyboard.h"
#endif

#include "esphome/core/static_task.h"

namespace esphome::tinyusb_audio {

class TinyUSBAudioSource final : public Component, public media_source::MediaSource, public Parented<TinyUSBAudio> {
 public:
#ifdef USE_TINYUSB_AUDIO_KEYBOARD
  void set_keyboard(tinyusb_keyboard::TinyUSBKeyboard *keyboard) { this->keyboard_ = keyboard; }
#endif
  void setup() override;
  void loop() override;
  void on_shutdown() override { this->stop_(); }
  bool can_handle(const std::string &uri) const override;
  bool play_uri(const std::string &uri) override;
  void handle_command(media_source::MediaSourceCommand command) override;
  bool has_internal_playlist() const override { return true; }

 protected:
  void stop_();
  void playback_task_run_();
  bool is_selected_() const { return (this->selection_.load() & 1) != 0; }
#ifdef USE_TINYUSB_AUDIO_KEYBOARD
  void send_media_key_(uint16_t usage);
  tinyusb_keyboard::TinyUSBKeyboard *keyboard_{nullptr};
#endif

  // Keep the pending audio outside the FreeRTOS task stack.
  std::array<uint8_t, TINYUSB_AUDIO_PLAYBACK_PACKET * 4> pending_{};
  StaticTask playback_task_;
  uint32_t generation_{0};
  bool auto_start_ready_{false};
  // Transitions increment this counter. The low bit indicates an active selection.
  std::atomic<uint32_t> selection_{0};
  std::atomic<uint32_t> selected_generation_{0};
};

}  // namespace esphome::tinyusb_audio
