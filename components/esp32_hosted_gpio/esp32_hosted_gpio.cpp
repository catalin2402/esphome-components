#include "esp32_hosted_gpio.h"
#include "esphome/core/log.h"

#include <cstdio>

namespace esphome::esp32_hosted_gpio {

static const char *const TAG = "esp32_hosted_gpio";
static constexpr uint32_t LINK_RETRY_INTERVAL_MS = 1000;
static constexpr uint32_t APPLY_RETRY_INTERVAL_MS = 250;

void ESP32HostedGPIOComponent::setup() {
  ESP_LOGCONFIG(TAG, "ESP32 Hosted GPIO expander enabled");
  this->next_retry_ = millis() + LINK_RETRY_INTERVAL_MS;
}

void ESP32HostedGPIOComponent::loop() { this->apply_pending_(); }

void ESP32HostedGPIOComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ESP32 Hosted GPIO:");
  ESP_LOGCONFIG(TAG, "  Uses ESP-Hosted co-processor GPIO expander RPCs");
}

float ESP32HostedGPIOComponent::get_setup_priority() const { return setup_priority::IO; }

esp_err_t ESP32HostedGPIOComponent::pin_mode(uint8_t pin, gpio::Flags flags) {
  if (pin >= 64) {
    ESP_LOGW(TAG, "Invalid co-processor GPIO%u", pin);
    return ESP_ERR_INVALID_ARG;
  }

  const uint64_t pin_mask = 1ULL << pin;
  this->pin_flags_[pin] = flags;
  this->pending_config_ |= pin_mask;
  this->configured_pins_ &= ~pin_mask;
  this->status_set_warning();
  return ESP_OK;
}

esp_err_t ESP32HostedGPIOComponent::digital_write(uint8_t pin, bool value) {
  if (pin >= 64) {
    ESP_LOGW(TAG, "Invalid co-processor GPIO%u", pin);
    return ESP_ERR_INVALID_ARG;
  }

  const uint64_t pin_mask = 1ULL << pin;
  if (value) {
    this->output_states_ |= pin_mask;
  } else {
    this->output_states_ &= ~pin_mask;
  }
  this->written_pins_ |= pin_mask;

  if ((this->configured_pins_ & pin_mask) == 0 || !this->link_ready_) {
    this->pending_writes_ |= pin_mask;
    this->status_set_warning();
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t err = esp_hosted_cp_gpio_set_level(pin, value ? 1 : 0);
  if (err != ESP_OK) {
    this->pending_writes_ |= pin_mask;
    this->mark_link_down_();
    this->status_set_warning();
    ESP_LOGW(TAG, "Failed to write co-processor GPIO%u: %s", pin, esp_err_to_name(err));
  }
  return err;
}

esp_err_t ESP32HostedGPIOComponent::digital_read(uint8_t pin, int *value) {
  if (pin >= 64 || value == nullptr) {
    ESP_LOGW(TAG, "Invalid co-processor GPIO%u read", pin);
    return ESP_ERR_INVALID_ARG;
  }

  const uint64_t pin_mask = 1ULL << pin;
  if ((this->configured_pins_ & pin_mask) == 0 || !this->link_ready_) {
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t err = esp_hosted_cp_gpio_get_level(pin, value);
  if (err != ESP_OK) {
    this->mark_link_down_();
    ESP_LOGW(TAG, "Failed to read co-processor GPIO%u: %s", pin, esp_err_to_name(err));
  }
  return err;
}

esp_err_t ESP32HostedGPIOComponent::probe_link_() {
  esp_hosted_coprocessor_fwver_t version{};
  return static_cast<esp_err_t>(esp_hosted_get_coprocessor_fwversion(&version));
}

esp_err_t ESP32HostedGPIOComponent::configure_pin_(uint8_t pin, gpio::Flags flags) {
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

void ESP32HostedGPIOComponent::mark_link_down_() {
  this->link_ready_ = false;
  this->pending_config_ |= this->configured_pins_;
  this->pending_writes_ |= this->written_pins_;
  this->configured_pins_ = 0;
}

void ESP32HostedGPIOComponent::apply_pending_() {
  if (this->pending_config_ == 0 && this->pending_writes_ == 0) {
    this->status_clear_warning();
    return;
  }

  const uint32_t now = millis();
  if (now < this->next_retry_) {
    return;
  }

  if (!this->link_ready_) {
    esp_err_t err = this->probe_link_();
    if (err == ESP_OK) {
      this->link_ready_ = true;
      this->next_retry_ = now + APPLY_RETRY_INTERVAL_MS;
    } else {
      this->next_retry_ = now + LINK_RETRY_INTERVAL_MS;
      if (!this->waiting_logged_) {
        ESP_LOGD(TAG, "Waiting for ESP-Hosted link before configuring co-processor GPIOs");
        this->waiting_logged_ = true;
      }
      this->status_set_warning();
      return;
    }
  } else {
    this->next_retry_ = now + APPLY_RETRY_INTERVAL_MS;
  }

  if (this->waiting_logged_) {
    ESP_LOGD(TAG, "ESP-Hosted link is ready; applying co-processor GPIO state");
    this->waiting_logged_ = false;
  }

  for (uint8_t pin = 0; pin < 64; pin++) {
    const uint64_t pin_mask = 1ULL << pin;
    if ((this->pending_config_ & pin_mask) == 0) {
      continue;
    }

    esp_err_t err = this->configure_pin_(pin, this->pin_flags_[pin]);
    if (err != ESP_OK) {
      this->mark_link_down_();
      this->status_set_warning();
      return;
    }

    this->pending_config_ &= ~pin_mask;
    this->configured_pins_ |= pin_mask;
    return;
  }

  for (uint8_t pin = 0; pin < 64; pin++) {
    const uint64_t pin_mask = 1ULL << pin;
    if ((this->pending_writes_ & pin_mask) == 0) {
      continue;
    }

    const bool value = (this->output_states_ & pin_mask) != 0;
    esp_err_t err = esp_hosted_cp_gpio_set_level(pin, value ? 1 : 0);
    if (err != ESP_OK) {
      this->mark_link_down_();
      this->status_set_warning();
      ESP_LOGW(TAG, "Failed to write co-processor GPIO%u: %s", pin, esp_err_to_name(err));
      return;
    }

    this->pending_writes_ &= ~pin_mask;
    return;
  }
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
