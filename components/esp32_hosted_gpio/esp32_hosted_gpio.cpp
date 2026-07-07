#include "esp32_hosted_gpio.h"
#include "esphome/core/log.h"

#include <cstdio>

namespace esphome::esp32_hosted_gpio {

static const char *const TAG = "esp32_hosted_gpio";

void ESP32HostedGPIOComponent::setup() {
  ESP_LOGCONFIG(TAG, "ESP32 Hosted GPIO expander enabled");
}

void ESP32HostedGPIOComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ESP32 Hosted GPIO:");
  ESP_LOGCONFIG(TAG, "  Uses ESP-Hosted co-processor GPIO expander RPCs");
}

float ESP32HostedGPIOComponent::get_setup_priority() const { return setup_priority::IO; }

esp_err_t ESP32HostedGPIOComponent::pin_mode(uint8_t pin, gpio::Flags flags) {
  uint32_t mode = H_CP_GPIO_MODE_DISABLE;

  const bool input = flags & gpio::FLAG_INPUT;
  const bool output = flags & gpio::FLAG_OUTPUT;
  const bool open_drain = flags & gpio::FLAG_OPEN_DRAIN;

  if (input && output && open_drain) {
    mode = H_CP_GPIO_MODE_INPUT_OUTPUT_OD;
  } else if (input && output) {
    mode = H_CP_GPIO_MODE_INPUT_OUTPUT;
  } else if (output && open_drain) {
    mode = H_CP_GPIO_MODE_OUTPUT_OD;
  } else if (output) {
    mode = H_CP_GPIO_MODE_OUTPUT;
  } else if (input) {
    mode = H_CP_GPIO_MODE_INPUT;
  }

  esp_hosted_cp_gpio_config_t config{};
  config.pin_bit_mask = 1ULL << pin;
  config.mode = mode;
  config.pull_up_en = static_cast<uint32_t>(bool(flags & gpio::FLAG_PULLUP));
  config.pull_down_en = static_cast<uint32_t>(bool(flags & gpio::FLAG_PULLDOWN));
  config.intr_type = 0;

  esp_err_t err = esp_hosted_cp_gpio_config(&config);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to configure co-processor GPIO%u: %s", pin, esp_err_to_name(err));
  }
  return err;
}

esp_err_t ESP32HostedGPIOComponent::digital_write(uint8_t pin, bool value) {
  esp_err_t err = esp_hosted_cp_gpio_set_level(pin, value ? 1 : 0);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to write co-processor GPIO%u: %s", pin, esp_err_to_name(err));
  }
  return err;
}

esp_err_t ESP32HostedGPIOComponent::digital_read(uint8_t pin, int *value) {
  esp_err_t err = esp_hosted_cp_gpio_get_level(pin, value);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to read co-processor GPIO%u: %s", pin, esp_err_to_name(err));
  }
  return err;
}

void ESP32HostedGPIOPin::setup() { this->pin_mode(this->flags_); }

void ESP32HostedGPIOPin::pin_mode(gpio::Flags flags) {
  this->flags_ = flags;
  if (this->parent_ != nullptr) {
    this->parent_->pin_mode(this->pin_, flags);
  }
}

bool ESP32HostedGPIOPin::digital_read() {
  int value = 0;
  if (this->parent_ == nullptr || this->parent_->digital_read(this->pin_, &value) != ESP_OK) {
    return this->inverted_;
  }
  return bool(value) != this->inverted_;
}

void ESP32HostedGPIOPin::digital_write(bool value) {
  if (this->parent_ != nullptr) {
    this->parent_->digital_write(this->pin_, value != this->inverted_);
  }
}

size_t ESP32HostedGPIOPin::dump_summary(char *buffer, size_t len) const {
  return snprintf(buffer, len, "GPIO%u via ESP32 Hosted", this->pin_);
}

}  // namespace esphome::esp32_hosted_gpio
