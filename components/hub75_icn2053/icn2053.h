#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "icn2053_config.h"
#include "icn2053_registers.h"
#include "icn2053_transport.h"

namespace esphome::hub75_icn2053 {

class ICN2053Panel {
 public:
  explicit ICN2053Panel(const Hub75Config &configuration);

  ~ICN2053Panel();
  ICN2053Panel(const ICN2053Panel &) = delete;
  ICN2053Panel &operator=(const ICN2053Panel &) = delete;

  bool begin();

  void end();

  [[nodiscard]] esp_err_t error() const;

  void present();

  void set_brightness(uint8_t brightness);

  void fill(uint8_t red, uint8_t green, uint8_t blue);

  void fill_rect(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t red, uint8_t green, uint8_t blue);

  void send_callback();

 private:
  struct DmaBuffers {
    std::array<uint16_t *, ICN2053_DMA_ROW_BUFFER_COUNT + ICN2053_PREFIX_BUFFER_COUNT + ICN2053_SUFFIX_BUFFER_COUNT>
        rows{};
    size_t row_length{};
    size_t prefix_length{};
    size_t suffix_length{};
    uint8_t row_count{};
    uint8_t total_count{};
  };

  Hub75Config configuration_{};
  uint16_t pixels_per_row_{};
  uint8_t rows_per_frame_{};
  uint16_t current_color_{};
  volatile int16_t rows_to_send_{};

  DmaBuffers dma_{};
  uint16_t *frame_buffer_{};
  size_t frame_pixel_count_{};
  lldesc_t *descriptor_storage_{};
  lldesc_t *data_descriptors_{};
  lldesc_t *prefix_descriptors_{};
  lldesc_t *suffix_descriptors_[2]{};
  lldesc_t *extended_descriptors_{};
  uint16_t prefix_to_suffix_{};
  uint16_t data_to_suffix_{};
  uint16_t data_descriptor_count_{};
  uint16_t prefix_descriptor_count_{};
  uint16_t suffix_descriptor_count_{};

  std::array<ICN2053RegisterRGB, ICN2053_REGISTER_COUNT> driver_registers_{};
  std::array<uint16_t, 256> brightness_table_{};
  uint16_t driver_count_{};
  uint8_t current_register_{};
  uint8_t current_suffix_{};

  volatile int dma_interrupt_count_{};
  volatile bool buffer_ready_{};
  volatile bool buffer_busy_{};

  mutable portMUX_TYPE state_mutex_ = portMUX_INITIALIZER_UNLOCKED;
  esp_err_t last_error_{ESP_OK};
  bool initialized_{};
  volatile bool clear_frame_{};
  volatile bool automatic_vsync_{};

  void release_memory_();

  bool allocate_memory_();

  bool configure_dma_();

  void initialize_dma_buffers_();

  bool initialize_driver_();

  void prepare_dma_rows_(uint8_t row_offset);

  void send_vsync_callback_();

  void send_row_callback_();

  bool wait_dma_ready_();

  bool send_vsync_();

  bool send_frame_(bool wait = false, bool automatic_vsync = true);

  bool panel_off_();

  void set_panel_brightness_(uint8_t brightness);

  void fill_rectangle_(int16_t x, int16_t y, int16_t width, int16_t height);

  void set_color_(uint8_t red, uint8_t green, uint8_t blue);

  void set_vsync_(bool leds_enabled, bool vsync) const;

  void set_register_(uint8_t register_index, const ICN2053RegisterRGB *register_data) const;
};

}  // namespace esphome::hub75_icn2053
