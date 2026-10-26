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
    if (this->gamma_mode_ == 1) {
      const float luminance = value * 100.0f;
      corrected = luminance <= 8.0f ? luminance / 903.3f : std::pow((luminance + 16.0f) / 116.0f, 3.0f);
    } else if (this->gamma_mode_ == 2) {
      corrected = std::pow(value, 2.2f);
    }
    this->gamma_[i] = static_cast<uint8_t>(std::lround(corrected * 255.0f));
  }
  this->panel_ = new MatrixPanel_I2S_DMA(this->config_);
  if (!this->panel_->begin()) {
    ESP_LOGE(TAG, "ICN2053 initialization failed: %s", esp_err_to_name(this->panel_->error()));
    this->mark_failed();
    this->error_callback_.call();
    return;
  }
  this->panel_->setBrightness8(this->brightness_);
  this->panel_->fillScreenRGB888(0, 0, 0);
  this->dirty_ = true;
  this->present_();
  if (this->is_failed())
    return;
  this->initialized_ = true;
  this->ready_callback_.call();
}

void HUB75Display::dump_config() {
  ESP_LOGCONFIG(TAG, "ICN2053 HUB75: %ux%u panels, %ux%u grid, %dx%d virtual pixels", this->config_.panel_width,
                this->config_.panel_height, this->config_.chain_length_y, this->config_.chain_length_x,
                this->get_width_internal(), this->get_height_internal());
  ESP_LOGCONFIG(TAG, "  Clock: %u Hz; brightness: %u; layout: %u; RGB565 framebuffer",
                static_cast<unsigned>(this->config_.clock_hz), this->brightness_, static_cast<unsigned>(this->layout_));
  LOG_UPDATE_INTERVAL(this);
}

void HUB75Display::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (this->panel_ == nullptr || this->is_failed() || x < 0 || y < 0 || x >= this->get_width_internal() ||
      y >= this->get_height_internal())
    return;
  const auto coords =
      PanelLayoutRemap::remap({x, y}, this->layout_, this->config_.panel_width, this->config_.panel_height,
                              this->config_.chain_length_y, this->config_.chain_length_x);
  this->panel_->fillRect(coords.x, coords.y, 1, 1, this->gamma_[color.r], this->gamma_[color.g], this->gamma_[color.b]);
  this->dirty_ = true;
}

void HUB75Display::present_() {
  if (this->panel_ == nullptr || this->is_failed() || !this->dirty_)
    return;
  this->panel_->present();
  this->dirty_ = false;
  if (this->panel_->error() != ESP_OK) {
    ESP_LOGE(TAG, "ICN2053 transfer failed: %s", esp_err_to_name(this->panel_->error()));
    this->mark_failed();
    this->error_callback_.call();
  }
}

void HUB75Display::update() {
  if (this->is_failed())
    return;
  this->do_update_();
  this->present_();
}

// LVGL and direct draw calls can modify the framebuffer without update().
void HUB75Display::loop() { this->present_(); }

void HUB75Display::set_brightness(int brightness) {
  const auto value = static_cast<uint8_t>(clamp(brightness, 0, 255));
  if (this->brightness_ == value)
    return;
  this->brightness_ = value;
  if (this->is_display_ready()) {
    this->panel_->setBrightness8(value);
    this->dirty_ = true;
  }
  if (this->initialized_)
    this->brightness_callback_.call(value);
}

void HUB75Display::on_shutdown() {
  this->initialized_ = false;
  if (this->panel_ != nullptr)
    this->panel_->end();
}

}  // namespace esphome::hub75_icn2053
