#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "esphome/components/light/light_state.h"
#ifdef USE_SOUND_REACTIVE_USB_AUDIO_BRIDGE
#include "esphome/components/usb_audio_bridge/usb_audio_bridge.h"
#endif
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
#include "esphome/components/usb_media_player/media_source/usb_media_source.h"
#endif
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome {
namespace sound_reactive_rgb {

enum class SoundReactiveMode : uint8_t {
  OFF,
  STEREO_VU,
  SPECTRUM,
  RAINBOW_PULSE,
};

class SoundReactiveRGB : public Component
#ifdef USE_SOUND_REACTIVE_USB_AUDIO_BRIDGE
    , public usb_audio_bridge::AudioFrameListener
#endif
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
    , public usb_media_player::AudioFrameListener
#endif
{
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE - 1.0f; }

  void on_audio_frames(const int16_t *interleaved_stereo, size_t frames, uint32_t playback_gain_q16,
                       bool muted) override;

#ifdef USE_SOUND_REACTIVE_USB_AUDIO_BRIDGE
  void set_audio_source(usb_audio_bridge::USBAudioBridge *source) { source->add_audio_listener(this); }
#endif
#ifdef USE_USB_MEDIA_PLAYER_SOURCE
  void set_audio_source(usb_media_player::USBMediaSource *source) { source->add_audio_listener(this); }
#endif
  void set_left_light(light::LightState *light) { this->left_light_ = light; }
  void set_right_light(light::LightState *light) { this->right_light_ = light; }
  void set_mode(SoundReactiveMode mode);
  void set_brightness(float brightness) { this->brightness_ = brightness; }
  void set_sensitivity(float sensitivity) { this->sensitivity_ = sensitivity; }
  void set_noise_floor(float noise_floor) { this->noise_floor_ = noise_floor; }
  void set_smoothing(float smoothing) { this->smoothing_ = smoothing; }
  void set_update_interval(uint32_t update_interval_ms) { this->update_interval_ms_ = update_interval_ms; }
  void set_buffer_frames(size_t buffer_frames) { this->buffer_frames_ = buffer_frames; }
  void set_use_psram(bool use_psram) { this->use_psram_ = use_psram; }
  void set_follow_volume(bool follow_volume) { this->follow_volume_ = follow_volume; }
  void set_enabled(bool enabled) { this->enabled_ = enabled; }
  void set_mode_by_name(const std::string &mode);

 protected:
  struct RGB {
    float red;
    float green;
    float blue;
  };

  void analyze_();
  void render_(float left_level, float right_level, float bass, float mid, float treble);
  void write_light_(light::LightState *light, const RGB &color, float level);
  void turn_off_light_(light::LightState *light);
  void all_off_();
  float normalize_level_(float rms) const;
  static RGB vu_color_(float level);
  static RGB hsv_to_rgb_(float hue, float saturation, float value);

  light::LightState *left_light_{nullptr};
  light::LightState *right_light_{nullptr};

  int16_t *sample_buffer_{nullptr};
  size_t buffer_frames_{8192};
  volatile uint32_t write_sequence_{0};
  uint32_t read_sequence_{0};
  bool buffer_in_psram_{false};
  bool use_psram_{true};
  bool follow_volume_{false};

  SoundReactiveMode mode_{SoundReactiveMode::STEREO_VU};
  float brightness_{0.70f};
  float sensitivity_{4.0f};
  float noise_floor_{0.002f};
  float smoothing_{0.80f};
  float smoothed_left_{0.0f};
  float smoothed_right_{0.0f};
  float smoothed_bass_{0.0f};
  float smoothed_mid_{0.0f};
  float smoothed_treble_{0.0f};
  float low_filter_state_{0.0f};
  float mid_filter_state_{0.0f};
  uint32_t update_interval_ms_{20};
  uint32_t last_update_ms_{0};
  bool enabled_{true};
};

template<typename... Ts> class SetModeAction final : public Action<Ts...> {
 public:
  explicit SetModeAction(SoundReactiveRGB *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(std::string, mode)

  void play(const Ts &...x) override { this->parent_->set_mode_by_name(this->mode_.value(x...)); }

 protected:
  SoundReactiveRGB *parent_;
};

}  // namespace sound_reactive_rgb
}  // namespace esphome
