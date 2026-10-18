#include "pt2323.h"

#include "esphome/core/log.h"

namespace esphome::pt2323 {

static const char *const TAG = "pt2323";
static constexpr uint8_t INPUT_COMMAND = 0xC7;
static constexpr uint8_t ENHANCE_ON_COMMAND = 0xD0;
static constexpr uint8_t ENHANCE_OFF_COMMAND = 0xD1;
static constexpr uint8_t BOOST_ON_COMMAND = 0x91;
static constexpr uint8_t BOOST_OFF_COMMAND = 0x90;
static constexpr uint8_t MUTE_OFF_COMMAND = 0xFE;
static constexpr uint8_t MUTE_ON_COMMAND = 0xFF;
static constexpr uint8_t CHANNEL_MUTE_COMMANDS[] = {0xF0, 0xF2, 0xF8, 0xFA, 0xF6, 0xF4};

void PT2323::dump_config() {
  ESP_LOGCONFIG(TAG, "PT2323:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Input: %u", this->input_);
  ESP_LOGCONFIG(TAG, "  Enhance: %s", ONOFF(this->enhance_));
  ESP_LOGCONFIG(TAG, "  Boost: %s", ONOFF(this->boost_));
  ESP_LOGCONFIG(TAG, "  Mute: %s", ONOFF(this->mute_));
}

void PT2323::setup() {
  this->initialized_ = true;
  this->send_data_();
}

void PT2323::set_input(int input) {
  if (input < 0 || input > 4) {
    ESP_LOGW(TAG, "Invalid input: %d", input);
    return;
  }
  const bool changed = this->input_ != input;
  this->input_ = input;
  this->send_data_();
  if (changed) {
    this->state_callback_.call();
    this->input_callback_.call(input);
  }
}

void PT2323::set_enhance(bool enhance) {
  const bool changed = this->enhance_ != enhance;
  this->enhance_ = enhance;
  this->send_data_();
  if (changed) {
    this->state_callback_.call();
    this->enhance_callback_.call(enhance);
  }
}

void PT2323::set_boost(bool boost) {
  const bool changed = this->boost_ != boost;
  this->boost_ = boost;
  this->send_data_();
  if (changed) {
    this->state_callback_.call();
    this->boost_callback_.call(boost);
  }
}

void PT2323::mute_all_channels(bool mute) {
  const bool changed = this->mute_ != mute;
  this->mute_ = mute;
  this->send_data_();
  if (changed) {
    this->state_callback_.call();
    this->mute_callback_.call(mute);
  }
}

void PT2323::mute_channel(int channel, bool mute) {
  if (channel < 1 || channel > 6) {
    ESP_LOGW(TAG, "Invalid channel: %d", channel);
    return;
  }
  const bool changed = this->channels_muted_[channel - 1] != mute;
  this->channels_muted_[channel - 1] = mute;
  this->send_data_();
  if (changed) {
    this->state_callback_.call();
    this->channel_mute_callback_.call(channel, mute);
  }
}

void PT2323::set_defaults() { this->resend_data(); }
void PT2323::resend_data() { this->send_data_(); }

void PT2323::send_data_() {
  if (!this->initialized_)
    return;
  const uint8_t settings[] = {
      static_cast<uint8_t>(INPUT_COMMAND + this->input_),
      this->enhance_ ? ENHANCE_ON_COMMAND : ENHANCE_OFF_COMMAND,
      this->boost_ ? BOOST_ON_COMMAND : BOOST_OFF_COMMAND,
      this->mute_ ? MUTE_ON_COMMAND : MUTE_OFF_COMMAND,
  };
  bool success = true;
  for (uint8_t command : settings)
    success = (this->write(&command, 1) == i2c::ERROR_OK) && success;
  // Restore individual mutes after the all-channel command, including on resend.
  if (!this->mute_) {
    for (uint8_t channel = 0; channel < 6; channel++) {
      const uint8_t command = CHANNEL_MUTE_COMMANDS[channel] | this->channels_muted_[channel];
      success = (this->write(&command, 1) == i2c::ERROR_OK) && success;
    }
  }
  if (success) {
    this->status_clear_warning();
  } else {
    this->status_set_warning();
    ESP_LOGW(TAG, "Failed to write audio settings");
  }
}

}  // namespace esphome::pt2323
