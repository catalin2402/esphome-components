#include "usb_audio_bridge.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>

#include "esphome/core/log.h"
#include "freertos/FreeRTOS.h"
#include "tusb.h"
#include "usb_device_uac.h"

namespace esphome {
namespace usb_audio_bridge {

static const char *const TAG = "usb_audio_bridge";

void USBAudioBridge::setup() {
  ESP_LOGCONFIG(TAG, "Setting up USB Audio playback to I2S bridge...");

  esp_err_t result = this->initialize_i2s_();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "I2S initialization failed: %s", esp_err_to_name(result));
    this->mark_failed();
    return;
  }

  uac_device_config_t config{};
  config.skip_tinyusb_init = false;
  config.output_cb = USBAudioBridge::speaker_output_cb_;
  config.input_cb = nullptr;
  config.set_mute_cb = USBAudioBridge::mute_cb_;
  config.set_volume_cb = USBAudioBridge::volume_cb_;
  config.cb_ctx = this;

  result = uac_device_init(&config);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "USB Audio initialization failed: %s", esp_err_to_name(result));
    this->mark_failed();
    return;
  }

#ifdef USE_OTA_STATE_LISTENER
  ota::get_global_ota_callback()->add_global_state_listener(this);
#endif

  ESP_LOGI(TAG, "USB stereo playback is ready");
}

void USBAudioBridge::loop() { this->process_media_keys_(); }

void USBAudioBridge::on_shutdown() { this->suspend_playback_(); }

#ifdef USE_OTA_STATE_LISTENER
void USBAudioBridge::on_ota_global_state(ota::OTAState state, float progress, uint8_t error,
                                         ota::OTAComponent *component) {
  if (state == ota::OTA_STARTED) {
    ESP_LOGI(TAG, "Suspending USB audio playback for OTA update");
    this->suspend_playback_();
  } else if (state == ota::OTA_ERROR || state == ota::OTA_ABORT) {
    this->resume_playback_();
  }
}
#endif

void USBAudioBridge::dump_config() {
  ESP_LOGCONFIG(TAG, "USB Audio Bridge:");
  ESP_LOGCONFIG(TAG, "  Sample rate: %" PRIu32 " Hz", this->sample_rate_);
  ESP_LOGCONFIG(TAG, "  USB playback: stereo, signed 16-bit PCM");
  ESP_LOGCONFIG(TAG, "  I2S slots: stereo, 32-bit, Philips format");
  ESP_LOGCONFIG(TAG, "  Maximum output gain: %.1f dB", this->output_gain_db_);
  ESP_LOGCONFIG(TAG, "  BCLK pin: GPIO%d", this->bclk_pin_);
  ESP_LOGCONFIG(TAG, "  LRCLK pin: GPIO%d", this->lrclk_pin_);
  ESP_LOGCONFIG(TAG, "  PCM5102A DIN pin: GPIO%d", this->dac_dout_pin_);
}

void USBAudioBridge::set_output_gain_db(float output_gain_db) {
  this->output_gain_db_ = output_gain_db;
  this->update_playback_gain_();
}

void USBAudioBridge::send_media_key(MediaKey key) {
  if (!tud_mounted()) {
    ESP_LOGD(TAG, "Ignoring media key while USB is not mounted");
    return;
  }

  if (this->media_key_queue_count_ >= MEDIA_KEY_QUEUE_SIZE) {
    ESP_LOGW(TAG, "USB media-key queue is full; dropping key press");
    return;
  }

  this->media_key_queue_[this->media_key_queue_tail_] = key;
  this->media_key_queue_tail_ = (this->media_key_queue_tail_ + 1) % MEDIA_KEY_QUEUE_SIZE;
  this->media_key_queue_count_++;
}

void USBAudioBridge::process_media_keys_() {
  if (!tud_mounted()) {
    this->clear_media_key_queue_();
    return;
  }
  if (!tud_hid_ready()) {
    return;
  }

  if (this->media_key_pressed_) {
    const uint16_t released = 0;
    if (tud_hid_report(0, &released, sizeof(released))) {
      this->media_key_pressed_ = false;
    }
    return;
  }

  if (this->media_key_queue_count_ == 0) {
    return;
  }

  const uint16_t usage = static_cast<uint16_t>(this->media_key_queue_[this->media_key_queue_head_]);
  if (!tud_hid_report(0, &usage, sizeof(usage))) {
    return;
  }

  this->media_key_queue_head_ = (this->media_key_queue_head_ + 1) % MEDIA_KEY_QUEUE_SIZE;
  this->media_key_queue_count_--;
  this->media_key_pressed_ = true;
}

void USBAudioBridge::clear_media_key_queue_() {
  this->media_key_queue_head_ = 0;
  this->media_key_queue_tail_ = 0;
  this->media_key_queue_count_ = 0;
  this->media_key_pressed_ = false;
}

void USBAudioBridge::update_playback_gain_() {
  // Take a value snapshot first: std::min() accepts const references and
  // cannot bind them directly to a volatile field shared with the USB task.
  const uint32_t current_volume = this->volume_percent_;
  const uint32_t volume = std::min<uint32_t>(current_volume, 100U);
  if (volume == 0) {
    this->playback_gain_q16_ = 0;
    return;
  }

  // usb_device_uac maps its advertised -50..0 dB control range onto
  // 0..100 before invoking the callback. Convert it back to decibels and
  // apply a perceptually correct amplitude gain instead of treating the
  // percentage as a linear PCM multiplier.
  const float usb_gain_db = static_cast<float>(volume) * 0.5f - 50.0f;
  const float total_gain_db = std::min(0.0f, usb_gain_db + this->output_gain_db_);
  const float linear_gain = std::pow(10.0f, total_gain_db / 20.0f);
  this->playback_gain_q16_ = static_cast<uint32_t>(std::lround(linear_gain * 65536.0f));
}

esp_err_t USBAudioBridge::initialize_i2s_() {
  i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  esp_err_t result = i2s_new_channel(&channel_config, &this->tx_handle_, nullptr);
  if (result != ESP_OK) {
    return result;
  }

  i2s_std_config_t standard_config = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(this->sample_rate_),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg = {
          .mclk = I2S_GPIO_UNUSED,
          .bclk = static_cast<gpio_num_t>(this->bclk_pin_),
          .ws = static_cast<gpio_num_t>(this->lrclk_pin_),
          .dout = static_cast<gpio_num_t>(this->dac_dout_pin_),
          .din = I2S_GPIO_UNUSED,
          .invert_flags = {
              .mclk_inv = false,
              .bclk_inv = false,
              .ws_inv = false,
          },
      },
  };

  result = i2s_channel_init_std_mode(this->tx_handle_, &standard_config);
  if (result != ESP_OK) {
    return result;
  }
  return i2s_channel_enable(this->tx_handle_);
}

void USBAudioBridge::suspend_playback_() {
  if (this->playback_suspended_) {
    return;
  }

  this->playback_suspended_ = true;
  if (this->tx_handle_ == nullptr) {
    return;
  }

  const esp_err_t result = i2s_channel_disable(this->tx_handle_);
  if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
    ESP_LOGW(TAG, "Unable to disable I2S playback: %s", esp_err_to_name(result));
  }
}

void USBAudioBridge::resume_playback_() {
  if (!this->playback_suspended_ || this->tx_handle_ == nullptr) {
    return;
  }

  const esp_err_t result = i2s_channel_enable(this->tx_handle_);
  if (result != ESP_OK) {
    ESP_LOGW(TAG, "Unable to resume I2S playback after OTA: %s", esp_err_to_name(result));
    return;
  }
  this->playback_suspended_ = false;
  ESP_LOGI(TAG, "USB audio playback resumed after OTA cancellation");
}

esp_err_t USBAudioBridge::write_speaker_(uint8_t *buffer, size_t length) {
  if ((length % (sizeof(int16_t) * 2)) != 0) {
    return ESP_ERR_INVALID_SIZE;
  }
  if (this->playback_suspended_) {
    return ESP_OK;
  }

  const int16_t *usb_pcm = reinterpret_cast<const int16_t *>(buffer);
  size_t frames_remaining = length / (sizeof(int16_t) * 2);
  size_t source_frame = 0;
  const bool muted = this->muted_;
  const uint32_t playback_gain_q16 = this->playback_gain_q16_;

  for (auto *listener : this->audio_listeners_) {
    listener->on_audio_frames(usb_pcm, frames_remaining, playback_gain_q16, muted);
  }

  while (frames_remaining > 0) {
    const size_t frames = std::min(frames_remaining, CONVERSION_FRAMES);

    for (size_t frame = 0; frame < frames; ++frame) {
      int32_t left = usb_pcm[(source_frame + frame) * 2];
      int32_t right = usb_pcm[(source_frame + frame) * 2 + 1];
      if (muted) {
        left = 0;
        right = 0;
      }
      this->speaker_i2s_[frame * 2] =
          static_cast<int32_t>(static_cast<int64_t>(left) * playback_gain_q16);
      this->speaker_i2s_[frame * 2 + 1] =
          static_cast<int32_t>(static_cast<int64_t>(right) * playback_gain_q16);
    }

    const size_t bytes_to_write = frames * 2 * sizeof(int32_t);
    size_t bytes_written = 0;
    esp_err_t result = i2s_channel_write(this->tx_handle_, this->speaker_i2s_, bytes_to_write, &bytes_written,
                                         pdMS_TO_TICKS(30));
    if (result != ESP_OK) {
      return result;
    }
    if (bytes_written != bytes_to_write) {
      return ESP_FAIL;
    }

    source_frame += frames;
    frames_remaining -= frames;
  }

  return ESP_OK;
}

esp_err_t USBAudioBridge::speaker_output_cb_(uint8_t *buffer, size_t length, void *context) {
  return static_cast<USBAudioBridge *>(context)->write_speaker_(buffer, length);
}

void USBAudioBridge::mute_cb_(uint32_t mute, void *context) {
  auto *bridge = static_cast<USBAudioBridge *>(context);
  const bool muted = mute != 0;
  if (bridge->muted_ == muted) {
    return;
  }
  bridge->muted_ = muted;
  ESP_LOGD(TAG, "USB mute: %s", muted ? "on" : "off");
}

void USBAudioBridge::volume_cb_(uint32_t volume, void *context) {
  auto *bridge = static_cast<USBAudioBridge *>(context);
  const uint32_t volume_percent = std::min<uint32_t>(volume, 100);
  if (bridge->volume_percent_ == volume_percent) {
    return;
  }
  bridge->volume_percent_ = volume_percent;
  bridge->update_playback_gain_();
  if (bridge->volume_percent_ == 0) {
    ESP_LOGV(TAG, "USB volume: 0%% (silent)");
  } else {
    ESP_LOGV(TAG, "USB volume: %" PRIu32 "%%, effective gain %.1f dB", bridge->volume_percent_,
             static_cast<double>(bridge->volume_percent_) * 0.5 - 50.0 + bridge->output_gain_db_);
  }
}

}  // namespace usb_audio_bridge
}  // namespace esphome
