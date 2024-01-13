#include "cc1101.h"

namespace esphome {
namespace cc1101 {

static const char *const TAG = "cc1101";

void CC1101::dump_config() {
  ESP_LOGCONFIG(TAG, "cc1101:");
  ESP_LOGCONFIG(TAG, "  Pins: CLK:%i, MISO:%i, MOSI:%i, CS:%i, GD0:%i, ",
                this->clk_pin_, this->miso_pin_, this->mosi_pin_, this->cs_pin_,
                this->d0_pin_);
  ESP_LOGCONFIG(TAG, "  Frequency:%u", this->frequency_);
  ESP_LOGCONFIG(TAG, "  Bandwidth:%u", this->bandwidth_);
}

void CC1101::setup() {
  pinMode(this->d0_pin_, INPUT);
  ELECHOUSE_cc1101.addSpiPin(this->clk_pin_, this->miso_pin_, this->mosi_pin_,
                             this->cs_pin_, this->module_number_);
  ELECHOUSE_cc1101.setModul(this->module_number_);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setRxBW(this->bandwidth_ / 1000);
  ELECHOUSE_cc1101.setMHZ(this->frequency_ / 100000);
  ELECHOUSE_cc1101.SetRx();
}

} // namespace cc1101
} // namespace esphome