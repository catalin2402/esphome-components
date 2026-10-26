#include "sound_reactive_rgb.h"

#include <algorithm>
#include <cmath>

#include "esp_heap_caps.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sound_reactive_rgb {

static const char *const TAG = "sound_reactive_rgb";
static constexpr float SAMPLE_RATE = 48000.0f;
static constexpr float TWO_PI = 6.28318530718f;

void SoundReactiveRGB::setup() {
  if (this->left_light_ == nullptr && this->right_light_ == nullptr) {
    ESP_LOGW(TAG, "No light is configured; the component will remain idle");
    return;
  }

  const size_t bytes = this->buffer_frames_ * 2 * sizeof(int16_t);
  if (this->use_psram_) {
    this->sample_buffer_ = static_cast<int16_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    this->buffer_in_psram_ = this->sample_buffer_ != nullptr;
  }
  if (this->sample_buffer_ == nullptr) {
    this->sample_buffer_ = static_cast<int16_t *>(heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  }
  if (this->sample_buffer_ == nullptr) {
    ESP_LOGE(TAG, "Unable to allocate %u-byte audio analysis buffer", static_cast<unsigned>(bytes));
    this->mark_failed();
    this->all_off_();
    return;
  }

  this->all_off_();
  ESP_LOGI(TAG, "Allocated %u audio frames in %s", static_cast<unsigned>(this->buffer_frames_),
           this->buffer_in_psram_ ? "PSRAM" : "internal RAM");
}

void SoundReactiveRGB::dump_config() {
  ESP_LOGCONFIG(TAG, "Sound Reactive RGB Lights:");
  ESP_LOGCONFIG(TAG, "  Left light: %s", this->left_light_ != nullptr ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  Right light: %s", this->right_light_ != nullptr ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  Mode: %s", this->mode_ == SoundReactiveMode::OFF
                                      ? "off"
                                      : this->mode_ == SoundReactiveMode::STEREO_VU
                                            ? "stereo_vu"
                                            : this->mode_ == SoundReactiveMode::SPECTRUM ? "spectrum"
                                                                                       : "rainbow_pulse");
  ESP_LOGCONFIG(TAG, "  Brightness: %.0f%%", this->brightness_ * 100.0f);
  ESP_LOGCONFIG(TAG, "  Sensitivity: %.2f", this->sensitivity_);
  ESP_LOGCONFIG(TAG, "  Noise floor: %.3f", this->noise_floor_);
  ESP_LOGCONFIG(TAG, "  Smoothing: %.2f", this->smoothing_);
  ESP_LOGCONFIG(TAG, "  Update interval: %u ms", static_cast<unsigned>(this->update_interval_ms_));
  ESP_LOGCONFIG(TAG, "  Analysis buffer: %u stereo frames (%s)", static_cast<unsigned>(this->buffer_frames_),
                this->buffer_in_psram_ ? "PSRAM" : "internal RAM");
  ESP_LOGCONFIG(TAG, "  Follow USB source volume: %s", YESNO(this->follow_volume_));
}

void SoundReactiveRGB::on_shutdown() { this->all_off_(); }

void SoundReactiveRGB::set_mode(SoundReactiveMode mode) {
  const bool was_off = this->mode_ == SoundReactiveMode::OFF;
  this->mode_ = mode;

  if (mode == SoundReactiveMode::OFF) {
    this->smoothed_left_ = 0.0f;
    this->smoothed_right_ = 0.0f;
    this->smoothed_bass_ = 0.0f;
    this->smoothed_mid_ = 0.0f;
    this->smoothed_treble_ = 0.0f;
    this->low_filter_state_ = 0.0f;
    this->mid_filter_state_ = 0.0f;
    this->all_off_();
  } else if (was_off) {
    this->read_sequence_ = __atomic_load_n(&this->write_sequence_, __ATOMIC_ACQUIRE);
  }
}

void SoundReactiveRGB::set_mode_by_name(const std::string &mode) {
  if (mode == "off") {
    this->set_mode(SoundReactiveMode::OFF);
  } else if (mode == "stereo_vu") {
    this->set_mode(SoundReactiveMode::STEREO_VU);
  } else if (mode == "spectrum") {
    this->set_mode(SoundReactiveMode::SPECTRUM);
  } else if (mode == "rainbow_pulse") {
    this->set_mode(SoundReactiveMode::RAINBOW_PULSE);
  } else {
    ESP_LOGW(TAG, "Unknown sound-reactive mode: %s", mode.c_str());
  }
}

void SoundReactiveRGB::on_audio_frames(const int16_t *interleaved_stereo, size_t frames, uint32_t playback_gain_q16,
                                       bool muted) {
  if (this->sample_buffer_ == nullptr || frames == 0) {
    return;
  }

  uint32_t sequence = __atomic_load_n(&this->write_sequence_, __ATOMIC_RELAXED);
  for (size_t frame = 0; frame < frames; ++frame) {
    const size_t destination = static_cast<size_t>((sequence + frame) % this->buffer_frames_) * 2;
    if (muted) {
      this->sample_buffer_[destination] = 0;
      this->sample_buffer_[destination + 1] = 0;
    } else if (this->follow_volume_) {
      this->sample_buffer_[destination] = static_cast<int16_t>(
          (static_cast<int64_t>(interleaved_stereo[frame * 2]) * playback_gain_q16) / 65536);
      this->sample_buffer_[destination + 1] = static_cast<int16_t>(
          (static_cast<int64_t>(interleaved_stereo[frame * 2 + 1]) * playback_gain_q16) / 65536);
    } else {
      this->sample_buffer_[destination] = interleaved_stereo[frame * 2];
      this->sample_buffer_[destination + 1] = interleaved_stereo[frame * 2 + 1];
    }
  }
  __atomic_store_n(&this->write_sequence_, sequence + static_cast<uint32_t>(frames), __ATOMIC_RELEASE);
}

void SoundReactiveRGB::loop() {
  const uint32_t now = millis();
  if (now - this->last_update_ms_ < this->update_interval_ms_) {
    return;
  }
  this->last_update_ms_ = now;

  if (!this->enabled_ || this->is_failed()) {
    this->all_off_();
    return;
  }
  if (this->mode_ == SoundReactiveMode::OFF) {
    return;
  }
  this->analyze_();
}

void SoundReactiveRGB::analyze_() {
  const uint32_t end_sequence = __atomic_load_n(&this->write_sequence_, __ATOMIC_ACQUIRE);
  uint32_t available = end_sequence - this->read_sequence_;
  if (available > this->buffer_frames_) {
    this->read_sequence_ = end_sequence - static_cast<uint32_t>(this->buffer_frames_);
    available = static_cast<uint32_t>(this->buffer_frames_);
  }

  double left_energy = 0.0;
  double right_energy = 0.0;
  double bass_energy = 0.0;
  double mid_energy = 0.0;
  double treble_energy = 0.0;

  const float bass_alpha = 1.0f - std::exp(-TWO_PI * 200.0f / SAMPLE_RATE);
  const float mid_alpha = 1.0f - std::exp(-TWO_PI * 2000.0f / SAMPLE_RATE);

  for (uint32_t offset = 0; offset < available; ++offset) {
    const size_t source = static_cast<size_t>((this->read_sequence_ + offset) % this->buffer_frames_) * 2;
    const float left = static_cast<float>(this->sample_buffer_[source]) / 32768.0f;
    const float right = static_cast<float>(this->sample_buffer_[source + 1]) / 32768.0f;
    const float mono = (left + right) * 0.5f;

    this->low_filter_state_ += bass_alpha * (mono - this->low_filter_state_);
    this->mid_filter_state_ += mid_alpha * (mono - this->mid_filter_state_);
    const float bass = this->low_filter_state_;
    const float mid = this->mid_filter_state_ - this->low_filter_state_;
    const float treble = mono - this->mid_filter_state_;

    left_energy += static_cast<double>(left) * left;
    right_energy += static_cast<double>(right) * right;
    bass_energy += static_cast<double>(bass) * bass;
    mid_energy += static_cast<double>(mid) * mid;
    treble_energy += static_cast<double>(treble) * treble;
  }
  this->read_sequence_ = end_sequence;

  float left_level = 0.0f;
  float right_level = 0.0f;
  float bass_level = 0.0f;
  float mid_level = 0.0f;
  float treble_level = 0.0f;
  if (available != 0) {
    const double divisor = static_cast<double>(available);
    left_level = this->normalize_level_(std::sqrt(left_energy / divisor));
    right_level = this->normalize_level_(std::sqrt(right_energy / divisor));
    bass_level = this->normalize_level_(std::sqrt(bass_energy / divisor));
    mid_level = this->normalize_level_(std::sqrt(mid_energy / divisor));
    treble_level = this->normalize_level_(std::sqrt(treble_energy / divisor));
  }

  const auto smooth = [this](float previous, float target) {
    const float retention = target > previous ? this->smoothing_ * 0.35f : this->smoothing_;
    return previous * retention + target * (1.0f - retention);
  };
  this->smoothed_left_ = smooth(this->smoothed_left_, left_level);
  this->smoothed_right_ = smooth(this->smoothed_right_, right_level);
  this->smoothed_bass_ = smooth(this->smoothed_bass_, bass_level);
  this->smoothed_mid_ = smooth(this->smoothed_mid_, mid_level);
  this->smoothed_treble_ = smooth(this->smoothed_treble_, treble_level);

  this->render_(this->smoothed_left_, this->smoothed_right_, this->smoothed_bass_, this->smoothed_mid_,
                this->smoothed_treble_);
}

float SoundReactiveRGB::normalize_level_(float rms) const {
  return std::clamp((rms - this->noise_floor_) * this->sensitivity_, 0.0f, 1.0f);
}

void SoundReactiveRGB::render_(float left_level, float right_level, float bass, float mid, float treble) {
  if (this->mode_ == SoundReactiveMode::STEREO_VU) {
    this->write_light_(this->left_light_, this->vu_color_(left_level), left_level);
    this->write_light_(this->right_light_, this->vu_color_(right_level), right_level);
    return;
  }

  if (this->mode_ == SoundReactiveMode::SPECTRUM) {
    const float maximum = std::max({bass, mid, treble, 0.001f});
    const RGB spectrum{bass / maximum, mid / maximum, treble / maximum};
    this->write_light_(this->left_light_, spectrum, left_level);
    this->write_light_(this->right_light_, spectrum, right_level);
    return;
  }

  const float hue = std::fmod(static_cast<float>(millis()) / 12000.0f + bass * 0.12f, 1.0f);
  const RGB rainbow = this->hsv_to_rgb_(hue, 1.0f, 1.0f);
  this->write_light_(this->left_light_, rainbow, left_level);
  this->write_light_(this->right_light_, rainbow, right_level);
}

void SoundReactiveRGB::write_light_(light::LightState *light_state, const RGB &color, float level) {
  if (light_state == nullptr) {
    return;
  }

  const float brightness = std::clamp(level * this->brightness_, 0.0f, 1.0f);
  auto call = light_state->make_call();
  call.set_transition_length(0);
  call.set_publish(false);
  call.set_save(false);
  call.set_effect(static_cast<uint32_t>(0));
  if (brightness <= 0.0001f) {
    call.set_state(false);
  } else {
    call.set_state(true);
    call.set_brightness(brightness);
    call.set_rgb(std::clamp(color.red, 0.0f, 1.0f), std::clamp(color.green, 0.0f, 1.0f),
                 std::clamp(color.blue, 0.0f, 1.0f));
  }
  call.perform();
}

void SoundReactiveRGB::all_off_() {
  this->turn_off_light_(this->left_light_);
  this->turn_off_light_(this->right_light_);
}

void SoundReactiveRGB::turn_off_light_(light::LightState *light_state) {
  if (light_state == nullptr) {
    return;
  }
  auto call = light_state->turn_off();
  call.set_transition_length(0);
  call.set_publish(false);
  call.set_save(false);
  call.set_effect(static_cast<uint32_t>(0));
  call.perform();
}

SoundReactiveRGB::RGB SoundReactiveRGB::vu_color_(float level) {
  level = std::clamp(level, 0.0f, 1.0f);
  if (level < 0.60f) {
    return RGB{level / 0.60f, 1.0f, 0.0f};
  }
  return RGB{1.0f, (1.0f - level) / 0.40f, 0.0f};
}

SoundReactiveRGB::RGB SoundReactiveRGB::hsv_to_rgb_(float hue, float saturation, float value) {
  hue = hue - std::floor(hue);
  const float scaled = hue * 6.0f;
  const int sector = static_cast<int>(scaled);
  const float fraction = scaled - static_cast<float>(sector);
  const float p = value * (1.0f - saturation);
  const float q = value * (1.0f - saturation * fraction);
  const float t = value * (1.0f - saturation * (1.0f - fraction));
  switch (sector % 6) {
    case 0:
      return RGB{value, t, p};
    case 1:
      return RGB{q, value, p};
    case 2:
      return RGB{p, value, t};
    case 3:
      return RGB{p, q, value};
    case 4:
      return RGB{t, p, value};
    default:
      return RGB{value, p, q};
  }
}

}  // namespace sound_reactive_rgb
}  // namespace esphome
