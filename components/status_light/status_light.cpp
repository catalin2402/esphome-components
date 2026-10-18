#include "status_light.h"

#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#ifdef USE_NETWORK
#include "esphome/components/network/util.h"
#endif
#ifdef USE_API
#include "esphome/components/api/api_server.h"
#endif

namespace esphome::status_light {

static const char *const TAG = "status_light";

static constexpr uint32_t ERROR_PERIOD_MS = 250;
static constexpr uint32_t ERROR_ON_MS = 150;
static constexpr uint32_t WARNING_PERIOD_MS = 1500;
static constexpr uint32_t WARNING_ON_MS = 250;

void StatusLight::setup() {
  // Select an RGB mode even if the light was previously using its white output.
  for (auto mode : this->light_->get_traits().get_supported_color_modes()) {
    if (mode & light::ColorCapability::RGB) {
      this->color_mode_ = mode;
      break;
    }
  }
  this->loop();
}

StatusLight::Status StatusLight::get_status_() const {
  const uint8_t app_state = App.get_app_state();
  if ((app_state & STATUS_LED_ERROR) != 0u)
    return Status::ERROR;
#ifdef USE_NETWORK
  // Connection states precede the warnings raised by disconnected services.
  if (!network::is_disabled() && !network::is_connected())
    return Status::NETWORK;
#endif
#ifdef USE_API
  if (api::global_api_server != nullptr && !api::global_api_server->is_connected())
    return Status::API;
#endif
  if ((app_state & STATUS_LED_WARNING) != 0u)
    return Status::WARNING;
  return Status::OK;
}

void StatusLight::loop() {
  const Status status = this->get_status_();
  const uint32_t now = millis();
  const bool on = status == Status::ERROR ? now % ERROR_PERIOD_MS < ERROR_ON_MS
                                          : status != Status::OK && now % WARNING_PERIOD_MS < WARNING_ON_MS;
  if (!this->initialized_ || status != this->last_status_ || on != this->last_on_) {
    if (status == Status::OK) {
      this->write_idle_();
    } else {
      this->write_light_(status, on);
    }
    this->last_status_ = status;
    this->last_on_ = on;
    this->initialized_ = true;
  }
}

void StatusLight::write_idle_() { this->write_light_(Status::OK, false); }

void StatusLight::write_light_(Status status, bool on) {
  auto call = this->light_->make_call();
  call.set_state(on);
  call.set_transition_length_if_supported(0);
  call.set_effect(uint32_t{0});
  call.set_brightness_if_supported(1.0f);
  if (this->color_mode_ != light::ColorMode::UNKNOWN) {
    call.set_color_mode(this->color_mode_);
    call.set_color_brightness_if_supported(1.0f);
    RGBColor color{0.0f, 0.0f, 0.0f};
    switch (status) {
      case Status::ERROR:
        color = this->error_color_;
        break;
      case Status::WARNING:
        color = this->warning_color_;
        break;
      case Status::NETWORK:
        color = this->network_color_;
        break;
      case Status::API:
        color = this->api_color_;
        break;
      case Status::OK:
        break;
    }
    call.set_rgb(color.red, color.green, color.blue);
    call.set_white_if_supported(0.0f);
    call.set_cold_white_if_supported(0.0f);
    call.set_warm_white_if_supported(0.0f);
  }
  call.set_publish(false);
  call.set_save(false);
  call.perform();
}

void StatusLight::dump_config() {
  ESP_LOGCONFIG(TAG, "Status Light:");
  ESP_LOGCONFIG(TAG, "  Light: %s", this->light_->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  RGB: %s", YESNO(this->color_mode_ != light::ColorMode::UNKNOWN));
}

}  // namespace esphome::status_light
