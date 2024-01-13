#pragma once

#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

namespace esphome {
namespace cc1101 {

class CC1101 : public Component {
public:
  void dump_config() override;
  void setup() override;
  void set_spi_pins(uint8_t clk_pin, uint8_t miso_pin, uint8_t mosi_pin,
                    uint8_t cs_pin) {
    this->clk_pin_ = clk_pin;
    this->miso_pin_ = miso_pin;
    this->mosi_pin_ = mosi_pin;
    this->cs_pin_ = cs_pin_;
  };
  void set_d0_pin(uint8_t d0_pin) { this->d0_pin_ = d0_pin; };
  void set_module_number(uint8_t module_number) {
    this->module_number_ = module_number;
  };
  void set_bandwidth(uint32_t bandwidth) { this->bandwidth_ = bandwidth; };
  void set_frequency(uint32_t frequency) { this->frequency_ = frequency; };

private:
  uint8_t clk_pin_;
  uint8_t miso_pin_;
  uint8_t mosi_pin_;
  uint8_t cs_pin_;
  uint8_t d0_pin_;
  uint8_t module_number_;
  uint32_t bandwidth_;
  uint32_t frequency_;
};

} // namespace cc1101
} // namespace esphome
