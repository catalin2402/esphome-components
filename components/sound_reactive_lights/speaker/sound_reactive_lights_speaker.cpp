#include "sound_reactive_lights_speaker.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cinttypes>

namespace esphome::sound_reactive_lights {
static const char *const TAG = "sound_reactive_lights";
void SoundReactiveLightsSpeaker::setup() {
  this->queue_ = xQueueCreate(4, sizeof(Block));
  if (!this->queue_) {
    ESP_LOGE(TAG, "Could not allocate the audio analysis queue");
    this->mark_failed();
    return;
  }
  this->mode_preference_ = global_preferences->make_preference<uint8_t>(this->preference_key_);
  uint8_t restored_mode;
  if (this->mode_preference_.load(&restored_mode) && restored_mode <= MODE_RAINBOW_PULSE)
    this->set_mode(restored_mode);
  this->light_->add_remote_values_listener(this);
  this->output_->add_audio_output_callback(
      [this](uint32_t frames, int64_t time) { this->audio_output_callback_.call(frames, time); });
  this->saved_values_ = this->light_->remote_values;
  this->saved_effect_ = this->light_->get_effect_name().str();
  this->ready_ = true;
}
void SoundReactiveLightsSpeaker::set_enabled(bool value) { this->set_enabled_(value, true); }
void SoundReactiveLightsSpeaker::set_enabled_(bool value, bool restore_light) {
  const bool previous = this->enabled_.exchange(value);
  if (previous == value)
    return;
  if (!this->ready_) {
    this->enabled_callback_.call(value);
    return;
  }
  if (value) {
    this->saved_values_ = this->light_->remote_values;
    this->saved_effect_ = this->light_->get_effect_name().str();
    this->level_ = this->bass_ = this->mid_ = this->treble_ = 0;
  } else if (restore_light) {
    this->restore_();
  }
  this->enabled_callback_.call(value);
}
void SoundReactiveLightsSpeaker::set_sensitivity(float value) {
  if (!std::isfinite(value))
    return;
  value = std::clamp(value, 0.1f, 100.0f);
  if (this->sensitivity_ == value)
    return;
  this->sensitivity_ = value;
  this->sensitivity_callback_.call(value);
}
void SoundReactiveLightsSpeaker::set_max_brightness(float value) {
  if (!std::isfinite(value))
    return;
  value = std::clamp(value, 0.0f, 1.0f);
  if (this->max_brightness_ == value)
    return;
  this->max_brightness_ = value;
  this->max_brightness_callback_.call(value);
}
void SoundReactiveLightsSpeaker::restore_() {
  this->own_light_call_ = true;
  auto call = this->light_->make_call();
  call.from_light_color_values(this->saved_values_);
  call.set_effect(this->saved_effect_).set_transition_length_if_supported(0).set_save(false).perform();
  this->own_light_call_ = false;
}
void SoundReactiveLightsSpeaker::on_light_remote_values_update() {
  if (!this->own_light_call_ && this->enabled_.load()) {
    // An explicit user off or another effect takes ownership; don't fight it.
    if (!this->light_->remote_values.is_on() || this->light_->get_effect_name() != "None")
      this->set_enabled_(false, false);
  }
}
void SoundReactiveLightsSpeaker::start() {
  if (this->shutting_down_.load())
    return;
  this->output_->set_audio_stream_info(this->audio_stream_info_);
  this->output_->start();
  this->state_ = speaker::STATE_STARTING;
}
void SoundReactiveLightsSpeaker::stop() {
  this->output_->stop();
  this->state_ = speaker::STATE_STOPPING;
  if (this->queue_)
    xQueueReset(this->queue_);
}
void SoundReactiveLightsSpeaker::finish() {
  this->output_->finish();
  this->state_ = speaker::STATE_STOPPING;
}
void SoundReactiveLightsSpeaker::set_volume(float value) {
  this->volume_ = value;
  this->output_->set_volume(value);
}
void SoundReactiveLightsSpeaker::set_mute_state(bool value) {
  this->mute_state_ = value;
  this->output_->set_mute_state(value);
}
size_t SoundReactiveLightsSpeaker::play(const uint8_t *data, size_t bytes, TickType_t wait) {
  if (this->shutting_down_.load())
    return 0;
  this->output_->set_audio_stream_info(this->audio_stream_info_);
  const size_t accepted = this->output_->play(data, bytes, wait);
  const auto info = this->audio_stream_info_;
  const size_t channels = info.get_channels(), frame_bytes = channels * 2;
  if (!this->queue_ || !this->enabled_.load() || info.get_bits_per_sample() != 16 || channels < 1 || channels > 2)
    return accepted;
  // Observe accepted frames only; the producer retries unaccepted bytes.
  // Bound analysis copying to four blocks per call, even for a large producer.
  size_t offset = 0;
  for (unsigned n = 0; n < 4 && offset + frame_bytes <= accepted; ++n) {
    Block block{};
    block.frames = std::min<size_t>(256, (accepted - offset) / frame_bytes);
    block.channels = channels;
    block.rate = info.get_sample_rate();
    block.timestamp = millis();
    block.gain = this->analysis_gain_.load();
    std::memcpy(block.samples, data + offset, block.frames * frame_bytes);
    if (xQueueSend(this->queue_, &block, 0) != pdTRUE)
      break;
    offset += block.frames * frame_bytes;
  }
  return accepted;
}
void SoundReactiveLightsSpeaker::analyze_(const Block &block) {
  if (millis() - block.timestamp > this->silence_timeout_ || !block.rate)
    return;
  const float a = 1.0f - std::exp(-6.2831853f * 200.0f / block.rate);
  const float b = 1.0f - std::exp(-6.2831853f * 2000.0f / block.rate);
  float total = 0, bass = 0, mid = 0, treble = 0;
  for (size_t frame = 0; frame < block.frames; ++frame) {
    for (size_t channel = 0; channel < block.channels; ++channel) {
      const float value = block.samples[frame * block.channels + channel] * (block.gain / 32768.0f);
      this->low_[channel] += a * (value - this->low_[channel]);
      this->mid_low_[channel] += b * (value - this->mid_low_[channel]);
      const float m = this->mid_low_[channel] - this->low_[channel];
      const float t = value - this->mid_low_[channel];
      total += value * value;
      bass += this->low_[channel] * this->low_[channel];
      mid += m * m;
      treble += t * t;
    }
  }
  const float count = std::max<size_t>(1, block.frames * block.channels);
  auto smooth = [this, count](float previous, float energy) {
    const float target = std::clamp((std::sqrt(energy / count) - 0.002f) * this->sensitivity_, 0.0f, 1.0f);
    const float retain = target > previous ? 0.28f : 0.80f;
    return previous * retain + target * (1 - retain);
  };
  this->level_ = smooth(this->level_, total);
  this->bass_ = smooth(this->bass_, bass);
  this->mid_ = smooth(this->mid_, mid);
  this->treble_ = smooth(this->treble_, treble);
  this->last_audio_ = block.timestamp;
}
void SoundReactiveLightsSpeaker::loop() {
  if (this->queue_ == nullptr || this->shutting_down_.load())
    return;
  this->state_ = this->output_->is_running()   ? speaker::STATE_RUNNING
                 : this->output_->is_stopped() ? speaker::STATE_STOPPED
                                               : this->state_;
  this->analysis_gain_.store(this->output_->get_mute_state() ? 0.0f : this->output_->get_volume());
  Block block;
  for (unsigned n = 0; n < 4 && xQueueReceive(this->queue_, &block, 0) == pdTRUE; ++n)
    if (this->enabled_.load())
      this->analyze_(block);
  const uint32_t now = millis();
  if (!this->enabled_.load() || now - this->last_render_ < this->update_interval_)
    return;
  this->last_render_ = now;
  if (now - this->last_audio_ > this->silence_timeout_)
    this->level_ = this->bass_ = this->mid_ = this->treble_ = 0;
  float red = 0, green = 0, blue = 0;
  float brightness = this->mode_ == MODE_OFF ? 0 : this->level_ * this->max_brightness_;
  if (this->mode_ == MODE_VU) {
    red = this->level_ < 0.6f ? this->level_ / 0.6f : 1;
    green = this->level_ < 0.6f ? 1 : (1 - this->level_) / 0.4f;
  } else if (this->mode_ == MODE_SPECTRUM) {
    const float peak = std::max({this->bass_, this->mid_, this->treble_, 0.001f});
    red = this->bass_ / peak;
    green = this->mid_ / peak;
    blue = this->treble_ / peak;
  } else if (this->mode_ == MODE_RAINBOW_PULSE) {
    const float hue = float(now % 12000) / 12000.0f;
    red = std::clamp(std::fabs(hue * 6 - 3) - 1, 0.0f, 1.0f);
    green = std::clamp(2 - std::fabs(hue * 6 - 2), 0.0f, 1.0f);
    blue = std::clamp(2 - std::fabs(hue * 6 - 4), 0.0f, 1.0f);
  }
  this->own_light_call_ = true;
  auto call = this->light_->make_call();
  call.set_state(brightness > 0.001f).set_brightness_if_supported(brightness);
  call.set_red_if_supported(red).set_green_if_supported(green).set_blue_if_supported(blue);
  if (this->light_->get_traits().supports_color_capability(light::ColorCapability::RGB))
    call.set_white_if_supported(0).set_color_brightness_if_supported(1.0f);
  call.set_effect(static_cast<uint32_t>(0))
      .set_transition_length_if_supported(0)
      .set_publish(false)
      .set_save(false)
      .perform();
  this->own_light_call_ = false;
}
void SoundReactiveLightsSpeaker::on_shutdown() {
  this->shutting_down_.store(true);
  this->set_enabled(false);
  this->ready_ = false;
}
void SoundReactiveLightsSpeaker::dump_config() {
  static const char *const MODES[] = {"Off", "VU", "Spectrum", "Rainbow Pulse"};
  ESP_LOGCONFIG(TAG, "Sound Reactive Lights Speaker:");
  ESP_LOGCONFIG(TAG, "  Mode: %s", MODES[this->mode_]);
  ESP_LOGCONFIG(TAG, "  Enabled: %s", YESNO(this->enabled_.load()));
  ESP_LOGCONFIG(TAG, "  Sensitivity: %.1f", this->sensitivity_);
  ESP_LOGCONFIG(TAG, "  Maximum brightness: %.0f%%", this->max_brightness_ * 100.0f);
  ESP_LOGCONFIG(TAG, "  Update interval: %" PRIu32 " ms", this->update_interval_);
  ESP_LOGCONFIG(TAG, "  Silence timeout: %" PRIu32 " ms", this->silence_timeout_);
}
}  // namespace esphome::sound_reactive_lights
