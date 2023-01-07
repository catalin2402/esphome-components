#include "PT2258.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"


namespace esphome {
namespace pt2258 {

static const char *const TAG = "PT2258";

void PT2258::dump_config() {
  ESP_LOGCONFIG("PT2258", "PT2258:");
  ESP_LOGCONFIG("PT2258", "  Address: 0x%X", address_);
  ESP_LOGCONFIG("PT2258", "  Default Volume: %d", defaultVolume_);
  ESP_LOGCONFIG("PT2258", "  Master Volume: %d", masterVolume_);
  ESP_LOGCONFIG("PT2258", "  Front Volume: %d", frontVolume_);
  ESP_LOGCONFIG("PT2258", "  Rear Volume: %d", rearVolume_);
  ESP_LOGCONFIG("PT2258", "  Center Volume: %d", centerVolume_);
  ESP_LOGCONFIG("PT2258", "  Subwoofer Volume: %d", subwooferVolume_);
  ESP_LOGCONFIG("PT2258", "  Front Offset: %d", offsetFront_);
  ESP_LOGCONFIG("PT2258", "  Rear Offset: %d", offsetRear_);
  ESP_LOGCONFIG("PT2258", "  Center Offset: %d", offsetCenter_);
  ESP_LOGCONFIG("PT2258", "  Subwoofer Offset: %d", offsetSubwoofer_);
  LOG_I2C_DEVICE(this);
}

void PT2258::setup() { setDefaults(); }

void PT2258::setMasterVolume(int volume) {
  masterVolume_ = volume;
  ESP_LOGD("PT2258", "Setting master volume to %d", masterVolume_);
  setVolume();
}

void PT2258::setFrontVolume(int offset) {
  if (offset == -22) {
    return;
  }
  offsetFront_ = offset;
  ESP_LOGD("PT2258", "Setting offset front to %d", offsetFront_);
  setVolume();
}

void PT2258::setRearVolume(int offset) {
  if (offset == -22) {
    return;
  }
  offsetRear_ = offset;
  ESP_LOGD("PT2258", "Setting offset rear to %d", offsetRear_);
  setVolume();
}

void PT2258::setCenterVolume(int offset) {
  if (offset == -22) {
    return;
  }
  offsetCenter_ = offset;
  ESP_LOGD("PT2258", "Setting offset center to %d", offsetCenter_);
  setVolume();
}

void PT2258::setSubwooferVolume(int offset) {
  if (offset == -22) {
    return;
  }
  offsetSubwoofer_ = offset;
  ESP_LOGD("PT2258", "Setting offset subwoofer to %d", offsetSubwoofer_);
  setVolume();
}

void PT2258::setDefaults() { setMasterVolume(masterVolume_); }

int PT2258::getMasterVolume() { return masterVolume_; }

int PT2258::getFrontVolume() { return frontVolume_; }

int PT2258::getRearVolume() { return rearVolume_; }

int PT2258::getCenterVolume() { return centerVolume_; }

int PT2258::getSubwooferVolume() { return subwooferVolume_; }

int PT2258::getOffsetFront() { return offsetFront_; }

int PT2258::getOffsetRear() { return offsetRear_; }

int PT2258::getOffsetCenter() { return offsetCenter_; }

int PT2258::getOffsetSubwoofer() { return offsetSubwoofer_; }

bool PT2258::isPowered() { return power_; }

void PT2258::setPower(bool power) { this->power_ = power; }

void PT2258::setVolume() {
  frontVolume_ = masterVolume_ + offsetFront_;
  if (offsetFront_ < -20)
    frontVolume_ = 0;

  rearVolume_ = masterVolume_ + offsetRear_;
  if (offsetRear_ < -20)
    rearVolume_ = 0;

  centerVolume_ = masterVolume_ + offsetCenter_;
  if (offsetCenter_ < -20)
    centerVolume_ = 0;

  subwooferVolume_ = masterVolume_ + offsetSubwoofer_;
  if (offsetSubwoofer_ < -20)
    subwooferVolume_ = 0;

  if (frontVolume_ >= 79)
    frontVolume_ = 79;
  if (frontVolume_ <= 0)
    frontVolume_ = 0;
  if (rearVolume_ >= 79)
    rearVolume_ = 79;
  if (rearVolume_ <= 0)
    rearVolume_ = 0;
  if (centerVolume_ >= 79)
    centerVolume_ = 79;
  if (centerVolume_ <= 0)
    centerVolume_ = 0;
  if (subwooferVolume_ >= 79)
    subwooferVolume_ = 79;
  if (subwooferVolume_ <= 0)
    subwooferVolume_ = 0;
  if (masterVolume_ == 0) {
    frontVolume_ = 0;
    rearVolume_ = 0;
    centerVolume_ = 0;
    subwooferVolume_ = 0;
  }

  if (masterVolume_ == frontVolume_ && masterVolume_ == rearVolume_ &&
      masterVolume_ == centerVolume_ && masterVolume_ == subwooferVolume_) {
    setChannelVolume(masterVolume_, 0);
  } else {
    setChannelVolume(subwooferVolume_, 1);
    setChannelVolume(centerVolume_, 2);
    setChannelVolume(rearVolume_, 3);
    setChannelVolume(rearVolume_, 4);
    setChannelVolume(frontVolume_, 5);
    setChannelVolume(frontVolume_, 6);
  }
}

void PT2258::setChannelVolume(int volume, int channel) {
  char x10;
  char x1;

  int newVolume = 79 - volume;

  if (newVolume >= 10) {
    x10 = newVolume / 10;
    x1 = newVolume % 10;
  } else {
    x1 = newVolume;
    x10 = 0;
  }

  switch (channel) {
  case 0:
    x1 = x1 + 0xe0;
    x10 = x10 + 0xd0;
    break;
  case 1:
    x1 = x1 + 0x90;
    x10 = x10 + 0x80;
    break;
  case 2:
    x1 = x1 + 0x50;
    x10 = x10 + 0x40;
    break;
  case 3:
    x1 = x1 + 0x10;
    x10 = x10 + 0x00;
    break;
  case 4:
    x1 = x1 + 0x30;
    x10 = x10 + 0x20;
    break;
  case 5:
    x1 = x1 + 0x70;
    x10 = x10 + 0x60;
    break;
  case 6:
    x1 = x1 + 0xb0;
    x10 = x10 + 0xa0;
  }

  ESP_LOGD("PT2258", "Sending data twice: x10:0x%02X x1:0x%02X", x10, x1);

  bool result1 = this->write_byte(x10, x1);
  delay(5);
  bool result2 = this->write_byte(x10, x1);
  delay(5);

  if (!result1 && result2) {
    ESP_LOGE("PT2258", "Error writing data");
    this->status_set_warning();
  } else {
    this->status_clear_warning();
  }
}

} // namespace pt2258
} // namespace esphome