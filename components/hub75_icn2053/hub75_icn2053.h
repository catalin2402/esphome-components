#pragma once

#include "esphome/core/helpers.h"
#include "esphome/components/display/display_buffer.h"
#include "icn2053.h"
#include "layout_types.h"
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif

namespace esphome::hub75_icn2053 {

class HUB75Display final : public display::DisplayBuffer {
 public:
#ifdef USE_HUB75_ICN2053_STATE_PUBLISHER
  HUB_PUBLISHER(state, int)
  HUB_PUBLISHER(dump, const char *)
#endif
#ifdef USE_NUMBER
  void control_brightness(number::Number *entity, float value);
#endif
  void set_geometry(uint16_t width, uint16_t height, uint16_t rows, uint16_t cols, Hub75PanelLayout layout) {
    this->config_.panel_width = width;
    this->config_.panel_height = height;
    this->config_.chain_length_x = cols;
    this->config_.chain_length_y = rows;
    this->layout_ = layout;
  }
  void set_pins(Hub75Pins pins) { this->config_.gpio = pins; }
  void set_clock(Hub75ClockSpeed speed, bool inverted) {
    this->config_.clock_hz = static_cast<uint32_t>(speed);
    this->config_.clock_phase_inverted = inverted;
  }
  void set_gamma(uint8_t mode) { this->gamma_mode_ = mode; }
  void set_brightness(int brightness);
  uint8_t get_brightness() const { return this->brightness_; }
  bool is_display_ready() const { return this->initialized_ && !this->is_failed(); }
  template<typename F> void add_on_brightness_callback(F &&callback) {
    this->brightness_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_ready_callback(F &&callback) {
    this->ready_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_error_callback(F &&callback) {
    this->error_callback_.add(std::forward<F>(callback));
  }
  void setup() override;
  void dump_config() override;
  void update() override;
  void fill(Color color) override;
  void loop() override;
  void on_shutdown() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }
  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_width_internal() override { return this->config_.panel_width * this->config_.chain_length_x; }
  int get_height_internal() override { return this->config_.panel_height * this->config_.chain_length_y; }
  void present_();
  Hub75Config config_{};
  Hub75PanelLayout layout_{Hub75PanelLayout::HUB75_PANEL_LAYOUT_HORIZONTAL};
  RAMUniquePtr<ICN2053Panel> panel_;
  uint8_t brightness_{128};
  uint8_t gamma_mode_{0};
  uint8_t gamma_[256]{};
  bool dirty_{false};
  bool initialized_{false};
  LazyCallbackManager<void(int)> brightness_callback_;
  LazyCallbackManager<void()> ready_callback_;
  LazyCallbackManager<void()> error_callback_;
};

}  // namespace esphome::hub75_icn2053
