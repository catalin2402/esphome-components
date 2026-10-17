#include "icn2053_registers.h"

#include <cstddef>

#include "esp_attr.h"
#include "icn2053.h"

namespace esphome::hub75_icn2053 {

DRAM_ATTR const std::uint8_t kIcn2053RegisterCommands[ICN2053_REGISTER_COUNT] = {
    ICN2053_WR_CFG1, ICN2053_WR_CFG2, ICN2053_WR_CFG3, ICN2053_WR_CFG4, ICN2053_WR_DEBUG,
};

const driver_rgb_t kIcn2053RegisterDefaults[ICN2053_REGISTER_COUNT] = {
    // Green: coupling optimization mode 2 and ghost suppression code 31.
    {.r = 0x1370, .g = 0x13b0, .b = 0x1370}, {.r = 0x7ddb, .g = 0x7ddb, .b = 0x5ddb},
    {.r = 0x4047, .g = 0x4047, .b = 0x4047}, {.r = 0x0e40, .g = 0x0e40, .b = 0x0e40},
    {.r = 0x0008, .g = 0x0008, .b = 0x0008},
};

namespace {
int IRAM_ATTR set_bits(std::uint16_t *buffer, int offset, const std::uint16_t mask, int length) {
  while (length-- > 0) {
    buffer[offset++ ^ 1] |= mask;
  }
  return offset;
}

int IRAM_ATTR clear_bits(std::uint16_t *buffer, int offset, const std::uint16_t mask, int length) {
  while (length-- > 0) {
    buffer[offset++ ^ 1] &= static_cast<std::uint16_t>(~mask);
  }
  return offset;
}

int IRAM_ATTR write_register(std::uint16_t *buffer, int offset, const driver_rgb_t &value) {
  auto red = static_cast<std::uint16_t>(value.r);
  auto green = static_cast<std::uint16_t>(value.g);
  auto blue = static_cast<std::uint16_t>(value.b);

  for (int bit = 0; bit < kIcn2053DriverBits; ++bit) {
    std::uint16_t data = 0;
    if ((red & 0x8000U) != 0) {
      data |= kIcn2053BitR1 | kIcn2053BitR2;
    }
    if ((green & 0x8000U) != 0) {
      data |= kIcn2053BitG1 | kIcn2053BitG2;
    }
    if ((blue & 0x8000U) != 0) {
      data |= kIcn2053BitB1 | kIcn2053BitB2;
    }

    const int index = offset++ ^ 1;
    buffer[index] = static_cast<std::uint16_t>((buffer[index] & ~kIcn2053RgbMask) | data);
    red <<= 1;
    green <<= 1;
    blue <<= 1;
  }
  return offset;
}

std::uint16_t row_address_bits(std::uint8_t address) {
  constexpr std::uint16_t bits[kIcn2053RowAddressBits] = {
      kIcn2053BitA, kIcn2053BitB, kIcn2053BitC, kIcn2053BitD, kIcn2053BitE,
  };

  std::uint16_t result = 0;
  for (const auto bit : bits) {
    if ((address & 1U) != 0) {
      result |= bit;
    }
    address >>= 1;
  }
  return result;
}

void set_latch_pulses(std::uint16_t *buffer, const int subrow_count, const int subrow_length) {
  int offset = subrow_length - 1;
  for (int subrow = 0; subrow < subrow_count; ++subrow) {
    buffer[offset ^ 1] |= kIcn2053BitLat;
    offset += subrow_length + kIcn2053SubrowPadding;
  }
}

int set_output_enable_and_address(std::uint16_t *buffer, int offset, const std::uint8_t row_count,
                                  const std::size_t buffer_length, int frame_offset) {
  if (frame_offset >= kIcn2053RowOeLength * (1 << kIcn2053RowAddressBits)) {
    frame_offset = 0;
  }

  auto address = static_cast<std::uint8_t>(frame_offset / kIcn2053RowOeLength);
  int row_offset = frame_offset % kIcn2053RowOeLength;
  int output_enable_count = row_offset > kIcn2053RowOeCount * 2 ? 0 : kIcn2053RowOeCount - (row_offset >> 1);
  auto address_data = row_address_bits(address);

  while (offset < static_cast<int>(buffer_length)) {
    if (row_offset == kIcn2053RowOeLength) {
      if (++address >= row_count) {
        address = 0;
        frame_offset = 0;
      }
      address_data = row_address_bits(address);
      row_offset = 0;
      output_enable_count = kIcn2053RowOeCount;
    }

    const bool output_disabled = output_enable_count > 0;
    if (output_disabled) {
      --output_enable_count;
    }
    buffer[offset++ ^ 1] = static_cast<std::uint16_t>(address_data | (output_disabled ? kIcn2053BitOe : 0));
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

int write_vsync(std::uint16_t *buffer, int offset, const bool leds_enabled, const bool vsync) {
  offset = clear_bits(buffer, offset, kIcn2053BitLat, kIcn2053CommandDelay);
  offset = set_bits(buffer, offset, kIcn2053BitLat, ICN2053_PRE_ACT);
  offset = clear_bits(buffer, offset, kIcn2053BitLat, kIcn2053CommandDelay);

  int low_length = kIcn2053CommandDelay;
  if (leds_enabled) {
    offset = set_bits(buffer, offset, kIcn2053BitLat, ICN2053_EN_OP);
    low_length += ICN2053_DIS_OP - ICN2053_EN_OP;
  } else {
    offset = set_bits(buffer, offset, kIcn2053BitLat, ICN2053_DIS_OP);
  }

  offset = clear_bits(buffer, offset, kIcn2053BitLat, low_length);
  offset = vsync ? set_bits(buffer, offset, kIcn2053BitLat, ICN2053_V_SYNC)
                 : clear_bits(buffer, offset, kIcn2053BitLat, ICN2053_V_SYNC);
  offset = clear_bits(buffer, offset, kIcn2053BitLat, kIcn2053CommandDelay);
  offset = set_bits(buffer, offset, kIcn2053BitLat, ICN2053_PRE_ACT);
  return clear_bits(buffer, offset, kIcn2053BitLat, kIcn2053CommandDelay);
}
}

void set_icn2053_register_rgb(driver_rgb_t *driver_register, const driver_reg_t value, const driver_reg_t mask,
                              const std::uint8_t offset) {
  const auto update = [=](driver_reg_t &channel) {
    channel = static_cast<driver_reg_t>((channel & ~mask) | ((value << offset) & mask));
  };
  update(driver_register->r);
  update(driver_register->g);
  update(driver_register->b);
}

void MatrixPanel_I2S_DMA::set_vsync(const bool leds_enabled, const bool vsync) const {
  write_vsync(dma_.rows[dma_.row_count], kIcn2053FramePadding, leds_enabled, vsync);
}

void IRAM_ATTR MatrixPanel_I2S_DMA::set_register(const std::uint8_t register_index,
                                                 const driver_rgb_t *register_data) const {
  int offset = kIcn2053FramePadding + kIcn2053PrefixStartLength;
  for (std::uint16_t driver = 0; driver < driver_count_; ++driver) {
    offset = write_register(dma_.rows[dma_.row_count], offset, *register_data);
  }

  clear_bits(dma_.rows[dma_.row_count], static_cast<int>(dma_.prefix_length) - kIcn2053DriverBits, kIcn2053BitLat,
             kIcn2053DriverBits);
  set_bits(dma_.rows[dma_.row_count], static_cast<int>(dma_.prefix_length) - kIcn2053RegisterCommands[register_index],
           kIcn2053BitLat, kIcn2053RegisterCommands[register_index]);
}

void MatrixPanel_I2S_DMA::initialize_dma_buffers() {
  int frame_offset = 0;
  for (std::uint8_t row = 0; row < dma_.row_count; ++row) {
    frame_offset = set_output_enable_and_address(dma_.rows[row], 0, rows_per_frame_, dma_.row_length, frame_offset);
    set_latch_pulses(dma_.rows[row], kIcn2053DriverBits, pixels_per_row_);
  }

  data_descriptors_[data_descriptor_count_ - 1].eof = 1;
  data_descriptors_[data_descriptor_count_ - 1].qe.stqe_next = &extended_descriptors_[ICN2053_EXT_DATA];

  frame_offset %= static_cast<int>(dma_.suffix_length);
  const int data_byte_offset = frame_offset * sizeof(std::uint16_t);
  const int data_suffix_descriptor = data_byte_offset / ICN2053_DMA_MAX_BYTES;
  const int data_descriptor_offset = data_byte_offset % ICN2053_DMA_MAX_BYTES;

  int prefix_offset = set_output_enable_and_address(
      dma_.rows[dma_.row_count], kIcn2053FramePadding + kIcn2053VsyncLength, rows_per_frame_, dma_.prefix_length, 0);
  prefix_offset %= static_cast<int>(dma_.suffix_length);
  const int prefix_byte_offset = prefix_offset * sizeof(std::uint16_t);
  const int prefix_suffix_descriptor = prefix_byte_offset / ICN2053_DMA_MAX_BYTES;
  const int prefix_descriptor_offset = prefix_byte_offset % ICN2053_DMA_MAX_BYTES;

  prefix_descriptors_[prefix_descriptor_count_ - 1].eof = 1;
  prefix_descriptors_[prefix_descriptor_count_ - 1].qe.stqe_next = &extended_descriptors_[ICN2053_EXT_PREFIX];

  set_output_enable_and_address(dma_.rows[dma_.row_count + 1], 0, rows_per_frame_, dma_.suffix_length, 0);

  for (auto *suffix : suffix_descriptors_) {
    suffix[suffix_descriptor_count_ - 1].eof = 1;
    suffix[suffix_descriptor_count_ - 1].qe.stqe_next = suffix;
  }

  auto configure_extension = [](lldesc_t &extension, lldesc_t *suffix, const int descriptor_index,
                                const int byte_offset, std::uint16_t &next_index) {
    const volatile std::uint8_t *base = suffix[descriptor_index].buf;
    extension.buf = base + byte_offset;
    extension.size = suffix[descriptor_index].size - byte_offset;
    extension.length = extension.size;
    next_index = static_cast<std::uint16_t>(descriptor_index + 1);
  };

  configure_extension(extended_descriptors_[ICN2053_EXT_DATA], suffix_descriptors_[0], data_suffix_descriptor,
                      data_descriptor_offset, data_to_suffix_);
  if (data_suffix_descriptor == suffix_descriptor_count_ - 1) {
    data_to_suffix_ = 0;
  }
  extended_descriptors_[ICN2053_EXT_DATA].qe.stqe_next = &suffix_descriptors_[0][data_to_suffix_];

  configure_extension(extended_descriptors_[ICN2053_EXT_PREFIX], suffix_descriptors_[0], prefix_suffix_descriptor,
                      prefix_descriptor_offset, prefix_to_suffix_);
  if (prefix_suffix_descriptor == suffix_descriptor_count_ - 1) {
    prefix_to_suffix_ = 0;
  }
  extended_descriptors_[ICN2053_EXT_PREFIX].qe.stqe_next = &suffix_descriptors_[0][prefix_to_suffix_];
}

}  // namespace esphome::hub75_icn2053
