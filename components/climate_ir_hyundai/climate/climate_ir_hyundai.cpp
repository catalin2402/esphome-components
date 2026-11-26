#include "climate_ir_hyundai.h"

#include <cmath>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::climate_ir_hyundai {

ESPHOME_LOG_TAG(TAG, "climate_ir_hyundai");

// Timings
static constexpr uint32_t HEADER_MARK_US = 8000;
static constexpr uint32_t HEADER_SPACE_US = 4000;
static constexpr uint32_t BIT_MARK_US = 600;
static constexpr uint32_t BIT_ONE_SPACE_US = 1600;
static constexpr uint32_t BIT_ZERO_SPACE_US = 550;

// Temperature
static constexpr uint8_t TEMP_MIN = 16;  // Celsius
static constexpr uint8_t TEMP_MAX = 31;  // Celsius

// Commands
static constexpr uint64_t COMMAND_MASK = 0xF00000000000;
static constexpr uint64_t COMMAND_ON = 0x800000000000;
static constexpr uint64_t COMMAND_OFF = 0x000000000000;

// Modes
static constexpr uint64_t MODE_MASK = 0xF0000000000;
static constexpr uint64_t MODE_COOL = 0x10000000000;
static constexpr uint64_t MODE_DRY = 0x20000000000;
static constexpr uint64_t MODE_FAN_ONLY = 0x40000000000;
static constexpr uint64_t MODE_HEAT = 0x80000000000;

// Features
static constexpr uint64_t FEATURE_SLEEP = 0x1000000000;
static constexpr uint64_t FEATURE_SWING = 0x2000000000;
static constexpr uint64_t FEATURE_FAHRENHEIT = 0x4000000000;

// Fan speed
static constexpr uint64_t FAN_MASK = 0xF00000000;
static constexpr uint64_t FAN_MIN = 0x100000000;
static constexpr uint64_t FAN_MED = 0x200000000;
static constexpr uint64_t FAN_MAX = 0x400000000;

static constexpr uint64_t TEMP_MASK = 0xFF000000;
static constexpr uint16_t NBITS = 56;

static constexpr uint64_t FRAME_HEADER = 0x18000000000000ULL;
static constexpr uint64_t FRAME_HEADER_MASK = 0xFF000000000000ULL;

HyundaiIrClimate::HyundaiIrClimate()
    : climate_ir::ClimateIR(TEMP_MIN, TEMP_MAX, 1.0f, true, true,
                            {climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                            {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL},
                            {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_SLEEP}) {}

climate::ClimateTraits HyundaiIrClimate::traits() {
  auto traits = climate_ir::ClimateIR::traits();
  // Keep the base's configured cooling/heating support, but omit automatic heat/cool.
  auto modes = traits.get_supported_modes();
  modes.erase(climate::CLIMATE_MODE_HEAT_COOL);
  traits.set_supported_modes(modes);
  return traits;
}

void HyundaiIrClimate::dump_config() {
  climate_ir::ClimateIR::dump_config();
  ESP_LOGCONFIG(TAG, "  Use Fahrenheit: %s", YESNO(this->fahrenheit_));
}

void HyundaiIrClimate::transmit_state() {
  uint64_t remote_state = FRAME_HEADER;

  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      this->mode_before_ = MODE_COOL;
      remote_state |= COMMAND_ON;
      remote_state |= MODE_COOL;
      break;
    case climate::CLIMATE_MODE_DRY:
      this->mode_before_ = MODE_DRY;
      remote_state |= COMMAND_ON;
      remote_state |= MODE_DRY;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      this->mode_before_ = MODE_FAN_ONLY;
      remote_state |= COMMAND_ON;
      remote_state |= MODE_FAN_ONLY;
      break;
    case climate::CLIMATE_MODE_HEAT:
      this->mode_before_ = MODE_HEAT;
      remote_state |= COMMAND_ON;
      remote_state |= MODE_HEAT;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      remote_state |= COMMAND_OFF;
      remote_state |= this->mode_before_;
      break;
  }

  switch (this->fan_mode.value_or(climate::CLIMATE_FAN_LOW)) {
    case climate::CLIMATE_FAN_HIGH:
      remote_state |= FAN_MAX;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      remote_state |= FAN_MED;
      break;
    case climate::CLIMATE_FAN_LOW:
    default:
      remote_state |= FAN_MIN;
      break;
  }

  if (this->swing_mode == climate::CLIMATE_SWING_VERTICAL) {
    remote_state |= FEATURE_SWING;
  }

  if (this->fahrenheit_) {
    remote_state |= FEATURE_FAHRENHEIT;
  }

  if (this->preset == climate::CLIMATE_PRESET_SLEEP) {
    remote_state |= FEATURE_SLEEP;
  }

  const float temperature = clamp(this->target_temperature, static_cast<float>(TEMP_MIN), static_cast<float>(TEMP_MAX));
  const auto encoded_temperature =
      static_cast<uint8_t>(roundf(this->fahrenheit_ ? celsius_to_fahrenheit(temperature) : temperature));
  remote_state |= uint64_t{encoded_temperature} << 24;

  this->transmit_(remote_state);
}

bool HyundaiIrClimate::on_receive(remote_base::RemoteReceiveData data) {
  uint64_t remote_state = 0;

  if (!data.expect_item(HEADER_MARK_US, HEADER_SPACE_US))
    return false;

  for (uint8_t bit = 0; bit < NBITS; bit++) {
    if (data.expect_item(BIT_MARK_US, BIT_ONE_SPACE_US)) {
      remote_state = (remote_state << 1) | 1;
    } else if (data.expect_item(BIT_MARK_US, BIT_ZERO_SPACE_US)) {
      remote_state = (remote_state << 1) | 0;
    } else {
      return false;
    }
  }

  if (!data.expect_mark(BIT_MARK_US))
    return false;
  if ((remote_state & FRAME_HEADER_MASK) != FRAME_HEADER ||
      static_cast<uint8_t>(remote_state) != this->checksum_(remote_state))
    return false;
  if ((remote_state & COMMAND_MASK) == COMMAND_OFF) {
    this->mode = climate::CLIMATE_MODE_OFF;
  } else {
    switch (remote_state & MODE_MASK) {
      case MODE_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case MODE_FAN_ONLY:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case MODE_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case MODE_COOL:
      default:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
    }

    this->mode_before_ = remote_state & MODE_MASK;

    this->fahrenheit_ = (remote_state & FEATURE_FAHRENHEIT) != 0;
    this->preset = (remote_state & FEATURE_SLEEP) != 0 ? climate::CLIMATE_PRESET_SLEEP : climate::CLIMATE_PRESET_NONE;
    this->swing_mode =
        (remote_state & FEATURE_SWING) != 0 ? climate::CLIMATE_SWING_VERTICAL : climate::CLIMATE_SWING_OFF;

    if (this->mode == climate::CLIMATE_MODE_DRY) {
      this->fan_mode = climate::CLIMATE_FAN_LOW;
    } else {
      if ((remote_state & FAN_MASK) == FAN_MIN) {
        this->fan_mode = climate::CLIMATE_FAN_LOW;
      } else if ((remote_state & FAN_MASK) == FAN_MED) {
        this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      } else if ((remote_state & FAN_MASK) == FAN_MAX) {
        this->fan_mode = climate::CLIMATE_FAN_HIGH;
      }
    }

    if (this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_HEAT) {
      const auto temperature = static_cast<uint8_t>((remote_state & TEMP_MASK) >> 24);
      this->target_temperature = this->fahrenheit_ ? fahrenheit_to_celsius(temperature) : temperature;
    }
  }
  this->publish_state();

  return true;
}

uint8_t HyundaiIrClimate::checksum_(uint64_t value) {
  const auto bytes = decode_value(value);
  // The 56-bit frame occupies bytes 1-7; exclude byte 7, the checksum itself.
  return static_cast<uint8_t>(0x18 - sum_bytes(bytes.data() + 1, bytes.size() - 2));
}

void HyundaiIrClimate::transmit_(uint64_t value) {
  value = (value & ~uint64_t{0xFF}) | this->checksum_(value);
  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();

  data->set_carrier_frequency(38000);
  data->reserve(3 + NBITS * 2u);

  data->item(HEADER_MARK_US, HEADER_SPACE_US);

  for (uint64_t mask = 1ULL << (NBITS - 1); mask != 0; mask >>= 1) {
    if (value & mask) {
      data->item(BIT_MARK_US, BIT_ONE_SPACE_US);
    } else {
      data->item(BIT_MARK_US, BIT_ZERO_SPACE_US);
    }
  }
  data->mark(BIT_MARK_US);
  transmit.perform();
}

}  // namespace esphome::climate_ir_hyundai
