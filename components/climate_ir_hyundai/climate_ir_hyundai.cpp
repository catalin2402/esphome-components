#include "climate_ir_hyundai.h"

#include <cmath>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::climate_ir_hyundai {

static const char *const TAG = "climate_ir_hyundai";

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
static constexpr uint64_t FEATURE_MASK = 0xF000000000;
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
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_DRY, climate::CLIMATE_MODE_FAN_ONLY});
  if (this->supports_cool_)
    traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  if (this->supports_heat_)
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
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

  const float temperature = clamp(this->target_temperature, float(TEMP_MIN), float(TEMP_MAX));
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
      static_cast<uint8_t>(remote_state) != checksum_(remote_state))
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

    uint64_t features = (remote_state & FEATURE_MASK) >> 36;

    this->fahrenheit_ = features & 0x4;
    this->preset = (features & 0x1) ? climate::CLIMATE_PRESET_SLEEP : climate::CLIMATE_PRESET_NONE;

    if (features & 0x2) {
      this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
    } else {
      this->swing_mode = climate::CLIMATE_SWING_OFF;
    }

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
      this->target_temperature = this->fahrenheit_ ? esphome::fahrenheit_to_celsius((remote_state & TEMP_MASK) >> 24)
                                                   : (remote_state & TEMP_MASK) >> 24;
    }
  }
  this->publish_state();

  return true;
}

uint8_t HyundaiIrClimate::checksum_(uint64_t value) {
  uint8_t sum = 0;
  for (uint8_t i = 1; i < 7; i++)
    sum += (value >> (i * 8)) & 0xFF;
  return static_cast<uint8_t>(0x18 - sum);
}

void HyundaiIrClimate::transmit_(uint64_t value) {
  value = (value & ~uint64_t{0xFF}) | checksum_(value);
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
