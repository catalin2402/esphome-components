#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace silvercrest {

class Silvercrest : public Component {
public:

  void setup() override;
  void set_transmitter_pin(GPIOPin *transmitter_pin) {
    transmitter_pin_ = transmitter_pin;
  }
  void SendCode_A_ON();
  void SendCode_B_ON();
  void SendCode_C_ON();
  void SendCode_D_ON();
  void SendCode_M_ON();
  void SendCode_A_OFF();
  void SendCode_B_OFF();
  void SendCode_C_OFF();
  void SendCode_D_OFF();
  void SendCode_M_OFF();

  void SendCode_A_ON(uint8_t _repeatTimes);
  void SendCode_B_ON(uint8_t _repeatTimes);
  void SendCode_C_ON(uint8_t _repeatTimes);
  void SendCode_D_ON(uint8_t _repeatTimes);
  void SendCode_M_ON(uint8_t _repeatTimes);
  void SendCode_A_OFF(uint8_t _repeatTimes);
  void SendCode_B_OFF(uint8_t _repeatTimes);
  void SendCode_C_OFF(uint8_t _repeatTimes);
  void SendCode_D_OFF(uint8_t _repeatTimes);
  void SendCode_M_OFF(uint8_t _repeatTimes);

private:
  GPIOPin *transmitter_pin_;

  void SendCode(const bool _code[24], uint8_t _repeatTimes);

};

} // namespace silvercrest
} // namespace esphome