#include "icn2053.h"

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "esp_attr.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome::hub75_icn2053 {

namespace {
constexpr char kTag[] = "hub75_icn2053";
constexpr std::size_t kBrightnessTableSize = 256;

constexpr std::uint8_t kLuminance[kBrightnessTableSize] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,
    2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,
    6,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  11,
    12,  12,  12,  13,  13,  13,  14,  14,  14,  15,  15,  16,  16,  17,  17,  17,  18,  18,  19,  19,  20,  20,
    21,  21,  22,  22,  23,  23,  24,  24,  25,  25,  26,  27,  27,  28,  28,  29,  30,  30,  31,  31,  32,  33,
    33,  34,  35,  35,  36,  37,  38,  38,  39,  40,  41,  41,  42,  43,  44,  45,  45,  46,  47,  48,  49,  50,
    51,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
    73,  74,  75,  76,  77,  78,  80,  81,  82,  83,  84,  86,  87,  88,  90,  91,  92,  93,  95,  96,  98,  99,
    100, 102, 103, 105, 106, 107, 109, 110, 112, 113, 115, 116, 118, 120, 121, 123, 124, 126, 128, 129, 131, 133,
    134, 136, 138, 139, 141, 143, 145, 146, 148, 150, 152, 154, 156, 157, 159, 161, 163, 165, 167, 169, 171, 173,
    175, 177, 179, 181, 183, 185, 187, 189, 192, 194, 196, 198, 200, 203, 205, 207, 209, 212, 214, 216, 218, 221,
    223, 226, 228, 230, 233, 235, 238, 240, 243, 245, 248, 250, 253, 255,
};

std::uint16_t rgb565(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue) {
  return static_cast<std::uint16_t>(((red & 0xf8U) << 8U) | ((green & 0xfcU) << 3U) | (blue >> 3U));
}

void IRAM_ATTR rgb888(const std::uint16_t color, std::uint8_t &red, std::uint8_t &green, std::uint8_t &blue) {
  red = static_cast<std::uint8_t>(((color >> 11U) * 527U + 23U) >> 6U);
  green = static_cast<std::uint8_t>((((color >> 5U) & 0x3fU) * 259U + 33U) >> 6U);
  blue = static_cast<std::uint8_t>(((color & 0x1fU) * 527U + 23U) >> 6U);
}

MatrixPanel_I2S_DMA *active_panel;

std::uint16_t descriptors_for(const std::size_t buffer_length, const std::uint8_t buffer_count) {
  const auto bytes = buffer_length * sizeof(std::uint16_t);
  const auto per_buffer = (bytes + ICN2053_DMA_MAX_BYTES - 1) / ICN2053_DMA_MAX_BYTES;
  return static_cast<std::uint16_t>(per_buffer * buffer_count);
}

lldesc_t *link_buffer(lldesc_t *descriptor, lldesc_t *previous, std::uint8_t *buffer, std::size_t bytes) {
  while (bytes > 0) {
    const auto chunk = std::min<std::size_t>(bytes, ICN2053_DMA_MAX_BYTES);
    icn2053_link_dma_descriptor(descriptor, previous, buffer, chunk);
    previous = descriptor++;
    buffer += chunk;
    bytes -= chunk;
  }
  return previous;
}

void link_buffers(lldesc_t *descriptors, std::uint16_t **buffers, const std::uint8_t buffer_count,
                  const std::size_t buffer_length) {
  lldesc_t *previous = nullptr;
  auto *next = descriptors;
  const auto bytes = buffer_length * sizeof(std::uint16_t);
  for (std::uint8_t index = 0; index < buffer_count; ++index) {
    previous = link_buffer(next, previous, reinterpret_cast<std::uint8_t *>(buffers[index]), bytes);
    next = previous + 1;
  }
}

void IRAM_ATTR transport_callback() {
  if (active_panel != nullptr) {
    active_panel->sendCallback();
  }
}
}

MatrixPanel_I2S_DMA::MatrixPanel_I2S_DMA(const hub75_i2s_cfg_t &configuration) : configuration_(configuration) {}

MatrixPanel_I2S_DMA::~MatrixPanel_I2S_DMA() { end(); }

void MatrixPanel_I2S_DMA::set_color(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue) {
  current_color_ = rgb565(red, green, blue);
}

void MatrixPanel_I2S_DMA::fillScreenRGB888(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue) {
  set_color(red, green, blue);
  fill_rectangle(0, 0, static_cast<std::int16_t>(pixels_per_row_),
                 static_cast<std::int16_t>(configuration_.panel_height));
}

void MatrixPanel_I2S_DMA::fillRect(const std::int16_t x, const std::int16_t y, const std::int16_t width,
                                   const std::int16_t height, const std::uint8_t red, const std::uint8_t green,
                                   const std::uint8_t blue) {
  set_color(red, green, blue);
  fill_rectangle(x, y, width, height);
}

bool MatrixPanel_I2S_DMA::begin() {
  if (initialized_) {
    return true;
  }

  last_error_ = ESP_OK;
  pixels_per_row_ = static_cast<std::uint16_t>(configuration_.panel_width * configuration_.chain_length_x *
                                               configuration_.chain_length_y);
  rows_per_frame_ = static_cast<std::uint8_t>(configuration_.panel_height / kIcn2053RowsInParallel);

  if (pixels_per_row_ == 0 || rows_per_frame_ == 0 || (pixels_per_row_ % kIcn2053DriverBits) != 0) {
    last_error_ = ESP_ERR_INVALID_ARG;
    ESP_LOGE(kTag, "Invalid geometry: width=%d height=%d chain=%dx%d", configuration_.panel_width,
             configuration_.panel_height, configuration_.chain_length_x, configuration_.chain_length_y);
    return false;
  }

  driver_count_ = pixels_per_row_ / kIcn2053DriverBits;
  if (!allocate_memory()) {
    last_error_ = ESP_ERR_NO_MEM;
    release_memory();
    return false;
  }
  if (!configure_dma()) {
    portENTER_CRITICAL(&state_mutex_);
    initialized_ = false;
    if (last_error_ == ESP_OK)
      last_error_ = ESP_FAIL;
    portEXIT_CRITICAL(&state_mutex_);
    icn2053_transport_set_callback(nullptr);
    icn2053_transport_uninstall();
    if (active_panel == this)
      active_panel = nullptr;
    release_memory();
    return false;
  }

  ESP_LOGI(kTag, "Ready: %dx%d pixels, %d ICN2053 drivers, %" PRIu32 " Hz", pixels_per_row_,
           configuration_.panel_height, driver_count_, configuration_.clock_hz);
  return true;
}

void MatrixPanel_I2S_DMA::end() {
  if (initialized_) {
    fillScreenRGB888(0, 0, 0);
    present();
    wait_dma_ready();
    panel_off();
    wait_dma_ready();
  }

  portENTER_CRITICAL(&state_mutex_);
  initialized_ = false;
  portEXIT_CRITICAL(&state_mutex_);

  icn2053_transport_set_callback(nullptr);
  icn2053_transport_stop();
  icn2053_transport_uninstall();
  if (active_panel == this) {
    active_panel = nullptr;
  }
  release_memory();
}

esp_err_t MatrixPanel_I2S_DMA::error() const {
  portENTER_CRITICAL(&state_mutex_);
  const esp_err_t result = last_error_;
  portEXIT_CRITICAL(&state_mutex_);
  return result;
}

void MatrixPanel_I2S_DMA::release_memory() {
  if (dma_.rows != nullptr) {
    for (std::uint8_t index = 0; index < dma_.total_count; ++index) {
      heap_caps_free(dma_.rows[index]);
    }
    heap_caps_free(dma_.rows);
  }
  heap_caps_free(frame_buffer_);
  heap_caps_free(descriptor_storage_);
  heap_caps_free(driver_registers_);
  heap_caps_free(brightness_table_);

  dma_ = {};
  frame_buffer_ = nullptr;
  frame_pixel_count_ = 0;
  descriptor_storage_ = nullptr;
  data_descriptors_ = nullptr;
  prefix_descriptors_ = nullptr;
  suffix_descriptors_[0] = nullptr;
  suffix_descriptors_[1] = nullptr;
  extended_descriptors_ = nullptr;
  driver_registers_ = nullptr;
  brightness_table_ = nullptr;
  data_descriptor_count_ = 0;
  prefix_descriptor_count_ = 0;
  suffix_descriptor_count_ = 0;
}

bool MatrixPanel_I2S_DMA::allocate_memory() {
  dma_.prefix_length = kIcn2053FramePadding + kIcn2053PrefixStartLength + pixels_per_row_;
  dma_.suffix_length = kIcn2053RowOeLength * rows_per_frame_;
  dma_.row_length = (pixels_per_row_ + kIcn2053SubrowPadding) * kIcn2053DriverBits + kIcn2053RowPadding;
  dma_.row_count = std::min<std::uint8_t>(kIcn2053DmaRowBufferCount, rows_per_frame_);
  dma_.total_count = dma_.row_count + kIcn2053PrefixBufferCount + kIcn2053SuffixBufferCount;
  frame_pixel_count_ = static_cast<std::size_t>(pixels_per_row_) * configuration_.panel_height;

  data_descriptor_count_ = descriptors_for(dma_.row_length, dma_.row_count);
  prefix_descriptor_count_ = descriptors_for(dma_.prefix_length, kIcn2053PrefixBufferCount);
  suffix_descriptor_count_ = descriptors_for(dma_.suffix_length, kIcn2053SuffixBufferCount);
  const std::size_t descriptor_count = data_descriptor_count_ + prefix_descriptor_count_ +
                                       suffix_descriptor_count_ * kIcn2053SuffixDescriptorSets + ICN2053_EXT_COUNT;

  const std::size_t dma_buffer_bytes =
      (dma_.row_length * dma_.row_count + dma_.prefix_length + dma_.suffix_length) * sizeof(std::uint16_t);
  const std::size_t descriptor_bytes = descriptor_count * sizeof(lldesc_t);

  ESP_LOGI(kTag, "Allocating %zu B DMA buffers, %zu B descriptors, %zu B framebuffer", dma_buffer_bytes,
           descriptor_bytes, frame_pixel_count_ * sizeof(std::uint16_t));

  dma_.rows = static_cast<std::uint16_t **>(
      heap_caps_calloc(dma_.total_count, sizeof(std::uint16_t *), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
  if (dma_.rows == nullptr) {
    ESP_LOGE(kTag, "Unable to allocate DMA buffer pointer table");
    return false;
  }

  for (std::uint8_t index = 0; index < dma_.total_count; ++index) {
    std::size_t length = dma_.row_length;
    if (index == dma_.row_count) {
      length = dma_.prefix_length;
    } else if (index > dma_.row_count) {
      length = dma_.suffix_length;
    }

    dma_.rows[index] = static_cast<std::uint16_t *>(
        heap_caps_calloc(length, sizeof(std::uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    if (dma_.rows[index] == nullptr) {
      ESP_LOGE(kTag, "Unable to allocate DMA buffer %d (%zu bytes)", index, length * sizeof(std::uint16_t));
      return false;
    }
  }

  frame_buffer_ = static_cast<std::uint16_t *>(
      heap_caps_calloc(frame_pixel_count_, sizeof(std::uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  brightness_table_ = static_cast<std::uint16_t *>(
      heap_caps_calloc(kBrightnessTableSize, sizeof(std::uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  descriptor_storage_ = static_cast<lldesc_t *>(
      heap_caps_calloc(descriptor_count, sizeof(lldesc_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
  driver_registers_ = static_cast<driver_rgb_t *>(
      heap_caps_calloc(ICN2053_REGISTER_COUNT, sizeof(driver_rgb_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

  if (frame_buffer_ == nullptr || brightness_table_ == nullptr || descriptor_storage_ == nullptr ||
      driver_registers_ == nullptr) {
    ESP_LOGE(kTag, "Unable to allocate framebuffer, descriptors, or driver state");
    return false;
  }

  data_descriptors_ = descriptor_storage_;
  prefix_descriptors_ = data_descriptors_ + data_descriptor_count_;
  suffix_descriptors_[0] = prefix_descriptors_ + prefix_descriptor_count_;
  suffix_descriptors_[1] = suffix_descriptors_[0] + suffix_descriptor_count_;
  extended_descriptors_ = suffix_descriptors_[1] + suffix_descriptor_count_;
  return true;
}

bool MatrixPanel_I2S_DMA::configure_dma() {
  link_buffers(prefix_descriptors_, &dma_.rows[dma_.row_count], kIcn2053PrefixBufferCount, dma_.prefix_length);
  link_buffers(suffix_descriptors_[0], &dma_.rows[dma_.row_count + 1], kIcn2053SuffixBufferCount, dma_.suffix_length);
  link_buffers(suffix_descriptors_[1], &dma_.rows[dma_.row_count + 1], kIcn2053SuffixBufferCount, dma_.suffix_length);
  link_buffers(data_descriptors_, dma_.rows, dma_.row_count, dma_.row_length);

  for (std::uint8_t index = 0; index < ICN2053_EXT_COUNT; ++index) {
    icn2053_link_dma_descriptor(&extended_descriptors_[index], nullptr, nullptr, 0);
  }

  std::memcpy(driver_registers_, kIcn2053RegisterDefaults, sizeof(kIcn2053RegisterDefaults));
  set_icn2053_register_rgb(&driver_registers_[ICN2053_CFG1], static_cast<driver_reg_t>(rows_per_frame_ - 1),
                           kIcn2053Cfg1LineCountMask, kIcn2053Cfg1LineCountOffset);
  initialize_dma_buffers();

  icn2053_transport_config_t transport_configuration{};
  const int data_pins[16] = {
      configuration_.gpio.r1,
      configuration_.gpio.g1,
      configuration_.gpio.b1,
      configuration_.gpio.r2,
      configuration_.gpio.g2,
      configuration_.gpio.b2,
      configuration_.gpio.lat,
      configuration_.gpio.oe,
      configuration_.gpio.a,
      configuration_.gpio.b,
      configuration_.gpio.c,
      configuration_.gpio.d,
      configuration_.gpio.e,
      -1,
      -1,
      -1,
  };
  std::copy_n(data_pins, 16, transport_configuration.gpio_bus);
  transport_configuration.gpio_clk = configuration_.gpio.clk;
  transport_configuration.clock_hz = configuration_.clock_hz;
  transport_configuration.clock_phase_inverted = configuration_.clock_phase_inverted;

  const esp_err_t result = icn2053_transport_install(&transport_configuration);
  if (result != ESP_OK) {
    last_error_ = result;
    ESP_LOGE(kTag, "I2S parallel driver setup failed: %s", esp_err_to_name(result));
    return false;
  }

  buffer_ready_ = false;
  buffer_busy_ = false;
  clear_frame_ = false;
  automatic_vsync_ = false;
  current_register_ = 0;
  current_suffix_ = 0;
  rows_to_send_ = 0;
  dma_interrupt_count_ = 0;
  current_color_ = 0;

  active_panel = this;
  icn2053_transport_set_callback(transport_callback);
  initialized_ = true;

  const esp_err_t start_result = icn2053_transport_start(suffix_descriptors_[0]);
  if (start_result != ESP_OK) {
    ESP_LOGE(kTag, "I2S DMA start failed: %s", esp_err_to_name(start_result));
    initialized_ = false;
    last_error_ = start_result;
    return false;
  }

  if (!initialize_driver())
    return false;
  set_panel_brightness(kIcn2053DefaultBrightness);
  return error() == ESP_OK;
}

bool MatrixPanel_I2S_DMA::initialize_driver() {
  if (!wait_dma_ready())
    return false;
  set_vsync(false, true);
  for (std::uint8_t index = 0; index < ICN2053_REGISTER_COUNT; ++index) {
    if (!send_vsync())
      return false;
  }
  if (!wait_dma_ready())
    return false;
  portENTER_CRITICAL(&state_mutex_);
  clear_frame_ = true;
  portEXIT_CRITICAL(&state_mutex_);
  if (!send_frame(true))
    return false;
  portENTER_CRITICAL(&state_mutex_);
  clear_frame_ = true;
  portEXIT_CRITICAL(&state_mutex_);
  if (!send_frame(true))
    return false;
  set_vsync(true, true);
  return send_vsync();
}

void IRAM_ATTR MatrixPanel_I2S_DMA::prepare_dma_rows(std::uint8_t row_offset) {
  if (clear_frame_) {
    return;
  }

  for (std::uint8_t row_buffer = 0; row_buffer < dma_.row_count; ++row_buffer) {
    if (row_offset >= rows_per_frame_) {
      row_offset = static_cast<std::uint8_t>(row_offset - rows_per_frame_);
    }

    const std::size_t upper_row = static_cast<std::size_t>(row_offset) * pixels_per_row_;
    const std::size_t lower_row = upper_row + frame_pixel_count_ / kIcn2053RowsInParallel;
    int dma_offset = 0;
    int next_driver_offset = 0;
    int driver_bit = kIcn2053DriverBits;
    int x = pixels_per_row_ - 1;

    for (std::uint16_t pixel = 0; pixel < pixels_per_row_; ++pixel) {
      std::uint8_t upper_red;
      std::uint8_t upper_green;
      std::uint8_t upper_blue;
      std::uint8_t lower_red;
      std::uint8_t lower_green;
      std::uint8_t lower_blue;
      rgb888(frame_buffer_[upper_row + x], upper_red, upper_green, upper_blue);
      rgb888(frame_buffer_[lower_row + x], lower_red, lower_green, lower_blue);

      std::uint16_t upper_channels[3] = {
          brightness_table_[upper_red],
          brightness_table_[upper_green],
          brightness_table_[upper_blue],
      };
      std::uint16_t lower_channels[3] = {
          brightness_table_[lower_red],
          brightness_table_[lower_green],
          brightness_table_[lower_blue],
      };

      for (int bit = 0; bit < kIcn2053DriverBits; ++bit) {
        std::uint16_t rgb = 0;
        if ((upper_channels[0] & 0x8000U) != 0) {
          rgb |= kIcn2053BitR2;
        }
        if ((upper_channels[1] & 0x8000U) != 0) {
          rgb |= kIcn2053BitG2;
        }
        if ((upper_channels[2] & 0x8000U) != 0) {
          rgb |= kIcn2053BitB2;
        }
        if ((lower_channels[0] & 0x8000U) != 0) {
          rgb |= kIcn2053BitR1;
        }
        if ((lower_channels[1] & 0x8000U) != 0) {
          rgb |= kIcn2053BitG1;
        }
        if ((lower_channels[2] & 0x8000U) != 0) {
          rgb |= kIcn2053BitB1;
        }

        auto &destination = dma_.rows[row_buffer][dma_offset++ ^ 1];
        destination = static_cast<std::uint16_t>((destination & ~kIcn2053RgbMask) | rgb);
        for (int channel = 0; channel < 3; ++channel) {
          upper_channels[channel] <<= 1;
          lower_channels[channel] <<= 1;
        }
      }

      if (--driver_bit == 0) {
        driver_bit = kIcn2053DriverBits;
        next_driver_offset += kIcn2053DriverBits;
        dma_offset = next_driver_offset;
      } else {
        dma_offset += pixels_per_row_ + kIcn2053SubrowPadding - kIcn2053DriverBits;
      }
      x = x == 0 ? pixels_per_row_ - 1 : x - 1;
    }

    row_offset =
        row_offset == 0 ? static_cast<std::uint8_t>(rows_per_frame_ - 1) : static_cast<std::uint8_t>(row_offset - 1);
  }
}

void IRAM_ATTR MatrixPanel_I2S_DMA::send_vsync_callback() {
  set_register(current_register_, &driver_registers_[current_register_]);
  current_register_ = static_cast<std::uint8_t>((current_register_ + 1) % ICN2053_REGISTER_COUNT);

  extended_descriptors_[ICN2053_EXT_PREFIX].qe.stqe_next = &suffix_descriptors_[current_suffix_][prefix_to_suffix_];
  suffix_descriptors_[current_suffix_][suffix_descriptor_count_ - 1].qe.stqe_next =
      suffix_descriptors_[current_suffix_];
  suffix_descriptors_[current_suffix_][suffix_descriptor_count_ - 1].eof = 1;
  current_suffix_ ^= 1;
  suffix_descriptors_[current_suffix_][suffix_descriptor_count_ - 1].qe.stqe_next = prefix_descriptors_;
  dma_interrupt_count_ = -1;
}

void IRAM_ATTR MatrixPanel_I2S_DMA::send_row_callback() {
  extended_descriptors_[ICN2053_EXT_DATA].qe.stqe_next = &suffix_descriptors_[current_suffix_][data_to_suffix_];
  suffix_descriptors_[current_suffix_][suffix_descriptor_count_ - 1].qe.stqe_next =
      suffix_descriptors_[current_suffix_];
  suffix_descriptors_[current_suffix_][suffix_descriptor_count_ - 1].eof = 1;
  current_suffix_ ^= 1;
  suffix_descriptors_[current_suffix_][suffix_descriptor_count_ - 1].qe.stqe_next = data_descriptors_;
  dma_interrupt_count_ = -1;
}

void IRAM_ATTR MatrixPanel_I2S_DMA::sendCallback() {
  portENTER_CRITICAL_ISR(&state_mutex_);
  if (!initialized_ || buffer_busy_ || buffer_ready_ || dma_interrupt_count_ > 0) {
    portEXIT_CRITICAL_ISR(&state_mutex_);
    return;
  }

  buffer_busy_ = true;
  dma_interrupt_count_ = dma_interrupt_count_ + 1;
  if (dma_interrupt_count_ > 0) {
    suffix_descriptors_[current_suffix_ ^ 1][suffix_descriptor_count_ - 1].eof = 0;
    if (rows_to_send_ > 0) {
      prepare_dma_rows(static_cast<std::uint8_t>(rows_to_send_ - 1));
      send_row_callback();
      rows_to_send_ = static_cast<std::int16_t>(rows_to_send_ - dma_.row_count);
      if (rows_to_send_ <= 0) {
        clear_frame_ = false;
        rows_to_send_ = automatic_vsync_ ? 0 : -1;
        dma_interrupt_count_ = -4;
      }
    } else if (rows_to_send_ == 0) {
      send_vsync_callback();
      rows_to_send_ = -1;
    } else {
      buffer_ready_ = true;
    }
  }
  buffer_busy_ = false;
  portEXIT_CRITICAL_ISR(&state_mutex_);
}

bool MatrixPanel_I2S_DMA::wait_dma_ready() {
  const std::int64_t deadline_us = esp_timer_get_time() + 1'000'000;
  while (true) {
    portENTER_CRITICAL(&state_mutex_);
    const bool initialized = initialized_;
    const bool ready = buffer_ready_;
    portEXIT_CRITICAL(&state_mutex_);
    if (!initialized)
      return false;
    if (ready)
      return true;
    if (esp_timer_get_time() >= deadline_us) {
      portENTER_CRITICAL(&state_mutex_);
      last_error_ = ESP_ERR_TIMEOUT;
      initialized_ = false;
      portEXIT_CRITICAL(&state_mutex_);
      icn2053_transport_stop();
      ESP_LOGE(kTag, "ICN2053 DMA did not complete within 1000 ms");
      return false;
    }
    vTaskDelay(1);
  }
}

bool MatrixPanel_I2S_DMA::send_vsync() {
  if (!wait_dma_ready())
    return false;
  portENTER_CRITICAL(&state_mutex_);
  buffer_ready_ = false;
  buffer_busy_ = true;
  send_vsync_callback();
  buffer_busy_ = false;
  portEXIT_CRITICAL(&state_mutex_);
  return true;
}

bool MatrixPanel_I2S_DMA::send_frame(const bool wait, const bool automatic_vsync) {
  if (!wait_dma_ready())
    return false;
  portENTER_CRITICAL(&state_mutex_);
  buffer_ready_ = false;
  dma_interrupt_count_ = 0;
  automatic_vsync_ = automatic_vsync;
  suffix_descriptors_[current_suffix_ ^ 1][suffix_descriptor_count_ - 1].eof = 1;
  rows_to_send_ = rows_per_frame_;
  portEXIT_CRITICAL(&state_mutex_);
  return !wait || wait_dma_ready();
}

bool MatrixPanel_I2S_DMA::panel_off() {
  if (!wait_dma_ready())
    return false;
  set_vsync(false, false);
  if (!send_vsync() || !wait_dma_ready())
    return false;
  set_vsync(false, true);
  return true;
}

void MatrixPanel_I2S_DMA::present() {
  if (initialized_) {
    send_frame(true);
  }
}

void MatrixPanel_I2S_DMA::set_panel_brightness(const std::uint8_t brightness) {
  if (!initialized_ || brightness_table_ == nullptr) {
    return;
  }
  if (!wait_dma_ready())
    return;

  const auto maximum = static_cast<std::uint16_t>(kLuminance[brightness] | (kLuminance[brightness] << 8));
  for (std::size_t value = 0; value < kBrightnessTableSize; ++value) {
    brightness_table_[value] = static_cast<std::uint16_t>(maximum * value / (kBrightnessTableSize - 1));
  }
}

void MatrixPanel_I2S_DMA::setBrightness8(const std::uint8_t brightness) { set_panel_brightness(brightness); }

void MatrixPanel_I2S_DMA::fill_rectangle(std::int16_t x, std::int16_t y, const std::int16_t width,
                                         const std::int16_t height) {
  if (!initialized_ || width <= 0 || height <= 0) {
    return;
  }

  auto right = static_cast<std::int16_t>(x + width);
  auto bottom = static_cast<std::int16_t>(y + height);
  x = std::max<std::int16_t>(x, 0);
  y = std::max<std::int16_t>(y, 0);
  right = std::min<std::int16_t>(right, static_cast<std::int16_t>(pixels_per_row_));
  bottom = std::min<std::int16_t>(bottom, static_cast<std::int16_t>(configuration_.panel_height));
  if (x >= right || y >= bottom) {
    return;
  }

  if (!wait_dma_ready())
    return;
  for (std::int16_t row = y; row < bottom; ++row) {
    auto *destination = frame_buffer_ + static_cast<std::size_t>(row) * pixels_per_row_ + x;
    std::fill_n(destination, (right - x), current_color_);
  }
}

}  // namespace esphome::hub75_icn2053
