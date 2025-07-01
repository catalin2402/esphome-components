#include "climate_ir_hyundai.h"

namespace esphome {
namespace climate_ir_hyundai {

static const char *const TAG = "climate.climate_ir_hyundai";

void HyundaiIrClimate::transmit_state() {
  uint64_t remote_state = 0x18000000000000;

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

  switch (this->fan_mode.value()) {
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

  auto temp = (uint8_t)roundf(
                  clamp<float>(this->target_temperature, TEMP_MIN, TEMP_MAX))
              << 24;
  remote_state |= temp;

  this->transmit_(remote_state);
  this->publish_state();
}

bool HyundaiIrClimate::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t nbits = 0;
  uint64_t remote_state = 0;

  if (!data.expect_item(HEADER_HIGH, HEADER_LOW))
    return false;

  for (nbits = 0; nbits < 56; nbits++) {
    if (data.expect_item(BIT_HIGH, BIT_ONE_LOW)) {
      remote_state = (remote_state << 1) | 1;
    } else if (data.expect_item(BIT_HIGH, BIT_ZERO_LOW)) {
      remote_state = (remote_state << 1) | 0;
    } else if (nbits == BITS) {
      break;
    } else {
      return false;
    }
  }

  if ((remote_state & 0xFF000000000000) != 0x18000000000000)
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

    uint64_t features = (remote_state & FEATURE_MASK) >> 36;

    this->fahrenheit_ = features & 0x4;
    this->preset = (features & 0x1) ? climate::CLIMATE_PRESET_SLEEP
                                    : climate::CLIMATE_PRESET_NONE;

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

    if (this->mode == climate::CLIMATE_MODE_COOL ||
        this->mode == climate::CLIMATE_MODE_HEAT) {
      this->target_temperature =
          this->fahrenheit_
              ? esphome::fahrenheit_to_celsius((remote_state & TEMP_MASK) >> 24)
              : (remote_state & TEMP_MASK) >> 24;
    }
  }
  this->publish_state();

  return true;
}

void HyundaiIrClimate::calc_checksum_(uint64_t &value) {
  uint8_t sum = 0;
  for (int i = 6; i >= 1; i--) {
    sum += (value >> (i * 8)) & 0xFF;
  }

  uint8_t checksum = (0x18 - sum) & 0xFF;
  value = (value & 0xFFFFFFFFFFFFFF00ULL) | checksum;
}

void HyundaiIrClimate::transmit_(uint64_t value) {
  this->calc_checksum_(value);
  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();

  data->set_carrier_frequency(38000);
  data->reserve(2 + BITS * 2u);

  data->item(HEADER_HIGH, HEADER_LOW);

  for (uint64_t mask = 1ULL << (BITS - 1); mask != 0; mask >>= 1) {
    if (value & mask) {
      data->item(BIT_HIGH, BIT_ONE_LOW);
    } else {
      data->item(BIT_HIGH, BIT_ZERO_LOW);
    }
  }
  data->mark(BIT_HIGH);
  transmit.perform();
}

} // namespace climate_ir_hyundai
} // namespace esphome
