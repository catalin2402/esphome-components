#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "icn2053_config.h"
#include "icn2053_registers.h"
#include "icn2053_transport.h"

namespace esphome::hub75_icn2053 {

class MatrixPanel_I2S_DMA {
 public:
  explicit MatrixPanel_I2S_DMA(const hub75_i2s_cfg_t &configuration);

  ~MatrixPanel_I2S_DMA();

  bool begin();

  void end();

  [[nodiscard]] esp_err_t error() const;

  void present();

  void setBrightness8(std::uint8_t brightness);

  void fillScreenRGB888(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

  void fillRect(std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height, std::uint8_t red,
                std::uint8_t green, std::uint8_t blue);

  void sendCallback();

 private:
  struct DmaBuffers {
    std::uint16_t **rows{};
    std::size_t row_length{};
    std::size_t prefix_length{};
    std::size_t suffix_length{};
    std::uint8_t row_count{};
    std::uint8_t total_count{};
  };

  hub75_i2s_cfg_t configuration_{};
  std::uint16_t pixels_per_row_{};
  std::uint8_t rows_per_frame_{};
  std::uint16_t current_color_{};
  volatile std::int16_t rows_to_send_{};

  DmaBuffers dma_{};
  std::uint16_t *frame_buffer_{};
  std::size_t frame_pixel_count_{};
  lldesc_t *descriptor_storage_{};
  lldesc_t *data_descriptors_{};
  lldesc_t *prefix_descriptors_{};
  lldesc_t *suffix_descriptors_[2]{};
  lldesc_t *extended_descriptors_{};
  std::uint16_t prefix_to_suffix_{};
  std::uint16_t data_to_suffix_{};
  std::uint16_t data_descriptor_count_{};
  std::uint16_t prefix_descriptor_count_{};
  std::uint16_t suffix_descriptor_count_{};

  driver_rgb_t *driver_registers_{};
  std::uint16_t *brightness_table_{};
  std::uint16_t driver_count_{};
  std::uint8_t current_register_{};
  std::uint8_t current_suffix_{};

  volatile int dma_interrupt_count_{};
  volatile bool buffer_ready_{};
  volatile bool buffer_busy_{};

  mutable portMUX_TYPE state_mutex_ = portMUX_INITIALIZER_UNLOCKED;
  esp_err_t last_error_{ESP_OK};
  bool initialized_{};
  volatile bool clear_frame_{};
  volatile bool automatic_vsync_{};

  void release_memory();

  bool allocate_memory();

  bool configure_dma();

  void initialize_dma_buffers();

  bool initialize_driver();

  void prepare_dma_rows(std::uint8_t row_offset);

  void send_vsync_callback();

  void send_row_callback();

  bool wait_dma_ready();

  bool send_vsync();

  bool send_frame(bool wait = false, bool automatic_vsync = true);

  bool panel_off();

  void set_panel_brightness(std::uint8_t brightness);

  void fill_rectangle(std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height);

  void set_color(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

  void set_vsync(bool leds_enabled, bool vsync) const;

  void set_register(std::uint8_t register_index, const driver_rgb_t *register_data) const;
};

}  // namespace esphome::hub75_icn2053
