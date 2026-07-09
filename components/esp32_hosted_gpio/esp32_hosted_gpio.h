#pragma once

#include <cstdint>

#include "esphome/components/gpio_expander/cached_gpio.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

#include "esp_err.h"
#include "esp_hosted.h"

namespace esphome::esp32_hosted_gpio {

class ESP32HostedGPIOPin;

class ESP32HostedGPIOComponent final : public Component,
                                       public gpio_expander::CachedGpioExpander<uint8_t, 64> {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void register_pin(ESP32HostedGPIOPin *pin);
  void pin_mode(ESP32HostedGPIOPin *pin, gpio::Flags flags);

 protected:
  bool digital_read_hw(uint8_t pin) override;
  bool digital_read_cache(uint8_t pin) override;
  void digital_write_hw(uint8_t pin, bool value) override;

  esp_err_t probe_link_();
  esp_err_t configure_pin_(uint8_t pin, gpio::Flags flags);
  ESP32HostedGPIOPin *find_pin_(uint8_t pin) const;
  void apply_pending_();
  bool has_pending_() const;
  void mark_link_down_();

  ESP32HostedGPIOPin *pins_{nullptr};
  uint32_t next_retry_{0};
  bool link_ready_{false};
  bool waiting_logged_{false};
};

class ESP32HostedGPIOPin final : public GPIOPin {
  friend class ESP32HostedGPIOComponent;

 public:
  void setup() override;
  void pin_mode(gpio::Flags flags) override;
  bool digital_read() override;
  void digital_write(bool value) override;
  size_t dump_summary(char *buffer, size_t len) const override;

  void set_parent(ESP32HostedGPIOComponent *parent) { this->parent_ = parent; }
  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_inverted(bool inverted) { this->inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { this->flags_ = flags; }

  gpio::Flags get_flags() const override { return this->flags_; }

 protected:
  ESP32HostedGPIOComponent *parent_{nullptr};
  ESP32HostedGPIOPin *next_{nullptr};
  uint8_t pin_{0};
  bool inverted_{false};
  gpio::Flags flags_{gpio::FLAG_NONE};
  bool registered_{false};
  bool configured_{false};
  bool pending_write_{false};
  bool has_output_state_{false};
  bool output_state_{false};
  bool input_state_{false};
};

}  // namespace esphome::esp32_hosted_gpio
