#pragma once

#include <cstdint>

#include "esphome/core/component.h"
#include "esphome/core/hal.h"

#include "esp_err.h"
#include "esp_hosted.h"

namespace esphome::esp32_hosted_gpio {

class ESP32HostedGPIOComponent final : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  esp_err_t pin_mode(uint8_t pin, gpio::Flags flags);
  esp_err_t digital_write(uint8_t pin, bool value);
  esp_err_t digital_read(uint8_t pin, int *value);

 protected:
  esp_err_t probe_link_();
  esp_err_t configure_pin_(uint8_t pin, gpio::Flags flags);
  void apply_pending_();
  void mark_link_down_();

  gpio::Flags pin_flags_[64]{};
  uint64_t pending_config_{0};
  uint64_t configured_pins_{0};
  uint64_t pending_writes_{0};
  uint64_t written_pins_{0};
  uint64_t output_states_{0};
  uint32_t next_retry_{0};
  bool link_ready_{false};
  bool waiting_logged_{false};
};

class ESP32HostedGPIOPin final : public GPIOPin {
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
  uint8_t pin_{0};
  bool inverted_{false};
  gpio::Flags flags_{gpio::FLAG_NONE};
};

}  // namespace esphome::esp32_hosted_gpio
