#include "silvercrest_remote.h"
#include "esphome/core/log.h"

namespace esphome {
namespace silvercrest {

static const char *TAG = "silvercrest";
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

void Silvercrest::control_channel(Channel channel, bool state,
                                  uint8_t repeat_times) {
  switch (channel) {
  case A:
    send_code_(state ? CODE_A_ON : CODE_A_OFF, repeat_times);
    break;
  case B:
    send_code_(state ? CODE_B_ON : CODE_B_OFF, repeat_times);
    break;
  case C:
    send_code_(state ? CODE_C_ON : CODE_C_OFF, repeat_times);
    break;
  case D:
    send_code_(state ? CODE_D_ON : CODE_D_OFF, repeat_times);
    break;
  case MASTER:
    send_code_(state ? CODE_M_ON : CODE_M_OFF, repeat_times);
    break;
  }
}

void Silvercrest::send_code_(const bool code[24], uint8_t repeat_times) {
  for (int8_t x = 0; x < repeat_times; x++) {
    this->transmitter_pin_->digital_write(true);
    delayMicroseconds(288);
    this->transmitter_pin_->digital_write(false);
    delayMicroseconds(2232);
    for (int8_t i = 0; i < 24; i++) {
      if (code[i]) {
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

} // namespace silvercrest
} // namespace esphome