#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

#include <cinttypes>

namespace esphome {
namespace climate_ir_hyundai {

// Timings
const uint32_t HEADER_HIGH = 8000;
const uint32_t HEADER_LOW = 4000;
const uint32_t BIT_HIGH = 600;
const uint32_t BIT_ONE_LOW = 1600;
const uint32_t BIT_ZERO_LOW = 550;

// Temperature
const uint8_t TEMP_MIN = 16; // Celsius
const uint8_t TEMP_MAX = 31; // Celsius

// Commands
const uint64_t COMMAND_MASK = 0xF00000000000;
const uint64_t COMMAND_ON = 0x800000000000;
const uint64_t COMMAND_OFF = 0x000000000000;

// Modes
const uint64_t MODE_MASK = 0xF0000000000;
const uint64_t MODE_COOL = 0x10000000000;
const uint64_t MODE_DRY = 0x20000000000;
const uint64_t MODE_FAN_ONLY = 0x40000000000;
const uint64_t MODE_HEAT = 0x80000000000;

// Features
const uint64_t FEATURE_MASK = 0xF000000000;
const uint64_t FEATURE_SLEEP = 0x1000000000;
const uint64_t FEATURE_SWING = 0x2000000000;
const uint64_t FEATURE_FAHRENHEIT = 0x4000000000;

// Fan speed
const uint64_t FAN_MASK = 0xF00000000;
const uint64_t FAN_MIN = 0x100000000;
const uint64_t FAN_MED = 0x200000000;
const uint64_t FAN_MAX = 0x400000000;

const uint64_t TEMP_MASK = 0xFF000000;
const uint16_t BITS = 56;

class HyundaiIrClimate : public climate_ir::ClimateIR {
public:
  HyundaiIrClimate()
      : climate_ir::ClimateIR(
            TEMP_MIN, TEMP_MAX, 1.0f, true, true,
            {climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
             climate::CLIMATE_FAN_HIGH},
            {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL},
            {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_SLEEP}) {}

  void set_fahrenheit(bool value) {
    this->fahrenheit_ = value;
  }

protected:
  void transmit_state() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;

  void calc_checksum_(uint64_t &value);
  void transmit_(uint64_t value);

  climate::ClimateTraits traits() override {
    climate::ClimateTraits traits{};
    traits.set_supports_current_temperature(this->sensor_ != nullptr);
    traits.set_supports_current_temperature(false);
    traits.set_supported_modes({
        climate::CLIMATE_MODE_OFF,
        climate::CLIMATE_MODE_HEAT,
        climate::CLIMATE_MODE_COOL,
        climate::CLIMATE_MODE_DRY,
        climate::CLIMATE_MODE_FAN_ONLY,
    });
    traits.set_supports_action(true);
    traits.set_supported_fan_modes(this->fan_modes_);
    traits.set_supported_swing_modes(this->swing_modes_);
    traits.set_supports_two_point_target_temperature(false);
    traits.set_visual_min_temperature(this->minimum_temperature_);
    traits.set_visual_max_temperature(this->maximum_temperature_);
    traits.set_visual_temperature_step(this->temperature_step_);
    traits.set_supported_presets(this->presets_);

    return traits;
  }

  uint32_t header_high_;
  uint32_t header_low_;
  uint32_t bit_high_;
  uint32_t bit_one_low_;
  uint32_t bit_zero_low_;
  bool fahrenheit_{false};

  uint64_t mode_before_{MODE_COOL};
};

} // namespace climate_ir_hyundai
} // namespace esphome
