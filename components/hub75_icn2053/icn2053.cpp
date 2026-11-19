#include "icn2053.h"

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "esp_attr.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esphome/core/log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome::hub75_icn2053 {

namespace {
ESPHOME_LOG_TAG(TAG, "hub75_icn2053.driver");
constexpr size_t BRIGHTNESS_TABLE_SIZE = 256;

constexpr uint8_t LUMINANCE[BRIGHTNESS_TABLE_SIZE] = {
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

uint16_t rgb565(const uint8_t red, const uint8_t green, const uint8_t blue) {
  return static_cast<uint16_t>(((red & 0xf8U) << 8U) | ((green & 0xfcU) << 3U) | (blue >> 3U));
}

void IRAM_ATTR rgb888(const uint16_t color, uint8_t &red, uint8_t &green, uint8_t &blue) {
  red = static_cast<uint8_t>(((color >> 11U) * 527U + 23U) >> 6U);
  green = static_cast<uint8_t>((((color >> 5U) & 0x3fU) * 259U + 33U) >> 6U);
  blue = static_cast<uint8_t>(((color & 0x1fU) * 527U + 23U) >> 6U);
}

ICN2053Panel *active_panel;

uint16_t descriptors_for(const size_t buffer_length, const uint8_t buffer_count) {
  const auto bytes = buffer_length * sizeof(uint16_t);
  const auto per_buffer = (bytes + ICN2053_DMA_MAX_BYTES - 1) / ICN2053_DMA_MAX_BYTES;
  return static_cast<uint16_t>(per_buffer * buffer_count);
}

lldesc_t *link_buffer(lldesc_t *descriptor, lldesc_t *previous, uint8_t *buffer, size_t bytes) {
  while (bytes > 0) {
    const auto chunk = std::min<size_t>(bytes, ICN2053_DMA_MAX_BYTES);
    icn2053_link_dma_descriptor(descriptor, previous, buffer, chunk);
    previous = descriptor++;
    buffer += chunk;
    bytes -= chunk;
  }
  return previous;
}

void link_buffers(lldesc_t *descriptors, uint16_t **buffers, const uint8_t buffer_count, const size_t buffer_length) {
  lldesc_t *previous = nullptr;
  auto *next = descriptors;
  const auto bytes = buffer_length * sizeof(uint16_t);
  for (uint8_t index = 0; index < buffer_count; ++index) {
    previous = link_buffer(next, previous, reinterpret_cast<uint8_t *>(buffers[index]), bytes);
    next = previous + 1;
  }
}

void IRAM_ATTR transport_callback() {
  if (active_panel != nullptr) {
    active_panel->send_callback();
  }
}
}  // namespace

ICN2053Panel::ICN2053Panel(const Hub75Config &configuration) : configuration_(configuration) {}

ICN2053Panel::~ICN2053Panel() { this->end(); }

void ICN2053Panel::set_color_(const uint8_t red, const uint8_t green, const uint8_t blue) {
  this->current_color_ = rgb565(red, green, blue);
}

void ICN2053Panel::fill(const uint8_t red, const uint8_t green, const uint8_t blue) {
  this->set_color_(red, green, blue);
  this->fill_rectangle_(0, 0, static_cast<int16_t>(this->pixels_per_row_),
                        static_cast<int16_t>(this->configuration_.panel_height));
}

void ICN2053Panel::fill_rect(const int16_t x, const int16_t y, const int16_t width, const int16_t height,
                             const uint8_t red, const uint8_t green, const uint8_t blue) {
  this->set_color_(red, green, blue);
  this->fill_rectangle_(x, y, width, height);
}

bool ICN2053Panel::begin() {
  if (this->initialized_) {
    return true;
  }

  this->last_error_ = ESP_OK;
  this->pixels_per_row_ = static_cast<uint16_t>(this->configuration_.panel_width * this->configuration_.chain_length_x *
                                                this->configuration_.chain_length_y);
  this->rows_per_frame_ = static_cast<uint8_t>(this->configuration_.panel_height / ICN2053_ROWS_IN_PARALLEL);

  if (this->pixels_per_row_ == 0 || this->rows_per_frame_ == 0 || (this->pixels_per_row_ % ICN2053_DRIVER_BITS) != 0) {
    this->last_error_ = ESP_ERR_INVALID_ARG;
    ESP_LOGE(TAG, "Invalid geometry: width=%d height=%d chain=%dx%d", this->configuration_.panel_width,
             this->configuration_.panel_height, this->configuration_.chain_length_x,
             this->configuration_.chain_length_y);
    return false;
  }

  this->driver_count_ = this->pixels_per_row_ / ICN2053_DRIVER_BITS;
  if (!this->allocate_memory_()) {
    this->last_error_ = ESP_ERR_NO_MEM;
    this->release_memory_();
    return false;
  }
  if (!this->configure_dma_()) {
    portENTER_CRITICAL(&this->state_mutex_);
    this->initialized_ = false;
    if (this->last_error_ == ESP_OK)
      this->last_error_ = ESP_FAIL;
    portEXIT_CRITICAL(&this->state_mutex_);
    icn2053_transport_set_callback(nullptr);
    icn2053_transport_uninstall();
    if (active_panel == this)
      active_panel = nullptr;
    this->release_memory_();
    return false;
  }

  ESP_LOGI(TAG, "Ready: %dx%d pixels, %d ICN2053 drivers, %" PRIu32 " Hz", this->pixels_per_row_,
           this->configuration_.panel_height, this->driver_count_, this->configuration_.clock_hz);
  return true;
}

void ICN2053Panel::end() {
  if (this->initialized_) {
    this->fill(0, 0, 0);
    this->present();
    this->wait_dma_ready_();
    this->panel_off_();
    this->wait_dma_ready_();
  }

  portENTER_CRITICAL(&this->state_mutex_);
  this->initialized_ = false;
  portEXIT_CRITICAL(&this->state_mutex_);

  icn2053_transport_set_callback(nullptr);
  icn2053_transport_stop();
  icn2053_transport_uninstall();
  if (active_panel == this) {
    active_panel = nullptr;
  }
  this->release_memory_();
}

esp_err_t ICN2053Panel::error() const {
  portENTER_CRITICAL(&this->state_mutex_);
  const esp_err_t result = this->last_error_;
  portEXIT_CRITICAL(&this->state_mutex_);
  return result;
}

void ICN2053Panel::release_memory_() {
  for (auto *row : this->dma_.rows)
    heap_caps_free(row);
  heap_caps_free(this->frame_buffer_);
  heap_caps_free(this->descriptor_storage_);

  this->dma_ = {};
  this->frame_buffer_ = nullptr;
  this->frame_pixel_count_ = 0;
  this->descriptor_storage_ = nullptr;
  this->data_descriptors_ = nullptr;
  this->prefix_descriptors_ = nullptr;
  this->suffix_descriptors_[0] = nullptr;
  this->suffix_descriptors_[1] = nullptr;
  this->extended_descriptors_ = nullptr;
  this->data_descriptor_count_ = 0;
  this->prefix_descriptor_count_ = 0;
  this->suffix_descriptor_count_ = 0;
}

bool ICN2053Panel::allocate_memory_() {
  this->dma_.prefix_length = ICN2053_FRAME_PADDING + ICN2053_PREFIX_START_LENGTH + this->pixels_per_row_;
  this->dma_.suffix_length = ICN2053_ROW_OE_LENGTH * this->rows_per_frame_;
  this->dma_.row_length = (this->pixels_per_row_ + ICN2053_SUBROW_PADDING) * ICN2053_DRIVER_BITS + ICN2053_ROW_PADDING;
  this->dma_.row_count = std::min<uint8_t>(ICN2053_DMA_ROW_BUFFER_COUNT, this->rows_per_frame_);
  this->dma_.total_count = this->dma_.row_count + ICN2053_PREFIX_BUFFER_COUNT + ICN2053_SUFFIX_BUFFER_COUNT;
  this->frame_pixel_count_ = static_cast<size_t>(this->pixels_per_row_) * this->configuration_.panel_height;

  this->data_descriptor_count_ = descriptors_for(this->dma_.row_length, this->dma_.row_count);
  this->prefix_descriptor_count_ = descriptors_for(this->dma_.prefix_length, ICN2053_PREFIX_BUFFER_COUNT);
  this->suffix_descriptor_count_ = descriptors_for(this->dma_.suffix_length, ICN2053_SUFFIX_BUFFER_COUNT);
  const size_t descriptor_count = this->data_descriptor_count_ + this->prefix_descriptor_count_ +
                                  this->suffix_descriptor_count_ * ICN2053_SUFFIX_DESCRIPTOR_SETS + ICN2053_EXT_COUNT;

  const size_t dma_buffer_bytes =
      (this->dma_.row_length * this->dma_.row_count + this->dma_.prefix_length + this->dma_.suffix_length) *
      sizeof(uint16_t);
  const size_t descriptor_bytes = descriptor_count * sizeof(lldesc_t);

  ESP_LOGI(TAG, "Allocating %zu B DMA buffers, %zu B descriptors, %zu B framebuffer", dma_buffer_bytes,
           descriptor_bytes, this->frame_pixel_count_ * sizeof(uint16_t));

  for (uint8_t index = 0; index < this->dma_.total_count; ++index) {
    size_t length = this->dma_.row_length;
    if (index == this->dma_.row_count) {
      length = this->dma_.prefix_length;
    } else if (index > this->dma_.row_count) {
      length = this->dma_.suffix_length;
    }

    this->dma_.rows[index] =
        static_cast<uint16_t *>(heap_caps_calloc(length, sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    if (this->dma_.rows[index] == nullptr) {
      ESP_LOGE(TAG, "Unable to allocate DMA buffer %d (%zu bytes)", index, length * sizeof(uint16_t));
      return false;
    }
  }

  this->frame_buffer_ = static_cast<uint16_t *>(
      heap_caps_calloc(this->frame_pixel_count_, sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  this->brightness_table_.fill(0);
  this->descriptor_storage_ = static_cast<lldesc_t *>(
      heap_caps_calloc(descriptor_count, sizeof(lldesc_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));

  if (this->frame_buffer_ == nullptr || this->descriptor_storage_ == nullptr) {
    ESP_LOGE(TAG, "Unable to allocate framebuffer or descriptors");
    return false;
  }

  this->data_descriptors_ = this->descriptor_storage_;
  this->prefix_descriptors_ = this->data_descriptors_ + this->data_descriptor_count_;
  this->suffix_descriptors_[0] = this->prefix_descriptors_ + this->prefix_descriptor_count_;
  this->suffix_descriptors_[1] = this->suffix_descriptors_[0] + this->suffix_descriptor_count_;
  this->extended_descriptors_ = this->suffix_descriptors_[1] + this->suffix_descriptor_count_;
  return true;
}

bool ICN2053Panel::configure_dma_() {
  link_buffers(this->prefix_descriptors_, &this->dma_.rows[this->dma_.row_count], ICN2053_PREFIX_BUFFER_COUNT,
               this->dma_.prefix_length);
  link_buffers(this->suffix_descriptors_[0], &this->dma_.rows[this->dma_.row_count + 1], ICN2053_SUFFIX_BUFFER_COUNT,
               this->dma_.suffix_length);
  link_buffers(this->suffix_descriptors_[1], &this->dma_.rows[this->dma_.row_count + 1], ICN2053_SUFFIX_BUFFER_COUNT,
               this->dma_.suffix_length);
  link_buffers(this->data_descriptors_, this->dma_.rows.data(), this->dma_.row_count, this->dma_.row_length);

  for (uint8_t index = 0; index < ICN2053_EXT_COUNT; ++index) {
    icn2053_link_dma_descriptor(&this->extended_descriptors_[index], nullptr, nullptr, 0);
  }

  std::copy_n(ICN2053_REGISTER_DEFAULTS, this->driver_registers_.size(), this->driver_registers_.begin());
  set_icn2053_register_rgb(&this->driver_registers_[ICN2053_CFG1],
                           static_cast<ICN2053RegisterValue>(this->rows_per_frame_ - 1), ICN2053_CFG1_LINE_COUNT_MASK,
                           ICN2053_CFG1_LINE_COUNT_OFFSET);
  this->initialize_dma_buffers_();

  icn2053_transport_config_t transport_configuration{};
  const int data_pins[16] = {
      this->configuration_.gpio.r1,
      this->configuration_.gpio.g1,
      this->configuration_.gpio.b1,
      this->configuration_.gpio.r2,
      this->configuration_.gpio.g2,
      this->configuration_.gpio.b2,
      this->configuration_.gpio.lat,
      this->configuration_.gpio.oe,
      this->configuration_.gpio.a,
      this->configuration_.gpio.b,
      this->configuration_.gpio.c,
      this->configuration_.gpio.d,
      this->configuration_.gpio.e,
      -1,
      -1,
      -1,
  };
  std::copy_n(data_pins, 16, transport_configuration.gpio_bus);
  transport_configuration.gpio_clk = this->configuration_.gpio.clk;
  transport_configuration.clock_hz = this->configuration_.clock_hz;
  transport_configuration.clock_phase_inverted = this->configuration_.clock_phase_inverted;

  const esp_err_t result = icn2053_transport_install(&transport_configuration);
  if (result != ESP_OK) {
    this->last_error_ = result;
    ESP_LOGE(TAG, "I2S parallel driver setup failed: %s", esp_err_to_name(result));
    return false;
  }

  this->buffer_ready_ = false;
  this->buffer_busy_ = false;
  this->clear_frame_ = false;
  this->automatic_vsync_ = false;
  this->current_register_ = 0;
  this->current_suffix_ = 0;
  this->rows_to_send_ = 0;
  this->dma_interrupt_count_ = 0;
  this->current_color_ = 0;

  active_panel = this;
  icn2053_transport_set_callback(transport_callback);
  this->initialized_ = true;

  const esp_err_t start_result = icn2053_transport_start(this->suffix_descriptors_[0]);
  if (start_result != ESP_OK) {
    ESP_LOGE(TAG, "I2S DMA start failed: %s", esp_err_to_name(start_result));
    this->initialized_ = false;
    this->last_error_ = start_result;
    return false;
  }

  if (!this->initialize_driver_())
    return false;
  this->set_panel_brightness_(ICN2053_DEFAULT_BRIGHTNESS);
  return this->error() == ESP_OK;
}

bool ICN2053Panel::initialize_driver_() {
  if (!this->wait_dma_ready_())
    return false;
  this->set_vsync_(false, true);
  for (uint8_t index = 0; index < ICN2053_REGISTER_COUNT; ++index) {
    if (!this->send_vsync_())
      return false;
  }
  if (!this->wait_dma_ready_())
    return false;
  portENTER_CRITICAL(&this->state_mutex_);
  this->clear_frame_ = true;
  portEXIT_CRITICAL(&this->state_mutex_);
  if (!this->send_frame_(true))
    return false;
  portENTER_CRITICAL(&this->state_mutex_);
  this->clear_frame_ = true;
  portEXIT_CRITICAL(&this->state_mutex_);
  if (!this->send_frame_(true))
    return false;
  this->set_vsync_(true, true);
  return this->send_vsync_();
}

void IRAM_ATTR ICN2053Panel::prepare_dma_rows_(uint8_t row_offset) {
  if (this->clear_frame_) {
    return;
  }

  for (uint8_t row_buffer = 0; row_buffer < this->dma_.row_count; ++row_buffer) {
    if (row_offset >= this->rows_per_frame_) {
      row_offset = static_cast<uint8_t>(row_offset - this->rows_per_frame_);
    }

    const size_t upper_row = static_cast<size_t>(row_offset) * this->pixels_per_row_;
    const size_t lower_row = upper_row + this->frame_pixel_count_ / ICN2053_ROWS_IN_PARALLEL;
    int dma_offset = 0;
    int next_driver_offset = 0;
    int driver_bit = ICN2053_DRIVER_BITS;
    int x = this->pixels_per_row_ - 1;

    for (uint16_t pixel = 0; pixel < this->pixels_per_row_; ++pixel) {
      uint8_t upper_red;
      uint8_t upper_green;
      uint8_t upper_blue;
      uint8_t lower_red;
      uint8_t lower_green;
      uint8_t lower_blue;
      rgb888(this->frame_buffer_[upper_row + x], upper_red, upper_green, upper_blue);
      rgb888(this->frame_buffer_[lower_row + x], lower_red, lower_green, lower_blue);

      uint16_t upper_channels[3] = {
          this->brightness_table_[upper_red],
          this->brightness_table_[upper_green],
          this->brightness_table_[upper_blue],
      };
      uint16_t lower_channels[3] = {
          this->brightness_table_[lower_red],
          this->brightness_table_[lower_green],
          this->brightness_table_[lower_blue],
      };

      for (int bit = 0; bit < ICN2053_DRIVER_BITS; ++bit) {
        uint16_t rgb = 0;
        if ((upper_channels[0] & 0x8000U) != 0) {
          rgb |= ICN2053_BIT_R2;
        }
        if ((upper_channels[1] & 0x8000U) != 0) {
          rgb |= ICN2053_BIT_G2;
        }
        if ((upper_channels[2] & 0x8000U) != 0) {
          rgb |= ICN2053_BIT_B2;
        }
        if ((lower_channels[0] & 0x8000U) != 0) {
          rgb |= ICN2053_BIT_R1;
        }
        if ((lower_channels[1] & 0x8000U) != 0) {
          rgb |= ICN2053_BIT_G1;
        }
        if ((lower_channels[2] & 0x8000U) != 0) {
          rgb |= ICN2053_BIT_B1;
        }

        auto &destination = this->dma_.rows[row_buffer][dma_offset++ ^ 1];
        destination = static_cast<uint16_t>((destination & ~ICN2053_RGB_MASK) | rgb);
        for (int channel = 0; channel < 3; ++channel) {
          upper_channels[channel] <<= 1;
          lower_channels[channel] <<= 1;
        }
      }

      if (--driver_bit == 0) {
        driver_bit = ICN2053_DRIVER_BITS;
        next_driver_offset += ICN2053_DRIVER_BITS;
        dma_offset = next_driver_offset;
      } else {
        dma_offset += this->pixels_per_row_ + ICN2053_SUBROW_PADDING - ICN2053_DRIVER_BITS;
      }
      x = x == 0 ? this->pixels_per_row_ - 1 : x - 1;
    }

    row_offset =
        row_offset == 0 ? static_cast<uint8_t>(this->rows_per_frame_ - 1) : static_cast<uint8_t>(row_offset - 1);
  }
}

void IRAM_ATTR ICN2053Panel::send_vsync_callback_() {
  this->set_register_(this->current_register_, &this->driver_registers_[this->current_register_]);
  this->current_register_ = static_cast<uint8_t>((this->current_register_ + 1) % ICN2053_REGISTER_COUNT);

  this->extended_descriptors_[ICN2053_EXT_PREFIX].qe.stqe_next =
      &this->suffix_descriptors_[this->current_suffix_][this->prefix_to_suffix_];
  this->suffix_descriptors_[this->current_suffix_][this->suffix_descriptor_count_ - 1].qe.stqe_next =
      this->suffix_descriptors_[this->current_suffix_];
  this->suffix_descriptors_[this->current_suffix_][this->suffix_descriptor_count_ - 1].eof = 1;
  this->current_suffix_ ^= 1;
  this->suffix_descriptors_[this->current_suffix_][this->suffix_descriptor_count_ - 1].qe.stqe_next =
      this->prefix_descriptors_;
  this->dma_interrupt_count_ = -1;
}

void IRAM_ATTR ICN2053Panel::send_row_callback_() {
  this->extended_descriptors_[ICN2053_EXT_DATA].qe.stqe_next =
      &this->suffix_descriptors_[this->current_suffix_][this->data_to_suffix_];
  this->suffix_descriptors_[this->current_suffix_][this->suffix_descriptor_count_ - 1].qe.stqe_next =
      this->suffix_descriptors_[this->current_suffix_];
  this->suffix_descriptors_[this->current_suffix_][this->suffix_descriptor_count_ - 1].eof = 1;
  this->current_suffix_ ^= 1;
  this->suffix_descriptors_[this->current_suffix_][this->suffix_descriptor_count_ - 1].qe.stqe_next =
      this->data_descriptors_;
  this->dma_interrupt_count_ = -1;
}

void IRAM_ATTR ICN2053Panel::send_callback() {
  portENTER_CRITICAL_ISR(&this->state_mutex_);
  if (!this->initialized_ || this->buffer_busy_ || this->buffer_ready_ || this->dma_interrupt_count_ > 0) {
    portEXIT_CRITICAL_ISR(&this->state_mutex_);
    return;
  }

  this->buffer_busy_ = true;
  this->dma_interrupt_count_ = this->dma_interrupt_count_ + 1;
  if (this->dma_interrupt_count_ > 0) {
    this->suffix_descriptors_[this->current_suffix_ ^ 1][this->suffix_descriptor_count_ - 1].eof = 0;
    if (this->rows_to_send_ > 0) {
      this->prepare_dma_rows_(static_cast<uint8_t>(this->rows_to_send_ - 1));
      this->send_row_callback_();
      this->rows_to_send_ = static_cast<int16_t>(this->rows_to_send_ - this->dma_.row_count);
      if (this->rows_to_send_ <= 0) {
        this->clear_frame_ = false;
        this->rows_to_send_ = this->automatic_vsync_ ? 0 : -1;
        this->dma_interrupt_count_ = -4;
      }
    } else if (this->rows_to_send_ == 0) {
      this->send_vsync_callback_();
      this->rows_to_send_ = -1;
    } else {
      this->buffer_ready_ = true;
    }
  }
  this->buffer_busy_ = false;
  portEXIT_CRITICAL_ISR(&this->state_mutex_);
}

bool ICN2053Panel::wait_dma_ready_() {
  const int64_t deadline_us = esp_timer_get_time() + 1'000'000;
  while (true) {
    portENTER_CRITICAL(&this->state_mutex_);
    const bool initialized = this->initialized_;
    const bool ready = this->buffer_ready_;
    portEXIT_CRITICAL(&this->state_mutex_);
    if (!initialized)
      return false;
    if (ready)
      return true;
    if (esp_timer_get_time() >= deadline_us) {
      portENTER_CRITICAL(&this->state_mutex_);
      this->last_error_ = ESP_ERR_TIMEOUT;
      this->initialized_ = false;
      portEXIT_CRITICAL(&this->state_mutex_);
      icn2053_transport_stop();
      ESP_LOGE(TAG, "ICN2053 DMA did not complete within 1000 ms");
      return false;
    }
    vTaskDelay(1);
  }
}

bool ICN2053Panel::send_vsync_() {
  if (!this->wait_dma_ready_())
    return false;
  portENTER_CRITICAL(&this->state_mutex_);
  this->buffer_ready_ = false;
  this->buffer_busy_ = true;
  this->send_vsync_callback_();
  this->buffer_busy_ = false;
  portEXIT_CRITICAL(&this->state_mutex_);
  return true;
}

bool ICN2053Panel::send_frame_(const bool wait, const bool automatic_vsync) {
  if (!this->wait_dma_ready_())
    return false;
  portENTER_CRITICAL(&this->state_mutex_);
  this->buffer_ready_ = false;
  this->dma_interrupt_count_ = 0;
  this->automatic_vsync_ = automatic_vsync;
  this->suffix_descriptors_[this->current_suffix_ ^ 1][this->suffix_descriptor_count_ - 1].eof = 1;
  this->rows_to_send_ = this->rows_per_frame_;
  portEXIT_CRITICAL(&this->state_mutex_);
  return !wait || this->wait_dma_ready_();
}

bool ICN2053Panel::panel_off_() {
  if (!this->wait_dma_ready_())
    return false;
  this->set_vsync_(false, false);
  if (!this->send_vsync_() || !this->wait_dma_ready_())
    return false;
  this->set_vsync_(false, true);
  return true;
}

void ICN2053Panel::present() {
  if (this->initialized_) {
    this->send_frame_(true);
  }
}

void ICN2053Panel::set_panel_brightness_(const uint8_t brightness) {
  if (!this->initialized_) {
    return;
  }
  if (!this->wait_dma_ready_())
    return;

  const auto maximum = static_cast<uint16_t>(LUMINANCE[brightness] | (LUMINANCE[brightness] << 8));
  for (size_t value = 0; value < BRIGHTNESS_TABLE_SIZE; ++value) {
    this->brightness_table_[value] = static_cast<uint16_t>(maximum * value / (BRIGHTNESS_TABLE_SIZE - 1));
  }
}

void ICN2053Panel::set_brightness(const uint8_t brightness) { this->set_panel_brightness_(brightness); }

void ICN2053Panel::fill_rectangle_(int16_t x, int16_t y, const int16_t width, const int16_t height) {
  if (!this->initialized_ || width <= 0 || height <= 0) {
    return;
  }

  auto right = static_cast<int16_t>(x + width);
  auto bottom = static_cast<int16_t>(y + height);
  x = std::max<int16_t>(x, 0);
  y = std::max<int16_t>(y, 0);
  right = std::min<int16_t>(right, static_cast<int16_t>(this->pixels_per_row_));
  bottom = std::min<int16_t>(bottom, static_cast<int16_t>(this->configuration_.panel_height));
  if (x >= right || y >= bottom) {
    return;
  }

  if (!this->wait_dma_ready_())
    return;
  for (int16_t row = y; row < bottom; ++row) {
    auto *destination = this->frame_buffer_ + static_cast<size_t>(row) * this->pixels_per_row_ + x;
    std::fill_n(destination, (right - x), this->current_color_);
  }
}

}  // namespace esphome::hub75_icn2053
