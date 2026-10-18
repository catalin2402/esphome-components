#include "tea5767_switch.h"
#include "esphome/core/log.h"

namespace esphome::tea5767 {

static const char *const TAG = "tea5767.switch";

void TEA5767Switch::dump_config() {
  LOG_SWITCH("", "TEA5767 Switch", this);
  ESP_LOGCONFIG(TAG, "  Type: %u", this->type_);
  ESP_LOGCONFIG(TAG, "  State: %s", ONOFF(this->state));
}

void TEA5767Switch::setup() {
  this->parent_->add_on_state_callback([this]() { this->update(); });
  this->publish_state(this->get_new_state_());
}

void TEA5767Switch::update() {
  bool new_state = get_new_state_();
  if (this->state != new_state) {
    this->publish_state(new_state);
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

  this->update();
}

bool TEA5767Switch::get_new_state_() {
  switch (this->type_) {
    case MUTE:
      return this->parent_->is_muted();
    case MONO:
      return this->parent_->is_mono();
    case SOFT_MUTE:
      return this->parent_->is_soft_muted();
    case STEREO_NOISE_CANCELING:
      return this->parent_->get_stereo_noise_canceling();
    case HIGH_CUT_CONTROL:
      return this->parent_->get_high_cut_control();
  }

  return false;
}

}  // namespace esphome::tea5767