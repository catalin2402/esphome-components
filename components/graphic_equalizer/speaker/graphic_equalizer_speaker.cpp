#include "graphic_equalizer_speaker.h"

#include "esphome/core/log.h"
#include "freertos/task.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace esphome::graphic_equalizer {
static const char *const TAG = "graphic_equalizer";
static constexpr std::array<float, BAND_COUNT> FREQUENCIES{63.0f, 250.0f, 1000.0f, 4000.0f, 16000.0f};
static constexpr const char *PRESET_NAMES[]{"Flat", "Bass Boost", "Treble Boost", "Vocal",
                                            "Rock", "User 1",     "User 2",       "User 3"};
static constexpr float PI = 3.14159265358979323846f;
static constexpr float FILTER_Q = 2.0f / 3.0f;
static constexpr size_t BLOCK_FRAMES = 128;

GraphicEqualizerSpeaker::Settings GraphicEqualizerSpeaker::snapshot_() const {
  portENTER_CRITICAL(&this->settings_lock_);
  const auto settings = this->settings_;
  portEXIT_CRITICAL(&this->settings_lock_);
  return settings;
}

Curve GraphicEqualizerSpeaker::builtin_curve_(uint8_t preset) {
  switch (preset) {
    case PRESET_BASS_BOOST:
      return {{{6.0f, 3.0f, 0.0f, 0.0f, 0.0f}}, 0.0f};
    case PRESET_TREBLE_BOOST:
      return {{{0.0f, 0.0f, 0.0f, 3.0f, 6.0f}}, 0.0f};
    case PRESET_VOCAL:
      return {{{-3.0f, -2.0f, 3.0f, 2.0f, -2.0f}}, 0.0f};
    case PRESET_ROCK:
      return {{{4.0f, 2.0f, -2.0f, 3.0f, 4.0f}}, 0.0f};
    default:
      return {};
  }
}

bool GraphicEqualizerSpeaker::valid_curve_(const Curve &curve) {
  if (!std::isfinite(curve.preamp) || curve.preamp < -36.0f || curve.preamp > 0.0f)
    return false;
  for (float gain : curve.bands)
    if (!std::isfinite(gain) || gain < -12.0f || gain > 12.0f)
      return false;
  return true;
}

void GraphicEqualizerSpeaker::save_preferences_() {
  if (!this->ready_.load())
    return;
  StoredPresets stored{};
  stored.users = this->user_presets_;
  stored.selected = this->get_preset();
  this->preference_.save(&stored);
}

void GraphicEqualizerSpeaker::commit_(Settings settings, bool force_save) {
  const auto previous = this->snapshot_();
  const bool curve_changed =
      previous.curve.bands != settings.curve.bands || previous.curve.preamp != settings.curve.preamp;
  const bool preset_changed = previous.preset != settings.preset;
  const bool enabled_changed = previous.enabled != settings.enabled;
  if (!curve_changed && !preset_changed && !enabled_changed && previous.auto_headroom == settings.auto_headroom) {
    if (force_save)
      this->save_preferences_();
    return;
  }
  settings.revision = previous.revision + 1;
  portENTER_CRITICAL(&this->settings_lock_);
  this->settings_ = settings;
  portEXIT_CRITICAL(&this->settings_lock_);
  const bool user_changed = curve_changed && settings.preset >= PRESET_USER_1;
  if (user_changed)
    this->user_presets_[settings.preset - PRESET_USER_1] = settings.curve;
  if (force_save || preset_changed || user_changed)
    this->save_preferences_();
  for (uint8_t band = 0; band < BAND_COUNT; ++band)
    if (previous.curve.bands[band] != settings.curve.bands[band])
      this->gain_callback_.call(band, settings.curve.bands[band]);
  if (previous.curve.preamp != settings.curve.preamp)
    this->gain_callback_.call(BAND_COUNT, settings.curve.preamp);
  if (preset_changed)
    this->preset_callback_.call(settings.preset);
  if (enabled_changed)
    this->enabled_callback_.call(settings.enabled);
}

void GraphicEqualizerSpeaker::select_preset(uint8_t preset) {
  if (preset >= PRESET_COUNT)
    return;
  auto settings = this->snapshot_();
  settings.preset = preset;
  settings.curve = preset >= PRESET_USER_1 ? this->user_presets_[preset - PRESET_USER_1] : builtin_curve_(preset);
  this->commit_(settings);
}
void GraphicEqualizerSpeaker::save_preset(uint8_t slot) {
  if (slot < 1 || slot > USER_PRESET_COUNT)
    return;
  auto settings = this->snapshot_();
  this->user_presets_[slot - 1] = settings.curve;
  settings.preset = PRESET_USER_1 + slot - 1;
  this->commit_(settings, true);
}
void GraphicEqualizerSpeaker::set_band_gain(uint8_t band, float gain) {
  if (band >= BAND_COUNT || !std::isfinite(gain))
    return;
  auto settings = this->snapshot_();
  settings.curve.bands[band] = std::clamp(gain, -12.0f, 12.0f);
  this->commit_(settings);
}
float GraphicEqualizerSpeaker::get_band_gain(uint8_t band) const {
  return band < BAND_COUNT ? this->snapshot_().curve.bands[band] : 0.0f;
}
void GraphicEqualizerSpeaker::set_preamp(float gain) {
  if (!std::isfinite(gain))
    return;
  auto settings = this->snapshot_();
  settings.curve.preamp = std::clamp(gain, -36.0f, 0.0f);
  this->commit_(settings);
}
float GraphicEqualizerSpeaker::get_preamp() const { return this->snapshot_().curve.preamp; }
void GraphicEqualizerSpeaker::set_enabled(bool enabled) {
  auto settings = this->snapshot_();
  settings.enabled = enabled;
  this->commit_(settings);
}
bool GraphicEqualizerSpeaker::is_enabled() const { return this->snapshot_().enabled; }
void GraphicEqualizerSpeaker::set_auto_headroom(bool enabled) {
  auto settings = this->snapshot_();
  settings.auto_headroom = enabled;
  this->commit_(settings);
}
uint8_t GraphicEqualizerSpeaker::get_preset() const { return this->snapshot_().preset; }

void GraphicEqualizerSpeaker::setup() {
  if (this->output_ == nullptr) {
    ESP_LOGE(TAG, "An output speaker is required");
    this->mark_failed();
    return;
  }
  this->preference_ = global_preferences->make_preference<StoredPresets>(this->preference_key_);
  StoredPresets stored{};
  if (this->preference_.load(&stored)) {
    bool valid = stored.version == 1 && stored.selected < PRESET_COUNT;
    for (const auto &curve : stored.users)
      valid = valid && valid_curve_(curve);
    if (valid) {
      this->user_presets_ = stored.users;
      this->select_preset(stored.selected);
    } else {
      ESP_LOGW(TAG, "Ignoring invalid saved equalizer presets");
    }
  }
  this->output_->add_audio_output_callback(
      [this](uint32_t frames, int64_t time) { this->audio_output_callback_.call(frames, time); });
  this->ready_.store(true);
}
void GraphicEqualizerSpeaker::loop() {
  this->state_ = this->output_->is_running()   ? speaker::STATE_RUNNING
                 : this->output_->is_stopped() ? speaker::STATE_STOPPED
                                               : this->state_;
  const bool error = this->format_error_.load();
  if (error != this->reported_format_error_) {
    this->reported_format_error_ = error;
    if (error) {
      ESP_LOGW(TAG, "Expected 16-bit mono/stereo PCM at 44100 or 48000 Hz");
      this->status_set_warning("Unsupported PCM format");
    } else {
      this->status_clear_warning();
    }
  }
}
void GraphicEqualizerSpeaker::start() {
  if (this->shutting_down_.load())
    return;
  this->reset_generation_.fetch_add(1);
  this->output_->set_audio_stream_info(this->audio_stream_info_);
  this->output_->start();
  this->state_ = speaker::STATE_STARTING;
}
void GraphicEqualizerSpeaker::stop() {
  this->reset_generation_.fetch_add(1);
  this->output_->stop();
  this->state_ = speaker::STATE_STOPPING;
}
void GraphicEqualizerSpeaker::finish() {
  this->output_->finish();
  this->state_ = speaker::STATE_STOPPING;
}
void GraphicEqualizerSpeaker::set_volume(float volume) {
  this->volume_ = volume;
  this->output_->set_volume(volume);
}
void GraphicEqualizerSpeaker::set_mute_state(bool mute) {
  this->mute_state_ = mute;
  this->output_->set_mute_state(mute);
}
void GraphicEqualizerSpeaker::on_shutdown() {
  this->save_preferences_();
  this->shutting_down_.store(true);
  this->ready_.store(false);
  if (this->output_ != nullptr)
    this->stop();
}
void GraphicEqualizerSpeaker::dump_config() {
  const auto settings = this->snapshot_();
  ESP_LOGCONFIG(TAG, "Graphic Equalizer:");
  ESP_LOGCONFIG(TAG, "  Bands: 63 Hz, 250 Hz, 1 kHz, 4 kHz, 16 kHz");
  ESP_LOGCONFIG(TAG, "  Preset: %s", PRESET_NAMES[settings.preset]);
  ESP_LOGCONFIG(TAG, "  Enabled: %s", YESNO(settings.enabled));
  ESP_LOGCONFIG(TAG, "  Automatic headroom: %s", YESNO(settings.auto_headroom));
  ESP_LOGCONFIG(TAG, "  Gains: %.1f / %.1f / %.1f / %.1f / %.1f dB", settings.curve.bands[0], settings.curve.bands[1],
                settings.curve.bands[2], settings.curve.bands[3], settings.curve.bands[4]);
  ESP_LOGCONFIG(TAG, "  Preamp: %.1f dB", settings.curve.preamp);
}

GraphicEqualizerSpeaker::FilterBank GraphicEqualizerSpeaker::make_bank_(const Settings &settings,
                                                                        uint32_t sample_rate) {
  FilterBank bank{};
  if (!settings.enabled)
    return bank;
  float preamp = settings.curve.preamp;
  if (settings.auto_headroom)
    for (float gain : settings.curve.bands)
      preamp -= std::max(gain, 0.0f);
  bank.gain = std::pow(10.0f, preamp / 20.0f);
  bank.bypass = preamp == 0.0f;
  for (size_t band = 0; band < BAND_COUNT; ++band) {
    const float gain = settings.curve.bands[band];
    if (gain == 0.0f)
      continue;
    bank.bypass = false;
    // Peaking EQ coefficients: https://www.w3.org/TR/audio-eq-cookbook/
    const float amplitude = std::pow(10.0f, gain / 40.0f);
    const float omega = 2.0f * PI * FREQUENCIES[band] / sample_rate;
    const float alpha = std::sin(omega) / (2.0f * FILTER_Q);
    const float cosine = -2.0f * std::cos(omega);
    const float a0 = 1.0f + alpha / amplitude;
    bank.coefficients[band] = {(1.0f + alpha * amplitude) / a0, cosine / a0,
                               (1.0f - alpha * amplitude) / a0, cosine / a0,
                               (1.0f - alpha / amplitude) / a0, true};
  }
  return bank;
}
float GraphicEqualizerSpeaker::filter_sample_(FilterBank &bank, uint8_t channel, float sample) {
  if (bank.bypass)
    return sample;
  sample *= bank.gain;
  for (size_t band = 0; band < BAND_COUNT; ++band) {
    const auto &c = bank.coefficients[band];
    if (!c.active)
      continue;
    auto &state = bank.channels[channel][band];
    const float output = c.b0 * sample + state.z1;
    state.z1 = c.b1 * sample - c.a1 * output + state.z2;
    state.z2 = c.b2 * sample - c.a2 * output;
    sample = output;
  }
  return sample;
}
void GraphicEqualizerSpeaker::prepare_processing_(const Settings &settings, uint32_t sample_rate, uint8_t channels) {
  const uint32_t generation = this->reset_generation_.load();
  auto &state = this->processing_;
  if (!state.initialized || state.reset_generation != generation || state.sample_rate != sample_rate ||
      state.channels != channels) {
    state = {};
    state.current = make_bank_(settings, sample_rate);
    state.sample_rate = sample_rate;
    state.channels = channels;
    state.reset_generation = generation;
    state.revision = settings.revision;
    state.initialized = true;
    this->pending_offset_ = this->pending_size_ = 0;
  } else if (state.fade_frames == 0 && state.revision != settings.revision) {
    state.next = make_bank_(settings, sample_rate);
    state.next_revision = settings.revision;
    if (state.current.bypass && state.next.bypass) {
      state.current = state.next;
      state.revision = settings.revision;
      return;
    }
    state.fade_frames = sample_rate / 50;  // 20 ms crossfade; coefficients stay fixed within each bank.
    state.fade_position = 0;
  }
}
void GraphicEqualizerSpeaker::process_frames_(const uint8_t *data, int16_t *output, size_t frames, uint8_t channels) {
  auto &state = this->processing_;
  for (size_t frame = 0; frame < frames; ++frame) {
    const float mix = state.fade_frames ? float(state.fade_position + 1) / state.fade_frames : 0.0f;
    for (uint8_t channel = 0; channel < channels; ++channel) {
      int16_t input;
      std::memcpy(&input, data + (frame * channels + channel) * sizeof(input), sizeof(input));
      float sample = filter_sample_(state.current, channel, input);
      if (state.fade_frames)
        sample = sample * (1.0f - mix) + filter_sample_(state.next, channel, input) * mix;
      if (!std::isfinite(sample))
        sample = 0.0f;
      output[frame * channels + channel] = static_cast<int16_t>(std::lround(std::clamp(sample, -32768.0f, 32767.0f)));
    }
    if (state.fade_frames && ++state.fade_position >= state.fade_frames) {
      state.current = state.next;
      state.revision = state.next_revision;
      state.fade_frames = state.fade_position = 0;
    }
  }
}

size_t GraphicEqualizerSpeaker::play(const uint8_t *data, size_t length, TickType_t ticks_to_wait) {
  if (!this->ready_.load() || this->shutting_down_.load() || !length)
    return 0;
  const auto info = this->audio_stream_info_;
  const uint8_t channels = info.get_channels();
  const uint32_t rate = info.get_sample_rate();
  if (info.get_bits_per_sample() != 16 || channels < 1 || channels > 2 || (rate != 44100 && rate != 48000)) {
    this->format_error_.store(true);
    return 0;
  }
  this->format_error_.store(false);
  this->output_->set_audio_stream_info(info);
  if (this->output_->is_stopped())
    this->output_->start();
  const size_t frame_bytes = channels * sizeof(int16_t);
  const TickType_t began = xTaskGetTickCount();
  const auto remaining_wait = [began, ticks_to_wait]() -> TickType_t {
    if (ticks_to_wait == portMAX_DELAY)
      return portMAX_DELAY;
    const TickType_t elapsed = xTaskGetTickCount() - began;
    return elapsed >= ticks_to_wait ? 0 : ticks_to_wait - elapsed;
  };
  const uint32_t generation = this->reset_generation_.load();
  size_t consumed = 0;
  int16_t output[BLOCK_FRAMES * 2];
  this->prepare_processing_(this->snapshot_(), rate, channels);
  // An output speaker may accept only part of a PCM frame. The caller retries
  // the same input bytes; send the already filtered tail instead of filtering twice.
  if (this->reset_generation_.load() != generation)
    return 0;
  if (this->pending_size_) {
    const size_t requested = std::min(length, this->pending_size_ - this->pending_offset_);
    const size_t written = std::min(requested, this->output_->play(this->pending_frame_.data() + this->pending_offset_,
                                                                   requested, remaining_wait()));
    this->pending_offset_ += written;
    consumed += written;
    if (this->pending_offset_ != this->pending_size_)
      return consumed;
    this->pending_offset_ = this->pending_size_ = 0;
  }
  while (length - consumed >= frame_bytes && !this->shutting_down_.load() &&
         this->reset_generation_.load() == generation) {
    this->prepare_processing_(this->snapshot_(), rate, channels);
    const size_t frames = std::min(BLOCK_FRAMES, (length - consumed) / frame_bytes);
    const auto before = this->processing_;
    const size_t requested = frames * frame_bytes;
    if (this->processing_.fade_frames == 0 && this->processing_.current.bypass)
      std::memcpy(output, data + consumed, requested);
    else
      this->process_frames_(data + consumed, output, frames, channels);
    if (this->reset_generation_.load() != generation)
      return consumed;
    const size_t written = std::min(
        requested, this->output_->play(reinterpret_cast<const uint8_t *>(output), requested, remaining_wait()));
    if (written < requested) {
      const size_t partial = written % frame_bytes;
      if (partial) {
        const auto *bytes = reinterpret_cast<const uint8_t *>(output);
        this->pending_size_ = frame_bytes - partial;
        this->pending_offset_ = 0;
        std::memcpy(this->pending_frame_.data(), bytes + written, this->pending_size_);
      }
      // Only advance filter and fade state for frames accepted by the output.
      this->processing_ = before;
      const size_t committed_frames = written / frame_bytes + (partial != 0);
      this->process_frames_(data + consumed, output, committed_frames, channels);
      return consumed + written;
    }
    consumed += written;
  }
  return consumed;
}
}  // namespace esphome::graphic_equalizer
