#include "tea5767.h"

#include "esphome/core/log.h"

namespace esphome::tea5767 {

static const char *const TAG = "tea5767";
static constexpr uint32_t REFERENCE_FREQUENCY_HZ = 32768;
static constexpr uint32_t INTERMEDIATE_FREQUENCY_HZ = 225000;
static constexpr uint32_t MIN_FREQUENCY_HZ = 87500000;
static constexpr uint32_t JAPAN_MIN_FREQUENCY_HZ = 76000000;
static constexpr uint32_t MAX_FREQUENCY_HZ = 108000000;
static constexpr uint32_t TUNING_SETTLE_MS = 100;
static constexpr uint8_t PLL_HIGH_MASK = 0x3F;
static constexpr uint8_t MUTE_MASK = 0x80;
static constexpr uint8_t MONO_MASK = 0x08;
static constexpr uint8_t JAPAN_BAND_MASK = 0x20;
static constexpr uint8_t CRYSTAL_MASK = 0x10;
static constexpr uint8_t SOFT_MUTE_MASK = 0x08;
static constexpr uint8_t HIGH_CUT_MASK = 0x04;
static constexpr uint8_t STEREO_NOISE_CANCELING_MASK = 0x02;

void TEA5767::dump_config() {
  ESP_LOGCONFIG(TAG, "TEA5767 FM Radio:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Requested frequency: %.3f MHz", this->frequency_ / 1e6);
  ESP_LOGCONFIG(TAG, "  Japan band: %s", YESNO(this->in_japan_));
  ESP_LOGCONFIG(TAG, "  Mute: %s", ONOFF(this->is_muted()));
  ESP_LOGCONFIG(TAG, "  Forced mono: %s", ONOFF(this->is_mono()));
  LOG_UPDATE_INTERVAL(this);
}

void TEA5767::setup() {
  this->initialized_ = true;
  if (this->save_registers_())
    this->schedule_read_();
}

void TEA5767::set_frequency(uint64_t frequency) {
  const uint32_t minimum = this->in_japan_ ? JAPAN_MIN_FREQUENCY_HZ : MIN_FREQUENCY_HZ;
  if (frequency < minimum || frequency > MAX_FREQUENCY_HZ) {
    ESP_LOGW(TAG, "Frequency must be between %.1f and 108 MHz", minimum / 1e6);
    return;
  }
  const bool changed = frequency != this->frequency_;
  this->frequency_ = frequency;
  if (this->save_registers_()) {
    this->status_valid_ = false;
    this->schedule_read_();
  }
  if (changed) {
    this->state_callback_.call();
    this->frequency_callback_.call(frequency);
  }
}

void TEA5767::set_setting_(Setting setting, bool enabled) {
  auto &value = this->settings_[static_cast<uint8_t>(setting)];
  const bool changed = value != enabled;
  value = enabled;
  this->save_registers_();
  if (changed) {
    this->state_callback_.call();
    this->setting_callback_.call(setting, enabled);
  }
}

void TEA5767::resend_data() {
  if (this->save_registers_())
    this->schedule_read_();
}

bool TEA5767::save_registers_() {
  if (!this->initialized_)
    return false;
  const uint16_t pll = 4 * (this->frequency_ + INTERMEDIATE_FREQUENCY_HZ) / REFERENCE_FREQUENCY_HZ;
  uint8_t registers[5] = {
      static_cast<uint8_t>((pll >> 8) & PLL_HIGH_MASK),
      static_cast<uint8_t>(pll),
      0xB0,  // Search up, medium stop level, high-side injection.
      CRYSTAL_MASK,
      0,
  };
  if (this->is_muted())
    registers[0] |= MUTE_MASK;
  if (this->is_mono())
    registers[2] |= MONO_MASK;
  if (this->in_japan_) {
    registers[3] |= JAPAN_BAND_MASK;
    registers[4] = 0x40;
  }
  if (this->is_soft_muted())
    registers[3] |= SOFT_MUTE_MASK;
  if (this->get_high_cut_control())
    registers[3] |= HIGH_CUT_MASK;
  if (this->get_stereo_noise_canceling())
    registers[3] |= STEREO_NOISE_CANCELING_MASK;
  this->write_failed_ = this->write(registers, sizeof(registers)) != i2c::ERROR_OK;
  if (this->write_failed_)
    ESP_LOGW(TAG, "Failed to write tuner settings");
  this->update_status_();
  return !this->write_failed_;
}

void TEA5767::schedule_read_() {
  this->read_pending_ = true;
  this->set_timeout("read_status", TUNING_SETTLE_MS, [this]() {
    this->read_pending_ = false;
    this->update();
  });
}

uint64_t TEA5767::get_frequency() const {
  const uint16_t pll = (static_cast<uint16_t>(this->status_[0] & PLL_HIGH_MASK) << 8) | this->status_[1];
  const uint64_t oscillator = uint64_t{pll} * REFERENCE_FREQUENCY_HZ / 4;
  return oscillator >= INTERMEDIATE_FREQUENCY_HZ ? oscillator - INTERMEDIATE_FREQUENCY_HZ : 0;
}

void TEA5767::update() {
  if (!this->initialized_ || this->read_pending_)
    return;
  std::array<uint8_t, 5> status{};
  this->read_failed_ = this->read(status.data(), status.size()) != i2c::ERROR_OK;
  this->update_status_();
  if (this->read_failed_) {
    this->status_valid_ = false;
    ESP_LOGW(TAG, "Failed to read tuner status");
    return;
  }
  const bool first = !this->status_valid_;
  const bool previous_stereo = this->is_stereo();
  const uint8_t previous_level = this->get_level();
  this->status_ = status;
  this->status_valid_ = true;
  const bool stereo = this->is_stereo();
  const uint8_t level = this->get_level();
  this->status_callback_.call(this->get_frequency(), level, stereo);
  if (first || previous_stereo != stereo)
    this->stereo_callback_.call(stereo);
  if (first || previous_level != level)
    this->level_callback_.call(level);
}

void TEA5767::update_status_() {
  if (this->read_failed_ || this->write_failed_)
    this->status_set_warning();
  else
    this->status_clear_warning();
}

}  // namespace esphome::tea5767
