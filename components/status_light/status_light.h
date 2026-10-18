#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_state.h"

namespace esphome::status_light {

class StatusLight : public Component {
 public:
  void set_light(light::LightState *light) { this->light_ = light; }
  void set_error_color(float red, float green, float blue) { this->error_color_ = {red, green, blue}; }
  void set_warning_color(float red, float green, float blue) { this->warning_color_ = {red, green, blue}; }
  void set_network_color(float red, float green, float blue) { this->network_color_ = {red, green, blue}; }
  void set_api_color(float red, float green, float blue) { this->api_color_ = {red, green, blue}; }
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

 protected:
  enum class Status : uint8_t { OK, ERROR, NETWORK, API, WARNING };
  struct RGBColor {
    float red;
    float green;
    float blue;
  };

  Status get_status_() const;
  virtual void write_idle_();
  void write_light_(Status status, bool on);

  light::LightState *light_{nullptr};
  RGBColor error_color_{1.0f, 0.0f, 0.0f};
  RGBColor warning_color_{1.0f, 1.0f, 0.0f};
  RGBColor network_color_{0.0f, 0.0f, 1.0f};
  RGBColor api_color_{0.0f, 1.0f, 0.0f};
  light::ColorMode color_mode_{light::ColorMode::UNKNOWN};
  Status last_status_{Status::OK};
  bool last_on_{false};
  bool initialized_{false};
};

}  // namespace esphome::status_light
