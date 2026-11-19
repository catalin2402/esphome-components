#include "icn2053_registers.h"

#include <cstddef>

#include "esp_attr.h"
#include "icn2053.h"

namespace esphome::hub75_icn2053 {

DRAM_ATTR const uint8_t ICN2053_REGISTER_COMMANDS[ICN2053_REGISTER_COUNT] = {
    ICN2053_WR_CFG1, ICN2053_WR_CFG2, ICN2053_WR_CFG3, ICN2053_WR_CFG4, ICN2053_WR_DEBUG,
};

const ICN2053RegisterRGB ICN2053_REGISTER_DEFAULTS[ICN2053_REGISTER_COUNT] = {
    // Green: coupling optimization mode 2 and ghost suppression code 31.
    {.r = 0x1370, .g = 0x13b0, .b = 0x1370}, {.r = 0x7ddb, .g = 0x7ddb, .b = 0x5ddb},
    {.r = 0x4047, .g = 0x4047, .b = 0x4047}, {.r = 0x0e40, .g = 0x0e40, .b = 0x0e40},
    {.r = 0x0008, .g = 0x0008, .b = 0x0008},
};

namespace {
int IRAM_ATTR set_bits(uint16_t *buffer, int offset, const uint16_t mask, int length) {
  while (length-- > 0) {
    buffer[offset++ ^ 1] |= mask;
  }
  return offset;
}

int IRAM_ATTR clear_bits(uint16_t *buffer, int offset, const uint16_t mask, int length) {
  while (length-- > 0) {
    buffer[offset++ ^ 1] &= static_cast<uint16_t>(~mask);
  }
  return offset;
}

int IRAM_ATTR write_register(uint16_t *buffer, int offset, const ICN2053RegisterRGB &value) {
  auto red = static_cast<uint16_t>(value.r);
  auto green = static_cast<uint16_t>(value.g);
  auto blue = static_cast<uint16_t>(value.b);

  for (int bit = 0; bit < ICN2053_DRIVER_BITS; ++bit) {
    uint16_t data = 0;
    if ((red & 0x8000U) != 0) {
      data |= ICN2053_BIT_R1 | ICN2053_BIT_R2;
    }
    if ((green & 0x8000U) != 0) {
      data |= ICN2053_BIT_G1 | ICN2053_BIT_G2;
    }
    if ((blue & 0x8000U) != 0) {
      data |= ICN2053_BIT_B1 | ICN2053_BIT_B2;
    }

    const int index = offset++ ^ 1;
    buffer[index] = static_cast<uint16_t>((buffer[index] & ~ICN2053_RGB_MASK) | data);
    red <<= 1;
    green <<= 1;
    blue <<= 1;
  }
  return offset;
}

uint16_t row_address_bits(uint8_t address) {
  constexpr uint16_t bits[ICN2053_ROW_ADDRESS_BITS] = {
      ICN2053_BIT_A, ICN2053_BIT_B, ICN2053_BIT_C, ICN2053_BIT_D, ICN2053_BIT_E,
  };

  uint16_t result = 0;
  for (const auto bit : bits) {
    if ((address & 1U) != 0) {
      result |= bit;
    }
    address >>= 1;
  }
  return result;
}

void set_latch_pulses(uint16_t *buffer, const int subrow_count, const int subrow_length) {
  int offset = subrow_length - 1;
  for (int subrow = 0; subrow < subrow_count; ++subrow) {
    buffer[offset ^ 1] |= ICN2053_BIT_LAT;
    offset += subrow_length + ICN2053_SUBROW_PADDING;
  }
}

int set_output_enable_and_address(uint16_t *buffer, int offset, const uint8_t row_count, const size_t buffer_length,
                                  int frame_offset) {
  if (frame_offset >= ICN2053_ROW_OE_LENGTH * (1 << ICN2053_ROW_ADDRESS_BITS)) {
    frame_offset = 0;
  }

  auto address = static_cast<uint8_t>(frame_offset / ICN2053_ROW_OE_LENGTH);
  int row_offset = frame_offset % ICN2053_ROW_OE_LENGTH;
  int output_enable_count = row_offset > ICN2053_ROW_OE_COUNT * 2 ? 0 : ICN2053_ROW_OE_COUNT - (row_offset >> 1);
  auto address_data = row_address_bits(address);

  while (offset < static_cast<int>(buffer_length)) {
    if (row_offset == ICN2053_ROW_OE_LENGTH) {
      if (++address >= row_count) {
        address = 0;
        frame_offset = 0;
      }
      address_data = row_address_bits(address);
      row_offset = 0;
      output_enable_count = ICN2053_ROW_OE_COUNT;
    }

    const bool output_disabled = output_enable_count > 0;
    if (output_disabled) {
      --output_enable_count;
    }
    buffer[offset++ ^ 1] = static_cast<uint16_t>(address_data | (output_disabled ? ICN2053_BIT_OE : 0));
    ++row_offset;
    ++frame_offset;

    if (offset >= static_cast<int>(buffer_length)) {
      break;
    }
    buffer[offset++ ^ 1] = address_data;
    ++row_offset;
    ++frame_offset;
  }
  return frame_offset;
}

int write_vsync(uint16_t *buffer, int offset, const bool leds_enabled, const bool vsync) {
  offset = clear_bits(buffer, offset, ICN2053_BIT_LAT, ICN2053_COMMAND_DELAY);
  offset = set_bits(buffer, offset, ICN2053_BIT_LAT, ICN2053_PRE_ACT);
  offset = clear_bits(buffer, offset, ICN2053_BIT_LAT, ICN2053_COMMAND_DELAY);

  int low_length = ICN2053_COMMAND_DELAY;
  if (leds_enabled) {
    offset = set_bits(buffer, offset, ICN2053_BIT_LAT, ICN2053_EN_OP);
    low_length += ICN2053_DIS_OP - ICN2053_EN_OP;
  } else {
    offset = set_bits(buffer, offset, ICN2053_BIT_LAT, ICN2053_DIS_OP);
  }

  offset = clear_bits(buffer, offset, ICN2053_BIT_LAT, low_length);
  offset = vsync ? set_bits(buffer, offset, ICN2053_BIT_LAT, ICN2053_V_SYNC)
                 : clear_bits(buffer, offset, ICN2053_BIT_LAT, ICN2053_V_SYNC);
  offset = clear_bits(buffer, offset, ICN2053_BIT_LAT, ICN2053_COMMAND_DELAY);
  offset = set_bits(buffer, offset, ICN2053_BIT_LAT, ICN2053_PRE_ACT);
  return clear_bits(buffer, offset, ICN2053_BIT_LAT, ICN2053_COMMAND_DELAY);
}
}  // namespace

void set_icn2053_register_rgb(ICN2053RegisterRGB *driver_register, const ICN2053RegisterValue value,
                              const ICN2053RegisterValue mask, const uint8_t offset) {
  const auto update = [=](ICN2053RegisterValue &channel) {
    channel = static_cast<ICN2053RegisterValue>((channel & ~mask) | ((value << offset) & mask));
  };
  update(driver_register->r);
  update(driver_register->g);
  update(driver_register->b);
}

void ICN2053Panel::set_vsync_(const bool leds_enabled, const bool vsync) const {
  write_vsync(this->dma_.rows[this->dma_.row_count], ICN2053_FRAME_PADDING, leds_enabled, vsync);
}

void IRAM_ATTR ICN2053Panel::set_register_(const uint8_t register_index,
                                           const ICN2053RegisterRGB *register_data) const {
  int offset = ICN2053_FRAME_PADDING + ICN2053_PREFIX_START_LENGTH;
  for (uint16_t driver = 0; driver < this->driver_count_; ++driver) {
    offset = write_register(this->dma_.rows[this->dma_.row_count], offset, *register_data);
  }

  clear_bits(this->dma_.rows[this->dma_.row_count], static_cast<int>(this->dma_.prefix_length) - ICN2053_DRIVER_BITS,
             ICN2053_BIT_LAT, ICN2053_DRIVER_BITS);
  set_bits(this->dma_.rows[this->dma_.row_count],
           static_cast<int>(this->dma_.prefix_length) - ICN2053_REGISTER_COMMANDS[register_index], ICN2053_BIT_LAT,
           ICN2053_REGISTER_COMMANDS[register_index]);
}

void ICN2053Panel::initialize_dma_buffers_() {
  int frame_offset = 0;
  for (uint8_t row = 0; row < this->dma_.row_count; ++row) {
    frame_offset = set_output_enable_and_address(this->dma_.rows[row], 0, this->rows_per_frame_, this->dma_.row_length,
                                                 frame_offset);
    set_latch_pulses(this->dma_.rows[row], ICN2053_DRIVER_BITS, this->pixels_per_row_);
  }

  this->data_descriptors_[this->data_descriptor_count_ - 1].eof = 1;
  this->data_descriptors_[this->data_descriptor_count_ - 1].qe.stqe_next =
      &this->extended_descriptors_[ICN2053_EXT_DATA];

  frame_offset %= static_cast<int>(this->dma_.suffix_length);
  const int data_byte_offset = frame_offset * sizeof(uint16_t);
  const int data_suffix_descriptor = data_byte_offset / ICN2053_DMA_MAX_BYTES;
  const int data_descriptor_offset = data_byte_offset % ICN2053_DMA_MAX_BYTES;

  int prefix_offset =
      set_output_enable_and_address(this->dma_.rows[this->dma_.row_count], ICN2053_FRAME_PADDING + ICN2053_VSYNC_LENGTH,
                                    this->rows_per_frame_, this->dma_.prefix_length, 0);
  prefix_offset %= static_cast<int>(this->dma_.suffix_length);
  const int prefix_byte_offset = prefix_offset * sizeof(uint16_t);
  const int prefix_suffix_descriptor = prefix_byte_offset / ICN2053_DMA_MAX_BYTES;
  const int prefix_descriptor_offset = prefix_byte_offset % ICN2053_DMA_MAX_BYTES;

  this->prefix_descriptors_[this->prefix_descriptor_count_ - 1].eof = 1;
  this->prefix_descriptors_[this->prefix_descriptor_count_ - 1].qe.stqe_next =
      &this->extended_descriptors_[ICN2053_EXT_PREFIX];

  set_output_enable_and_address(this->dma_.rows[this->dma_.row_count + 1], 0, this->rows_per_frame_,
                                this->dma_.suffix_length, 0);

  for (auto *suffix : this->suffix_descriptors_) {
    suffix[this->suffix_descriptor_count_ - 1].eof = 1;
    suffix[this->suffix_descriptor_count_ - 1].qe.stqe_next = suffix;
  }

  auto configure_extension = [](lldesc_t &extension, lldesc_t *suffix, const int descriptor_index,
                                const int byte_offset, uint16_t &next_index) {
    const volatile uint8_t *base = suffix[descriptor_index].buf;
    extension.buf = base + byte_offset;
    extension.size = suffix[descriptor_index].size - byte_offset;
    extension.length = extension.size;
    next_index = static_cast<uint16_t>(descriptor_index + 1);
  };

  configure_extension(this->extended_descriptors_[ICN2053_EXT_DATA], this->suffix_descriptors_[0],
                      data_suffix_descriptor, data_descriptor_offset, this->data_to_suffix_);
  if (data_suffix_descriptor == this->suffix_descriptor_count_ - 1) {
    this->data_to_suffix_ = 0;
  }
  this->extended_descriptors_[ICN2053_EXT_DATA].qe.stqe_next = &this->suffix_descriptors_[0][this->data_to_suffix_];

  configure_extension(this->extended_descriptors_[ICN2053_EXT_PREFIX], this->suffix_descriptors_[0],
                      prefix_suffix_descriptor, prefix_descriptor_offset, this->prefix_to_suffix_);
  if (prefix_suffix_descriptor == this->suffix_descriptor_count_ - 1) {
    this->prefix_to_suffix_ = 0;
  }
  this->extended_descriptors_[ICN2053_EXT_PREFIX].qe.stqe_next = &this->suffix_descriptors_[0][this->prefix_to_suffix_];
}

}  // namespace esphome::hub75_icn2053
