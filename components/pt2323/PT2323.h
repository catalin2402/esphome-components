#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

namespace esphome {
namespace pt2323 {

class PT2323 : public Component, public i2c::I2CDevice {
public:
  float get_setup_priority() const override {
    return esphome::setup_priority::AFTER_WIFI;
  }
  void dump_config() override;

  void setup() override;
  void setDefaults();

  void setInput(int input);
  void setEnhance(bool enhance);
  void setBoost(bool boost);
  void muteAllChannels(bool mute);
  void muteChannel(int channel, bool mute);

  int getSelectedInput() { return input_; }
  bool getEnhance() { return enhance_; }
  bool getMute() { return mute_; }
  bool getBoost() { return boost_; }

private:
  int input_ = 0;
  bool enhance_ = false;
  bool boost_ = false;
  bool mute_ = false;
  void sendData(uint8_t data);
};

} // namespace pt2323
} // namespace esphome