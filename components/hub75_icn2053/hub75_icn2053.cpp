#include "hub75_icn2053.h"

#include <cmath>
#include "esphome/core/log.h"
#include "panel_layout.h"

namespace esphome::hub75_icn2053 {
static const char *const TAG = "hub75_icn2053";

void HUB75Display::setup() {
  for (int i = 0; i < 256; i++) {
    const float value = i / 255.0f;
    float corrected = value;
    if (gamma_mode_ == 1) {
      const float luminance = value * 100.0f;
      corrected = luminance <= 8.0f ? luminance / 903.3f : std::pow((luminance + 16.0f) / 116.0f, 3.0f);
    } else if (gamma_mode_ == 2) {
      corrected = std::pow(value, 2.2f);
    }
    gamma_[i] = static_cast<uint8_t>(std::lround(corrected * 255.0f));
  }
  panel_ = new MatrixPanel_I2S_DMA(config_);
  if (!panel_->begin()) {
    ESP_LOGE(TAG, "ICN2053 initialization failed: %s", esp_err_to_name(panel_->error()));
    mark_failed();
    return;
  }
  panel_->setBrightness8(brightness_);
  panel_->fillScreenRGB888(0, 0, 0);
  panel_->present();
}

void HUB75Display::dump_config() {
  ESP_LOGCONFIG(TAG, "ICN2053 HUB75: %ux%u panels, %ux%u grid, %dx%d virtual pixels",
                config_.panel_width, config_.panel_height, config_.chain_length_y, config_.chain_length_x,
                get_width_internal(), get_height_internal());
  ESP_LOGCONFIG(TAG, "  Clock: %u Hz; brightness: %u; layout: %u; RGB565 framebuffer",
                static_cast<unsigned>(config_.clock_hz), brightness_, static_cast<unsigned>(layout_));
  LOG_UPDATE_INTERVAL(this);
}

void HUB75Display::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (panel_ == nullptr || is_failed() || x < 0 || y < 0 || x >= get_width_internal() || y >= get_height_internal())
    return;
  const auto coords = PanelLayoutRemap::remap({x, y}, layout_, config_.panel_width, config_.panel_height,
                                             config_.chain_length_y, config_.chain_length_x);
  panel_->fillRect(coords.x, coords.y, 1, 1, gamma_[color.r], gamma_[color.g], gamma_[color.b]);
  dirty_ = true;
}

void HUB75Display::present_() {
  if (panel_ == nullptr || is_failed() || !dirty_)
    return;
  panel_->present();
  dirty_ = false;
  if (panel_->error() != ESP_OK) {
    ESP_LOGE(TAG, "ICN2053 transfer failed: %s", esp_err_to_name(panel_->error()));
    mark_failed();
  }
}

void HUB75Display::update() {
  if (is_failed())
    return;
  do_update_();
  present_();
}

// LVGL and direct draw calls can modify the framebuffer without update().
void HUB75Display::loop() { present_(); }

void HUB75Display::set_brightness(uint8_t brightness) {
  brightness_ = brightness;
  if (panel_ != nullptr && !is_failed()) {
    panel_->setBrightness8(brightness);
    dirty_ = true;
  }
}

void HUB75Display::on_shutdown() {
  if (panel_ != nullptr)
    panel_->end();
}

}  // namespace esphome::hub75_icn2053
