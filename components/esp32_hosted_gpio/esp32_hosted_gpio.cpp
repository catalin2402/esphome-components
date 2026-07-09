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

void ESP32HostedGPIOComponent::loop() {
  this->reset_pin_cache_();
  this->apply_pending_();
}

void ESP32HostedGPIOComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ESP32 Hosted GPIO:");
  ESP_LOGCONFIG(TAG, "  Uses ESP-Hosted co-processor GPIO expander RPCs");
}

float ESP32HostedGPIOComponent::get_setup_priority() const { return setup_priority::IO; }

void ESP32HostedGPIOComponent::register_pin(ESP32HostedGPIOPin *pin) {
  if (pin == nullptr || pin->registered_) {
    return;
  }
  pin->next_ = this->pins_;
  this->pins_ = pin;
  pin->registered_ = true;
}

void ESP32HostedGPIOComponent::pin_mode(ESP32HostedGPIOPin *pin, gpio::Flags flags) {
  if (pin == nullptr) {
    return;
  }
  if (pin->pin_ >= 64) {
    ESP_LOGW(TAG, "Invalid co-processor GPIO%u", pin->pin_);
    return;
  }

  this->register_pin(pin);
  pin->flags_ = flags;
  pin->configured_ = false;
  this->status_set_warning();
}

bool ESP32HostedGPIOComponent::digital_read_hw(uint8_t pin) {
  if (pin >= 64) {
    ESP_LOGW(TAG, "Invalid co-processor GPIO%u read", pin);
    return false;
  }

  if (!this->link_ready_) {
    return false;
  }

  const uint8_t bank_start = pin - (pin % BANK_SIZE);
  const uint8_t bank_end = bank_start + BANK_SIZE;
  bool read_any = false;

  for (auto *gpio_pin = this->pins_; gpio_pin != nullptr; gpio_pin = gpio_pin->next_) {
    if (gpio_pin->pin_ < bank_start || gpio_pin->pin_ >= bank_end || !gpio_pin->configured_) {
      continue;
    }

    int value = 0;
    esp_err_t err = esp_hosted_cp_gpio_get_level(gpio_pin->pin_, &value);
    if (err != ESP_OK) {
      this->mark_link_down_();
      ESP_LOGW(TAG, "Failed to read co-processor GPIO%u: %s", gpio_pin->pin_, esp_err_to_name(err));
      return false;
    }

    gpio_pin->input_state_ = value != 0;
    read_any = true;
  }

  return read_any;
}

bool ESP32HostedGPIOComponent::digital_read_cache(uint8_t pin) {
  auto *gpio_pin = this->find_pin_(pin);
  if (gpio_pin == nullptr) {
    return false;
  }
  return gpio_pin->input_state_;
}

void ESP32HostedGPIOComponent::digital_write_hw(uint8_t pin, bool value) {
  this->reset_pin_cache_();

  auto *gpio_pin = this->find_pin_(pin);
  if (gpio_pin == nullptr) {
    ESP_LOGW(TAG, "Invalid co-processor GPIO%u write", pin);
    return;
  }

  gpio_pin->output_state_ = value;
  gpio_pin->has_output_state_ = true;

  if (!gpio_pin->configured_ || !this->link_ready_) {
    gpio_pin->pending_write_ = true;
    this->status_set_warning();
    return;
  }

  esp_err_t err = esp_hosted_cp_gpio_set_level(pin, value ? 1 : 0);
  if (err != ESP_OK) {
    gpio_pin->pending_write_ = true;
    this->mark_link_down_();
    this->status_set_warning();
    ESP_LOGW(TAG, "Failed to write co-processor GPIO%u: %s", pin, esp_err_to_name(err));
  }
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

ESP32HostedGPIOPin *ESP32HostedGPIOComponent::find_pin_(uint8_t pin) const {
  for (auto *gpio_pin = this->pins_; gpio_pin != nullptr; gpio_pin = gpio_pin->next_) {
    if (gpio_pin->pin_ == pin) {
      return gpio_pin;
    }
  }
  return nullptr;
}

void ESP32HostedGPIOComponent::mark_link_down_() {
  this->link_ready_ = false;
  for (auto *pin = this->pins_; pin != nullptr; pin = pin->next_) {
    pin->configured_ = false;
    if (pin->has_output_state_) {
      pin->pending_write_ = true;
    }
  }
}

bool ESP32HostedGPIOComponent::has_pending_() const {
  for (auto *pin = this->pins_; pin != nullptr; pin = pin->next_) {
    if (!pin->configured_ || pin->pending_write_) {
      return true;
    }
  }
  return false;
}

void ESP32HostedGPIOComponent::apply_pending_() {
  if (!this->has_pending_()) {
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

  for (auto *pin = this->pins_; pin != nullptr; pin = pin->next_) {
    if (pin->configured_) {
      continue;
    }

    esp_err_t err = this->configure_pin_(pin->pin_, pin->flags_);
    if (err != ESP_OK) {
      this->mark_link_down_();
      this->status_set_warning();
      return;
    }

    pin->configured_ = true;
    return;
  }

  for (auto *pin = this->pins_; pin != nullptr; pin = pin->next_) {
    if (!pin->pending_write_) {
      continue;
    }

    esp_err_t err = esp_hosted_cp_gpio_set_level(pin->pin_, pin->output_state_ ? 1 : 0);
    if (err != ESP_OK) {
      this->mark_link_down_();
      this->status_set_warning();
      ESP_LOGW(TAG, "Failed to write co-processor GPIO%u: %s", pin->pin_, esp_err_to_name(err));
      return;
    }

    pin->pending_write_ = false;
    this->reset_pin_cache_();
    return;
  }
}

void ESP32HostedGPIOPin::setup() { this->pin_mode(this->flags_); }

void ESP32HostedGPIOPin::pin_mode(gpio::Flags flags) {
  this->flags_ = flags;
  if (this->parent_ != nullptr) {
    this->parent_->pin_mode(this, flags);
  }
}

bool ESP32HostedGPIOPin::digital_read() {
  if (this->parent_ == nullptr) {
    return this->inverted_;
  }
  this->parent_->register_pin(this);
  return this->parent_->digital_read(this->pin_) != this->inverted_;
}

void ESP32HostedGPIOPin::digital_write(bool value) {
  if (this->parent_ != nullptr) {
    this->parent_->register_pin(this);
    this->parent_->digital_write(this->pin_, value != this->inverted_);
  }
}

size_t ESP32HostedGPIOPin::dump_summary(char *buffer, size_t len) const {
  return snprintf(buffer, len, "GPIO%u via ESP32 Hosted", this->pin_);
}

}  // namespace esphome::esp32_hosted_gpio
