#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

namespace esphome {
namespace tea5767 {

enum RADIO_BAND {
  RADIO_BAND_NONE = 0, ///< No band selected.

  RADIO_BAND_FM = 0x01, ///< FM band 87.5 - 108 MHz (USA, Europe) selected.
  RADIO_BAND_FMWORLD =
      0x02, ///< FM band 76 - 108 MHz (Japan, Worldwide) selected.

};

#define QUARTZ 32768
#define FILTER 225000

#define REG_1 0x00
#define REG_1_MUTE 0x80
#define REG_1_SM 0x40
#define REG_1_PLL 0x3F

#define REG_2 0x01
#define REG_2_PLL 0xFF

#define REG_3 0x02
#define REG_3_MS 0x08
#define REG_3_SSL 0x60
#define REG_3_SUD 0x80

#define REG_4 0x03
#define REG_4_SMUTE 0x08
#define REG_4_XTAL 0x10
#define REG_4_BL 0x20
#define REG_4_STBY 0x40

#define REG_5 0x04
#define REG_5_PLLREF 0x80
#define REG_5_DTC 0x40

#define STAT_3 0x02
#define STAT_3_STEREO 0x80

#define STAT_4 0x03
#define STAT_4_ADC 0xF0

class TEA5767 : public Component, public i2c::I2CDevice {

public:
  void dump_config() override;
  void setup() override;

  void setMono(bool switchOn);
  void setMute(bool switchOn);
  void setFrequency(uint16_t newF);
  void setInEurope(bool inEurope) { this->inEurope_ = inEurope; };

  uint16_t getFrequency(void);

protected:
  uint8_t registers[5];
  uint8_t status[5];
  bool inEurope_ = false;
  void readRegisters();
  void saveRegisters();
};

} // namespace tea5767
} // namespace esphome