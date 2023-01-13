#include "silvercrest_remote.h"
#include "esphome/core/log.h"

namespace esphome {
namespace silvercrest {

static const char *TAG = "Silvercrest";
static const bool CODE_A_ON[24] = {1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1,
                                   0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1};
static const bool CODE_B_ON[24] = {1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1,
                                   0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1};
static const bool CODE_C_ON[24] = {1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1,
                                   1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1};
static const bool CODE_D_ON[24] = {1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0,
                                   0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1};
static const bool CODE_M_ON[24] = {1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1,
                                   1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1};
static const bool CODE_A_OFF[24] = {1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1,
                                    1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1};
static const bool CODE_B_OFF[24] = {1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1,
                                    1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1};
static const bool CODE_C_OFF[24] = {1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1,
                                    1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1};
static const bool CODE_D_OFF[24] = {1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0,
                                    1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1};
static const bool CODE_M_OFF[24] = {1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1,
                                    0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1};

void Silvercrest::setup() {
  this->transmitter_pin_->setup();
  this->transmitter_pin_->digital_write(false);
}

void Silvercrest::SendCode(const bool _code[24], uint8_t _repeatTimes) {
  for (int8_t _x = 0; _x < _repeatTimes; _x++) {
    this->transmitter_pin_->digital_write(true);
    delayMicroseconds(288);
    this->transmitter_pin_->digital_write(false);
    delayMicroseconds(2232);
    for (int8_t _i = 0; _i < 24; _i++) {
      if (_code[_i]) {
        this->transmitter_pin_->digital_write(true);
        delayMicroseconds(288);
        this->transmitter_pin_->digital_write(false);
        delayMicroseconds(1236);
      } else {
        this->transmitter_pin_->digital_write(true);
        delayMicroseconds(1056);
        this->transmitter_pin_->digital_write(false);
        delayMicroseconds(468);
      }
    }
  }
  this->transmitter_pin_->digital_write(true);
  delayMicroseconds(2952);
  this->transmitter_pin_->digital_write(false);
}

void Silvercrest::SendCode_A_ON() { SendCode(CODE_A_ON, 5); }
void Silvercrest::SendCode_B_ON() { SendCode(CODE_B_ON, 5); }
void Silvercrest::SendCode_C_ON() { SendCode(CODE_C_ON, 5); }
void Silvercrest::SendCode_D_ON() { SendCode(CODE_D_ON, 5); }
void Silvercrest::SendCode_M_ON() { SendCode(CODE_M_ON, 5); }

void Silvercrest::SendCode_A_OFF() { SendCode(CODE_A_OFF, 5); }
void Silvercrest::SendCode_B_OFF() { SendCode(CODE_B_OFF, 5); }
void Silvercrest::SendCode_C_OFF() { SendCode(CODE_C_OFF, 5); }
void Silvercrest::SendCode_D_OFF() { SendCode(CODE_D_OFF, 5); }
void Silvercrest::SendCode_M_OFF() { SendCode(CODE_M_OFF, 5); }

void Silvercrest::SendCode_A_ON(uint8_t _repeatTimes) {
  SendCode(CODE_A_ON, _repeatTimes);
}
void Silvercrest::SendCode_B_ON(uint8_t _repeatTimes) {
  SendCode(CODE_B_ON, _repeatTimes);
}
void Silvercrest::SendCode_C_ON(uint8_t _repeatTimes) {
  SendCode(CODE_C_ON, _repeatTimes);
}
void Silvercrest::SendCode_D_ON(uint8_t _repeatTimes) {
  SendCode(CODE_D_ON, _repeatTimes);
}
void Silvercrest::SendCode_M_ON(uint8_t _repeatTimes) {
  SendCode(CODE_M_ON, _repeatTimes);
}

void Silvercrest::SendCode_A_OFF(uint8_t _repeatTimes) {
  SendCode(CODE_A_OFF, _repeatTimes);
}
void Silvercrest::SendCode_B_OFF(uint8_t _repeatTimes) {
  SendCode(CODE_B_OFF, _repeatTimes);
}
void Silvercrest::SendCode_C_OFF(uint8_t _repeatTimes) {
  SendCode(CODE_C_OFF, _repeatTimes);
}
void Silvercrest::SendCode_D_OFF(uint8_t _repeatTimes) {
  SendCode(CODE_D_OFF, _repeatTimes);
}
void Silvercrest::SendCode_M_OFF(uint8_t _repeatTimes) {
  SendCode(CODE_M_OFF, _repeatTimes);
}

} // namespace silvercrest
} // namespace esphome