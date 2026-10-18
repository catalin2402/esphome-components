#include "pt2258.h"

#include "esphome/core/log.h"

namespace esphome::pt2258 {

static const char *const TAG = "pt2258";
static constexpr uint8_t MAX_VOLUME = 79;
static constexpr uint8_t CHANNEL_COUNT = 6;
static constexpr uint8_t RESET_COMMAND = 0xC0;
static constexpr uint8_t MUTE_OFF_COMMAND = 0xF8;
static constexpr uint8_t MUTE_ON_COMMAND = 0xF9;
static constexpr uint8_t TENS_COMMANDS[] = {0xD0, 0x80, 0x40, 0x00, 0x20, 0x60, 0xA0};

void PT2258::dump_config() {
  ESP_LOGCONFIG(TAG, "PT2258:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Default volume: %d", this->default_volume_);
  ESP_LOGCONFIG(TAG, "  Muted: %s", YESNO(this->muted_));
}

void PT2258::setup() {
  // Allow the audio controller to stabilize without blocking the main loop.
  this->set_timeout(200, [this]() {
    this->startup_delay_complete_ = true;
    if (this->write(&RESET_COMMAND, 1) != i2c::ERROR_OK) {
      this->status_set_warning();
      ESP_LOGW(TAG, "Failed to initialize volume controller");
      return;
    }
    this->initialized_ = true;
    this->send_data_();
  });
}

void PT2258::set_default_volume(int volume) {
  this->default_volume_ = clamp(volume, 0, int(MAX_VOLUME));
  for (auto &channel_volume : this->volumes_)
    channel_volume = this->default_volume_;
}

int PT2258::get_channel_volume(int channel, bool is_offset) const {
  if (channel < 0 || channel > CHANNEL_COUNT)
    return 0;
  return is_offset ? this->volumes_[channel] : clamp(this->volumes_[channel], 0, int(MAX_VOLUME));
}

void PT2258::set_channel_volume(int volume, int channel) {
  if (channel < 1 || channel > CHANNEL_COUNT) {
    ESP_LOGW(TAG, "Invalid channel: %d", channel);
    return;
  }
  const auto previous = this->volumes_;
  // Retain offsets outside the hardware range when the master volume changes.
  this->volumes_[channel] = clamp(volume, -int(MAX_VOLUME), 2 * int(MAX_VOLUME));
  this->send_data_();
  this->notify_volume_changes_(previous);
}

void PT2258::set_master_volume(int volume) {
  volume = clamp(volume, 0, int(MAX_VOLUME));
  const auto previous = this->volumes_;
  const int difference = volume - this->volumes_[0];
  this->volumes_[0] = volume;
  for (uint8_t channel = 1; channel <= CHANNEL_COUNT; channel++)
    this->volumes_[channel] += difference;
  this->send_data_();
  this->notify_volume_changes_(previous);
}

void PT2258::set_mute(bool mute) {
  const bool changed = this->muted_ != mute;
  this->muted_ = mute;
  this->send_data_();
  if (changed) {
    this->state_callback_.call();
    this->mute_callback_.call(mute);
  }
}

void PT2258::set_channel_offset(int offset, int channel) {
  if (offset < -int(MAX_VOLUME) || offset > MAX_VOLUME) {
    ESP_LOGW(TAG, "Invalid channel offset: %d", offset);
    return;
  }
  this->set_channel_volume(this->volumes_[0] + offset, channel);
}

void PT2258::notify_volume_changes_(const std::array<int, 7> &previous) {
  const auto current = this->volumes_;
  if (previous == current)
    return;
  this->state_callback_.call();
  for (int channel = 0; channel <= CHANNEL_COUNT; channel++) {
    if (previous[channel] != current[channel])
      this->volume_callback_.call(channel, current[channel]);
  }
}

void PT2258::resend_data() {
  if (!this->startup_delay_complete_)
    return;
  if (!this->initialized_) {
    // Permit an explicit resend to recover an unsuccessful startup reset.
    if (!this->is_ready() || this->write(&RESET_COMMAND, 1) != i2c::ERROR_OK) {
      this->status_set_warning();
      return;
    }
    this->initialized_ = true;
  }
  this->send_data_();
}

void PT2258::send_data_() {
  if (!this->initialized_)
    return;
  bool individual = false;
  for (uint8_t channel = 1; channel <= CHANNEL_COUNT; channel++)
    individual |= this->volumes_[channel] != this->volumes_[0];

  bool success = true;
  if (individual) {
    for (uint8_t channel = 1; channel <= CHANNEL_COUNT; channel++)
      success = this->send_channel_volume_(this->volumes_[channel], channel) && success;
  } else {
    success = this->send_channel_volume_(this->volumes_[0], 0);
  }
  success = this->send_mute_state_() && success;
  this->update_status_(success);
}

bool PT2258::send_mute_state_() {
  const uint8_t command = this->muted_ ? MUTE_ON_COMMAND : MUTE_OFF_COMMAND;
  return this->write(&command, 1) == i2c::ERROR_OK;
}

bool PT2258::send_channel_volume_(int volume, uint8_t channel) {
  const uint8_t attenuation = MAX_VOLUME - clamp(volume, 0, int(MAX_VOLUME));
  const uint8_t commands[] = {
      static_cast<uint8_t>(TENS_COMMANDS[channel] | (attenuation / 10)),
      static_cast<uint8_t>((TENS_COMMANDS[channel] + 0x10) | (attenuation % 10)),
  };
  return this->write(commands, sizeof(commands)) == i2c::ERROR_OK;
}

void PT2258::update_status_(bool success) {
  if (success) {
    this->status_clear_warning();
  } else {
    this->status_set_warning();
    ESP_LOGW(TAG, "Failed to write volume or mute settings");
  }
}

}  // namespace esphome::pt2258
