#pragma once

#include <utility>

#include "esphome/components/display/display_buffer.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include "icn2053_display.h"

namespace esphome {
namespace hub75_icn2053 {
class HUB75_ICN2053 : public display::DisplayBuffer {
public:
  void setup() override;

  void dump_config() override;

  void update() override;

  void set_panel_height(int panel_height) {
    this->panel_height_ = panel_height;
  }
  void set_panel_width(int panel_width) { this->panel_width_ = panel_width; }

  void set_chain_length(int chain_length) {
    this->chain_length_ = chain_length;
  }

  void set_initial_brightness(int brightness) {
    this->initial_brightness_ = brightness;
  };

  int get_initial_brightness() { return initial_brightness_; }

  void set_pins(InternalGPIOPin *R1_pin, InternalGPIOPin *G1_pin,
                InternalGPIOPin *B1_pin, InternalGPIOPin *R2_pin,
                InternalGPIOPin *G2_pin, InternalGPIOPin *B2_pin,
                InternalGPIOPin *A_pin, InternalGPIOPin *B_pin,
                InternalGPIOPin *C_pin, InternalGPIOPin *D_pin,
                InternalGPIOPin *E_pin, InternalGPIOPin *LAT_pin,
                InternalGPIOPin *OE_pin, InternalGPIOPin *CLK_pin) {
    int8_t d = -1;
    if (D_pin != NULL)
      d = D_pin->get_pin();
    int8_t e = -1;
    if (E_pin != NULL)
      e = E_pin->get_pin();

    pins_ = {static_cast<int8_t>(R1_pin->get_pin()),
             static_cast<int8_t>(G1_pin->get_pin()),
             static_cast<int8_t>(B1_pin->get_pin()),
             static_cast<int8_t>(R2_pin->get_pin()),
             static_cast<int8_t>(G2_pin->get_pin()),
             static_cast<int8_t>(B2_pin->get_pin()),
             static_cast<int8_t>(A_pin->get_pin()),
             static_cast<int8_t>(B_pin->get_pin()),
             static_cast<int8_t>(C_pin->get_pin()),
             d,
             e,
             static_cast<int8_t>(LAT_pin->get_pin()),
             static_cast<int8_t>(OE_pin->get_pin()),
             static_cast<int8_t>(CLK_pin->get_pin())};
  }

  void set_latch_blanking(int latch_blanking) {
    latch_blanking_ = latch_blanking;
  };

  display::DisplayType get_display_type() override {
    return display::DisplayType::DISPLAY_TYPE_COLOR;
  }

  void set_brightness(int brightness);
  ICN2053_Display *dma_display_ = nullptr;

protected:
  hub75_pins_t pins_;
  int latch_blanking_ = 1;
  int panel_width_ = 64;
  int panel_height_ = 32;
  int chain_length_ = 1;
  int initial_brightness_ = 255;
  bool enabled_ = true;
  int get_width_internal() override { return panel_width_ * chain_length_; };
  int get_height_internal() override { return panel_height_; };

  void draw_absolute_pixel_internal(int x, int y, Color color) override;
};

} // namespace hub75_icn2053
} // namespace esphome