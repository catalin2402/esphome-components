#pragma once

#include "esphome/core/automation.h"
#include "esphome/components/display/display_buffer.h"
#include "icn2053.h"
#include "layout_types.h"

namespace esphome::hub75_icn2053 {

class HUB75Display : public display::DisplayBuffer {
 public:
  void set_geometry(uint16_t width, uint16_t height, uint16_t rows, uint16_t cols, Hub75PanelLayout layout) {
    config_.panel_width = width;
    config_.panel_height = height;
    config_.chain_length_x = cols;
    config_.chain_length_y = rows;
    layout_ = layout;
  }
  void set_pins(hub75_pins_t pins) { config_.gpio = pins; }
  void set_clock(Hub75ClockSpeed speed, bool inverted) {
    config_.clock_hz = static_cast<uint32_t>(speed);
    config_.clock_phase_inverted = inverted;
  }
  void set_gamma(uint8_t mode) { gamma_mode_ = mode; }
  void set_brightness(uint8_t brightness);
  void setup() override;
  void dump_config() override;
  void update() override;
  void loop() override;
  void on_shutdown() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }
  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_width_internal() override { return config_.panel_width * config_.chain_length_x; }
  int get_height_internal() override { return config_.panel_height * config_.chain_length_y; }
  void present_();
  hub75_i2s_cfg_t config_{};
  Hub75PanelLayout layout_{Hub75PanelLayout::HORIZONTAL};
  MatrixPanel_I2S_DMA *panel_{nullptr};
  uint8_t brightness_{128};
  uint8_t gamma_mode_{0};
  uint8_t gamma_[256]{};
  bool dirty_{false};
};

template<typename... Ts> class SetBrightnessAction : public Action<Ts...>, public Parented<HUB75Display> {
 public:
  TEMPLATABLE_VALUE(uint8_t, brightness)
  void play(const Ts &...x) override { this->parent_->set_brightness(this->brightness_.value(x...)); }
};

}  // namespace esphome::hub75_icn2053
