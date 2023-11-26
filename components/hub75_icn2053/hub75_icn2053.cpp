#include "hub75_icn2053.h"

namespace esphome {
namespace hub75_icn2053 {

static const char *const TAG = "hub75_icn2053";

void HUB75_ICN2053::setup() {
  ESP_LOGCONFIG(TAG, "Setting up hub75_icn2053...");
  hub75_i2s_cfg_t mxconfig;
  mxconfig.mx_width = panel_width_;
  mxconfig.mx_height = panel_height_;
  mxconfig.chain_length_x = chain_length_;
  mxconfig.chain_length_y = 1;
  mxconfig.gpio = pins_;
  mxconfig.i2sspeed = HZ_1M;
  mxconfig.latch_blanking = latch_blanking_;

  dma_display_ = new ICN2053_Display(mxconfig);
  dma_display_->begin();
  set_brightness(initial_brightness_);
  dma_display_->clearScreen();
}

void HUB75_ICN2053::update() {
  this->do_update_();
  dma_display_->flipDMABuffer();
}

void HUB75_ICN2053::dump_config() {
  ESP_LOGCONFIG(TAG, "hub75_icn2053:");
  ESP_LOGCONFIG(TAG, "  Pins: R1:%i, G1:%i, B1:%i, R2:%i, G2:%i, B2:%i", pins_.r1,
                pins_.g1, pins_.b1, pins_.r2, pins_.g2, pins_.b2);
  ESP_LOGCONFIG(TAG, "  Pins: A:%i, B:%i, C:%i, D:%i, E:%i", pins_.a, pins_.b,
                pins_.c, pins_.d, pins_.e);
  ESP_LOGCONFIG(TAG, "  Pins: LAT:%i, OE:%i, CLK:%i", pins_.lat, pins_.oe,
                pins_.b1, pins_.clk);
}

void HUB75_ICN2053::set_brightness(int brightness) {
  dma_display_->setPanelBrightness(brightness);
}

void HOT HUB75_ICN2053::draw_absolute_pixel_internal(int x, int y,
                                                     Color color) {
  if (x >= this->get_width_internal() || x < 0 ||
      y >= this->get_height_internal() || y < 0)
    return;

  dma_display_->drawPixelRGB888(x, y, color.r, color.g, color.b);
}

} // namespace hub75_icn2053
} // namespace esphome