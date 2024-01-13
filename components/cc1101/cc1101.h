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
  void set_spi_pins(InternalGPIOPin *clk_pin, InternalGPIOPin *miso_pin,
                    InternalGPIOPin *mosi_pin, InternalGPIOPin *cs_pin) {
    this->clk_pin_ = clk_pin->get_pin();
    this->miso_pin_ = miso_pin->get_pin();
    this->mosi_pin_ = mosi_pin->get_pin();
    this->cs_pin_ = cs_pin->get_pin();
  };
  void set_d0_pin(InternalGPIOPin *d0_pin) {
    this->d0_pin_ = d0_pin->get_pin();
  };
  void set_bandwidth(uint32_t bandwidth) { this->bandwidth_ = bandwidth; };
  void set_frequency(uint32_t frequency) { this->frequency_ = frequency; };

private:
  uint8_t clk_pin_;
  uint8_t miso_pin_;
  uint8_t mosi_pin_;
  uint8_t cs_pin_;
  uint8_t d0_pin_;
  uint8_t module_number_{0};
  uint32_t bandwidth_;
  uint32_t frequency_;
};

} // namespace cc1101
} // namespace esphome
