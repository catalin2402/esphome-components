#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace pt2258 {

class PT2258 : public Component, public i2c::I2CDevice {
 public:
  void dump_config() override;
  void setup() override;
  void setMasterVolume(int volume);
  void setFrontVolume(int offset);
  void setRearVolume(int offset);
  void setCenterVolume(int offset);
  void setSubwooferVolume(int offset);
  void setDefaults();
  int getMasterVolume();
  int getFrontVolume();
  int getRearVolume();
  int getCenterVolume();
  int getSubwooferVolume();
  int getOffsetFront();
  int getOffsetRear();
  int getOffsetCenter();
  int getOffsetSubwoofer();
  bool isPowered();
  void setPower(bool power);

 protected:
 private:
  bool power_ = false;
  int defaultVolume_ = 50;
  int masterVolume_ = defaultVolume_;
  int frontVolume_ = defaultVolume_;
  int rearVolume_ = defaultVolume_;
  int centerVolume_ = defaultVolume_;
  int subwooferVolume_ = defaultVolume_;
  int offsetFront_ = 0;
  int offsetRear_ = 0;
  int offsetCenter_ = 0;
  int offsetSubwoofer_ = 0;
  void setVolume();
  void setChannelVolume(int volume, int channel);
};

}  // namespace pt2258
}  // namespace esphome