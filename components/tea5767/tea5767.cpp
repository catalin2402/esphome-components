#include "tea5767.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tea5767 {

static const char *const TAG = "tea5767";

void TEA5767::dump_config() {
  ESP_LOGCONFIG(TAG, "TEA5767:");
  LOG_I2C_DEVICE(this);
}

void TEA5767::setup() {
  this->registers_[0] = 0x00;
  this->registers_[1] = 0x00;
  this->registers_[2] = 0xB0;
  this->registers_[3] = 0x10 | 0x08;

  if (this->in_japan_) {
    this->registers_[3] |= 0x20;
    this->registers_[4] = 0x40;
  } else {
    this->registers_[3] &= ~0x20;
    this->registers_[4] = 0;
  }

  this->save_registers_();
  this->update();
}

void TEA5767::update() {
  this->read_registers_();
  if (!this->status_has_warning()) {

    if (this->frequency_sensor_ != nullptr) {
      this->frequency_sensor_->publish_state(this->get_frequency());
    }

    if (this->mono_sensor_ != nullptr) {
      this->mono_sensor_->publish_state(!this->is_stereo());
    }

    if (this->level_sensor_ != nullptr) {
      this->level_sensor_->publish_state(this->get_level());
    }
  }
}

void TEA5767::set_frequency(uint64_t frequency) {
  unsigned int frequencyB = 4 * (frequency + FILTER) / QUARTZ;
  this->registers_[0] = frequencyB >> 8;
  this->registers_[1] = frequencyB & 0XFF;
  this->save_registers_();
  this->update();
}

void TEA5767::set_mono(bool mono) {
  this->is_mono_ = mono;
  if (mono)
    this->registers_[2] |= 0x08;
  else
    this->registers_[2] &= ~0x08;

  this->save_registers_();
}

void TEA5767::set_mute(bool mute) {
  this->is_muted_ = mute;
  if (mute)
    this->registers_[0] |= 0x80;
  else
    this->registers_[0] &= ~0x80;

  this->save_registers_();
}

void TEA5767::set_soft_mute(bool soft_mute) {
  this->is_soft_muted_ = soft_mute;
  if (soft_mute)
    this->registers_[3] |= 0x08;
  else
    this->registers_[3] &= ~0x08;
}

void TEA5767::set_high_cut_control(bool high_cut_control) {
  this->high_cut_control_ = high_cut_control;
  if (high_cut_control)
    this->registers_[3] |= 0x04;
  else
    this->registers_[3] &= ~0x04;
}

void TEA5767::set_stereo_noise_canceling(bool stereo_noise_canceling) {
  this->stereo_noise_canceling_ = stereo_noise_canceling;
  if (stereo_noise_canceling)
    this->registers_[3] |= 0x02;
  else
    this->registers_[3] &= ~0x02;
}

uint64_t TEA5767::get_frequency() {
  return (((status_[0] & 0x3F) << 8) | status_[1] * QUARTZ / 4) - FILTER;
}

uint8_t TEA5767::get_level() { return this->status_[3] >> 4; }

bool TEA5767::is_stereo() { return this->status_[2] & 0x80; }

bool TEA5767::read_registers_() {
  if (this->is_ready()) {
    uint8_t result = this->read(this->status_, 5);
    if (result != i2c::ERROR_OK) {
      this->status_set_warning();
      return false;
    }
    this->status_clear_warning();
    return true;
  }
  return false;
}

void TEA5767::save_registers_() {
  if (this->is_ready()) {
    uint8_t result = this->write(this->registers_, 5);
    if (result != i2c::ERROR_OK) {
      this->status_set_warning();
    }
    this->status_clear_warning();
  }
}

} // namespace tea5767
} // namespace esphome