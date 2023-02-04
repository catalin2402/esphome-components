#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

namespace esphome {
namespace pt2258 {

class PT2258 : public Component, public i2c::I2CDevice {
public:
  void dump_config() override;
  void setup() override;
  float get_setup_priority() const override {
    return esphome::setup_priority::AFTER_WIFI;
  }
  void setDefaultVolume(int volume);
  void setMasterVolume(int volume);
  void setChannelVolume(int volume, int channel);
  void setOffsetChannelVolume(int offset, int channel);
  int getChannelVolume(int channel) { return volumes_[channel]; }

protected:
private:
  int defaultVolume_ = 40;
  int volumes_[7];
  void sendData();
  void sendChannelVolume(int volume, int channel);
  void calculateVolumes(int initialValue);
  char calculateX1(int volume);
  char calculateX10(int volume);
  bool shouldSendIndividualChannelsVolume();
};

} // namespace pt2258
} // namespace esphome