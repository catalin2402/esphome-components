#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace pt2323 {

class PT2323 : public Component, public i2c::I2CDevice {
 public:
  void dump_config() override;
  void setup() override;
  void setDefaults();
  void setPower(bool power);
  void setInput(int input);
  void setEnhance(bool enhance);
  void setBoost(bool boost);
  void setMute(bool mute);
  void setMuteFront(bool mute);
  void setMuteRear(bool mute);
  void setMuteSubwoofer(bool mute);
  void setMuteCenter(bool mute);
  bool isEnhanced();
  bool isBoosted();
  bool isMuted();
  bool isMutedFront();
  bool isMutedRear();
  bool isMutedSubwoofer();
  bool isMutedCenter();
  bool isPowered();
  int getSelectedInput();

 private:
  int input_ = 4;
  bool power_ = false;
  bool enhance_ = false;
  bool boost_ = false;
  bool mute_ = false;
  bool muteFront_ = false;
  bool muteRear_ = false;
  bool muteSubwoofer_ = false;
  bool muteCenter_ = false;
  void sendData(uint8_t data);
};

}  // namespace pt2323
}  // namespace esphome