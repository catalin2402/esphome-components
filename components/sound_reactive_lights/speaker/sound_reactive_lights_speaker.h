#pragma once
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"
#include "esphome/components/speaker/speaker.h"
#include "esphome/components/light/light_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <atomic>
#include <string>

namespace esphome::sound_reactive_lights {
enum ReactiveMode : uint8_t {
  MODE_OFF = 0,
  MODE_VU = 1,
  MODE_SPECTRUM = 2,
  MODE_RAINBOW_PULSE = 3,
};

class SoundReactiveLightsSpeaker : public Component, public speaker::Speaker, public light::LightRemoteValuesListener {
 public:
  void set_output_speaker(speaker::Speaker *value) { this->output_ = value; }
  void set_light(light::LightState *value) { this->light_ = value; }
  void set_preference_key(uint32_t key) { this->preference_key_ = key; }
  uint8_t get_mode() const { return this->mode_; }
  void set_mode(uint8_t value) {
    if (value > MODE_RAINBOW_PULSE || value == this->mode_)
      return;
    this->mode_ = static_cast<ReactiveMode>(value);
    if (this->ready_)
      this->mode_preference_.save(&value);
    this->mode_callback_.call(value);
  }
  void add_mode_callback(std::function<void(uint8_t)> callback) { this->mode_callback_.add(std::move(callback)); }
  void set_sensitivity(float value);
  float get_sensitivity() const { return this->sensitivity_; }
  void set_max_brightness(float value);
  float get_max_brightness() const { return this->max_brightness_; }
  bool is_enabled() const { return this->enabled_.load(); }
  template<typename F> void add_enabled_callback(F &&callback) {
    this->enabled_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_sensitivity_callback(F &&callback) {
    this->sensitivity_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_max_brightness_callback(F &&callback) {
    this->max_brightness_callback_.add(std::forward<F>(callback));
  }
  void set_update_interval(uint32_t value) { this->update_interval_ = value; }
  void set_silence_timeout(uint32_t value) { this->silence_timeout_ = value; }
  void set_enabled(bool value);
  void next_mode() {
    this->set_mode((this->mode_ + 1) % (MODE_RAINBOW_PULSE + 1));
    this->set_enabled(true);
  }
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;
  void on_light_remote_values_update() override;
  size_t play(const uint8_t *data, size_t bytes) override { return this->play(data, bytes, 0); }
  size_t play(const uint8_t *data, size_t bytes, TickType_t wait) override;
  void start() override;
  void stop() override;
  void finish() override;
  bool has_buffered_data() const override { return this->output_->has_buffered_data(); }
  void set_pause_state(bool value) override { this->output_->set_pause_state(value); }
  bool get_pause_state() const override { return this->output_->get_pause_state(); }
  void set_volume(float value) override;
  float get_volume() override { return this->output_->get_volume(); }
  void set_mute_state(bool value) override;
  bool get_mute_state() override { return this->output_->get_mute_state(); }

 protected:
  struct Block {
    int16_t samples[256 * 2];
    uint32_t timestamp, rate;
    uint16_t frames;
    uint8_t channels;
    float gain;
  };
  void analyze_(const Block &block);
  void restore_();
  void set_enabled_(bool value, bool restore_light);
  speaker::Speaker *output_{nullptr};
  light::LightState *light_{nullptr};
  QueueHandle_t queue_{nullptr};
  std::atomic<bool> enabled_{true};
  std::atomic<bool> shutting_down_{false};
  std::atomic<float> analysis_gain_{1.0f};
  ReactiveMode mode_{MODE_SPECTRUM};
  ESPPreferenceObject mode_preference_;
  uint32_t preference_key_{0};
  LazyCallbackManager<void(uint8_t)> mode_callback_;
  LazyCallbackManager<void(bool)> enabled_callback_;
  LazyCallbackManager<void(float)> sensitivity_callback_;
  LazyCallbackManager<void(float)> max_brightness_callback_;
  float sensitivity_{4.0f}, max_brightness_{1.0f};
  uint32_t update_interval_{20}, silence_timeout_{250}, last_render_{0}, last_audio_{0};
  float low_[2]{}, mid_low_[2]{}, level_{0}, bass_{0}, mid_{0}, treble_{0};
  bool ready_{false}, own_light_call_{false};
  light::LightColorValues saved_values_;
  std::string saved_effect_{"None"};
};
}  // namespace esphome::sound_reactive_lights
