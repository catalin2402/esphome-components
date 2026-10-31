#pragma once

#include "esphome/components/speaker/speaker.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"
#include "freertos/FreeRTOS.h"
#include <array>
#include <atomic>

namespace esphome::graphic_equalizer {

static constexpr size_t BAND_COUNT = 5;
static constexpr size_t USER_PRESET_COUNT = 3;
enum Preset : uint8_t {
  PRESET_FLAT,
  PRESET_BASS_BOOST,
  PRESET_TREBLE_BOOST,
  PRESET_VOCAL,
  PRESET_ROCK,
  PRESET_USER_1,
  PRESET_USER_2,
  PRESET_USER_3,
  PRESET_COUNT,
};

struct Curve {
  std::array<float, BAND_COUNT> bands{};
  float preamp{0.0f};
};

class GraphicEqualizerSpeaker : public Component, public speaker::Speaker {
 public:
  void set_output_speaker(speaker::Speaker *output) { this->output_ = output; }
  void set_preference_key(uint32_t key) { this->preference_key_ = key; }
  void select_preset(uint8_t preset);
  void save_preset(uint8_t slot);
  void reset() { this->select_preset(PRESET_FLAT); }
  void set_band_gain(uint8_t band, float gain);
  float get_band_gain(uint8_t band) const;
  void set_preamp(float gain);
  float get_preamp() const;
  void set_enabled(bool enabled);
  bool is_enabled() const;
  void set_auto_headroom(bool enabled);
  uint8_t get_preset() const;
  template<typename F> void add_gain_callback(F &&callback) { this->gain_callback_.add(std::forward<F>(callback)); }
  template<typename F> void add_preset_callback(F &&callback) { this->preset_callback_.add(std::forward<F>(callback)); }
  template<typename F> void add_enabled_callback(F &&callback) {
    this->enabled_callback_.add(std::forward<F>(callback));
  }
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR - 1.0f; }
  size_t play(const uint8_t *data, size_t length) override { return this->play(data, length, 0); }
  size_t play(const uint8_t *data, size_t length, TickType_t ticks_to_wait) override;
  void start() override;
  void stop() override;
  void finish() override;
  bool has_buffered_data() const override { return this->output_->has_buffered_data(); }
  void set_pause_state(bool pause) override { this->output_->set_pause_state(pause); }
  bool get_pause_state() const override { return this->output_->get_pause_state(); }
  void set_volume(float volume) override;
  float get_volume() override { return this->output_->get_volume(); }
  void set_mute_state(bool mute) override;
  bool get_mute_state() override { return this->output_->get_mute_state(); }

 protected:
  struct Settings {
    Curve curve{};
    uint32_t revision{0};
    uint8_t preset{PRESET_FLAT};
    bool enabled{true};
    bool auto_headroom{true};
  };
  struct StoredPresets {
    uint32_t version{1};
    std::array<Curve, USER_PRESET_COUNT> users{};
    uint8_t selected{PRESET_FLAT};
    std::array<uint8_t, 3> reserved{};
  };
  struct Coefficients {
    float b0{1.0f}, b1{0.0f}, b2{0.0f}, a1{0.0f}, a2{0.0f};
    bool active{false};
  };
  struct FilterState {
    float z1{0.0f}, z2{0.0f};
  };
  struct FilterBank {
    std::array<Coefficients, BAND_COUNT> coefficients{};
    std::array<std::array<FilterState, BAND_COUNT>, 2> channels{};
    float gain{1.0f};
    bool bypass{true};
  };
  struct ProcessingState {
    FilterBank current{}, next{};
    uint32_t revision{0}, next_revision{0}, reset_generation{0}, sample_rate{0};
    uint32_t fade_position{0}, fade_frames{0};
    uint8_t channels{0};
    bool initialized{false};
  };
  Settings snapshot_() const;
  void commit_(Settings settings, bool force_save = false);
  void save_preferences_();
  static bool valid_curve_(const Curve &curve);
  static Curve builtin_curve_(uint8_t preset);
  static FilterBank make_bank_(const Settings &settings, uint32_t sample_rate);
  static float filter_sample_(FilterBank &bank, uint8_t channel, float sample);
  void prepare_processing_(const Settings &settings, uint32_t sample_rate, uint8_t channels);
  void process_frames_(const uint8_t *data, int16_t *output, size_t frames, uint8_t channels);
  speaker::Speaker *output_{nullptr};
  mutable portMUX_TYPE settings_lock_ = portMUX_INITIALIZER_UNLOCKED;
  Settings settings_{};
  std::array<Curve, USER_PRESET_COUNT> user_presets_{};  // Main-loop ownership.
  ESPPreferenceObject preference_;
  uint32_t preference_key_{0};
  LazyCallbackManager<void(uint8_t, float)> gain_callback_;
  LazyCallbackManager<void(uint8_t)> preset_callback_;
  LazyCallbackManager<void(bool)> enabled_callback_;
  std::atomic<bool> ready_{false}, shutting_down_{false}, format_error_{false};
  std::atomic<uint32_t> reset_generation_{0};
  bool reported_format_error_{false};
  // Only the audio producer touches processing state and the partial-frame tail.
  ProcessingState processing_{};
  std::array<uint8_t, 4> pending_frame_{};
  size_t pending_offset_{0}, pending_size_{0};
};
}  // namespace esphome::graphic_equalizer
