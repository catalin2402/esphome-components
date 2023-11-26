#include "tea5767_switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tea5767 {

static const char *TAG = "tea5767.switch";

void TEA5767Switch::dump_config() {
  LOG_SWITCH("", "TEA5767 Switch", this);
  ESP_LOGCONFIG(TAG, "  Type: %u", this->type_);
  ESP_LOGCONFIG(TAG, "  State: %s", ONOFF(this->state));
}

void TEA5767Switch::setup() {
  this->state = get_new_state_();
  this->publish_state(this->state);
}

void TEA5767Switch::update() {
  bool new_state = get_new_state_();
  if (this->state != new_state) {
    this->state = new_state;
    this->publish_state(this->state);
  }
}

void TEA5767Switch::write_state(bool state) {
  switch (this->type_) {
  case MUTE:
    this->parent_->set_mute(state);
    break;
  case MONO:
    this->parent_->set_mono(state);
    break;
  case SOFT_MUTE:
    this->parent_->set_soft_mute(state);
    break;
  case STEREO_NOISE_CANCELING:
    this->parent_->set_stereo_noise_canceling(state);
    break;
  case HIGH_CUT_CONTROL:
    this->parent_->set_high_cut_control(state);
    break;
  }

  this->state = state;
  this->publish_state(this->state);
}

bool TEA5767Switch::get_new_state_() {
  switch (this->type_) {
  case MUTE:
    return this->parent_->is_muted();
    break;
  case MONO:
    return this->parent_->is_mono();
    break;
  case SOFT_MUTE:
    this->parent_->is_soft_muted();
    break;
  case STEREO_NOISE_CANCELING:
    this->parent_->get_stereo_noise_canceling();
    break;
  case HIGH_CUT_CONTROL:
    this->parent_->get_high_cut_control();
    break;
  }

  return false;
}

} // namespace tea5767
} // namespace esphome