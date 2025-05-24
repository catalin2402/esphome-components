#include "icn2053_display.h"
#include <Arduino.h>

#define offset_prefix dma_buff.all_row_data_cnt
#define offset_suffix dma_buff.all_row_data_cnt + dma_buff.frame_prefix_cnt


int data_set(ESP32_I2S_DMA_STORAGE_TYPE *buffer, int offset,
             ESP32_I2S_DMA_STORAGE_TYPE data_mask, int len) {
  while (len > 0) {
    buffer[offset ^ 1] |= data_mask;
    offset++;
    len--;
  }
  return offset;
}

int data_clr(ESP32_I2S_DMA_STORAGE_TYPE *buffer, int offset,
             ESP32_I2S_DMA_STORAGE_TYPE data_mask, int len) {
  while (len > 0) {
    buffer[offset ^ 1] &= ~data_mask;
    offset++;
    len--;
  }
  return offset;
}

int setDataRegBuffer(ESP32_I2S_DMA_STORAGE_TYPE *buffer, int offset,
                     const driver_rgb_t *regs_data) {
  driver_reg_t r, g, b;
  ESP32_I2S_DMA_STORAGE_TYPE d;
  r = regs_data->r;
  g = regs_data->g;
  b = regs_data->b;
  for (int i = DRIVER_BITS; i > 0; i--) {
    d = 0;
    if (r < 0)
      d |= BIT_R1 | BIT_R2;
    r <<= 1;
    if (g < 0)
      d |= BIT_G1 | BIT_G2;
    g <<= 1;
    if (b < 0)
      d |= BIT_B1 | BIT_B2;
    b <<= 1;

    int j = offset ^ 1;
    buffer[j] = (buffer[j] & ~BITMASK_RGB12) | d;
    offset++;
  }
  return offset;
}

int setDataRegBuffer_n(ESP32_I2S_DMA_STORAGE_TYPE *buffer, int offset,
                       driver_rgb_t *regs_data, int regs_cnt) {
  int i;
  for (i = regs_cnt - 1; i >= 0; i--) {
    offset = setDataRegBuffer(buffer, offset, regs_data);
  }
  return offset;
}

ESP32_I2S_DMA_STORAGE_TYPE getAddrBits(uint8_t addr) {
  ESP32_I2S_DMA_STORAGE_TYPE BIT_ADDR[ROW_ADDR_BITS] = {BIT_A, BIT_B, BIT_C,
                                                        BIT_D, BIT_E};
  ESP32_I2S_DMA_STORAGE_TYPE res = 0;
  int i;
  for (i = 0; i < ROW_ADDR_BITS; i++) {
    if (addr & 1)
      res |= BIT_ADDR[i];
    addr >>= 1;
  }
  return res;
}

int setLatRowBuffer(ESP32_I2S_DMA_STORAGE_TYPE *buffer, int offset,
                    int subrow_cnt, int subrow_len) {
  int i = offset + subrow_len - 1;
  while (subrow_cnt > 0) {
    buffer[i ^ 1] |= BIT_LAT;
    i += subrow_len + SUBROW_ADD_LEN;
    subrow_cnt--;
  }
  return offset;
}

int icn2053setOEaddrBuffer(ESP32_I2S_DMA_STORAGE_TYPE *buffer, int offset,
                           uint8_t row_cnt, size_t buffer_len,
                           int frame_offset) {
  ESP32_I2S_DMA_STORAGE_TYPE data;
  uint8_t addr;
  int row_offset;
  int oe_cnt;
  if (frame_offset >= (ICN2053_ROW_OE_LEN << ROW_ADDR_BITS))
    frame_offset = 0;
  addr = frame_offset / ICN2053_ROW_OE_LEN;
  row_offset = frame_offset % ICN2053_ROW_OE_LEN;
  if (row_offset > ICN2053_ROW_OE_CNT * 2)
    oe_cnt = 0;
  else
    oe_cnt = (ICN2053_ROW_OE_CNT - (row_offset >> 1));

  data = getAddrBits(addr);
  while (offset < buffer_len) {
    if (row_offset == ICN2053_ROW_OE_LEN) {
      addr++;
      if (addr >= row_cnt) {
        addr = 0;
        frame_offset = 0;
      }
      data = getAddrBits(addr);
      row_offset = 0;
      oe_cnt = ICN2053_ROW_OE_CNT;
    }
    if (oe_cnt > 0) {
      buffer[offset ^ 1] = data | BIT_OE;
      oe_cnt--;
    } else
      buffer[offset ^ 1] = data;
    row_offset++;
    frame_offset++;
    offset++;
    if (offset == buffer_len)
      break;
    buffer[offset ^ 1] = data;
    row_offset++;
    frame_offset++;
    offset++;
  }
  return frame_offset++;
}

int icn2053setVSyncBuffer(ESP32_I2S_DMA_STORAGE_TYPE *vsync_buffer, int offset,
                          bool leds_enable, bool vsync) {
  offset = data_clr(vsync_buffer, offset, BIT_LAT, ICN2053_CMD_DELAY);
  offset = data_set(vsync_buffer, offset, BIT_LAT, ICN2053_PRE_ACT);
  offset = data_clr(vsync_buffer, offset, BIT_LAT, ICN2053_CMD_DELAY);
  int n = ICN2053_CMD_DELAY;
  if (leds_enable) {
    offset = data_set(vsync_buffer, offset, BIT_LAT, ICN2053_EN_OP);
    n += ICN2053_DIS_OP - ICN2053_EN_OP;
  } else {
    offset = data_set(vsync_buffer, offset, BIT_LAT, ICN2053_DIS_OP);
  }
  offset = data_clr(vsync_buffer, offset, BIT_LAT, n);
  if (leds_enable)
    offset = data_set(vsync_buffer, offset, BIT_LAT, ICN2053_V_SYNC);
  else
    offset = data_clr(vsync_buffer, offset, BIT_LAT, ICN2053_V_SYNC);
  offset = data_clr(vsync_buffer, offset, BIT_LAT, ICN2053_CMD_DELAY);
  offset = data_set(vsync_buffer, offset, BIT_LAT, ICN2053_PRE_ACT);
  offset = data_clr(vsync_buffer, offset, BIT_LAT, ICN2053_CMD_DELAY);
  return offset;
}

void ICN2053_Display::icn2053setVSync(bool leds_enable, bool vsync) {
  icn2053setVSyncBuffer(dma_buff.rowBits[offset_prefix], FRAME_ADD_LEN,
                        leds_enable, vsync);
}

void ICN2053_Display::icn2053setReg(uint8_t reg_idx, driver_rgb_t *regs_data) {
  setDataRegBuffer_n(dma_buff.rowBits[offset_prefix],
                     FRAME_ADD_LEN + ICN2053_PREFIX_START_LEN, regs_data,
                     driver_cnt);
  data_clr(dma_buff.rowBits[offset_prefix],
           dma_buff.frame_prefix_len - DRIVER_BITS, BIT_LAT, DRIVER_BITS);
  data_set(dma_buff.rowBits[offset_prefix],
           dma_buff.frame_prefix_len - ICN2053_REG_CMD[reg_idx], BIT_LAT,
           ICN2053_REG_CMD[reg_idx]);
}

void ICN2053_Display::icn2053initBuffers() {
  int frame_offset_data = 0;
  int row_offset = 0;
  for (int buf_id = 0; buf_id >= 0; buf_id--) {
    frame_offset_data = 0;
    for (int row = 0; row < dma_buff.row_data_cnt; row++) {

      frame_offset_data = icn2053setOEaddrBuffer(
          dma_buff.rowBits[row_offset + row], 0, rows_per_frame,
          dma_buff.row_data_len, frame_offset_data);
      setLatRowBuffer(dma_buff.rowBits[row_offset + row], 0, DRIVER_BITS,
                      pixels_per_row);
    }
    dmadesc_data[buf_id][desc_data_cnt - 1].eof = true;
    dmadesc_data[buf_id][desc_data_cnt - 1].qe.stqe_next =
        &dmadesc_ext[ICN2053_EXT_DATA];
    row_offset += dma_buff.row_data_cnt;
  }

  frame_offset_data %= dma_buff.frame_suffix_len;

  frame_offset_data *= SIZE_DMA_TYPE;
  int desk_idx_datain = frame_offset_data / DMA_MAX;
  int offset_bufer_datain = frame_offset_data % DMA_MAX;
  int frame_offset_prefix = icn2053setOEaddrBuffer(
      dma_buff.rowBits[offset_prefix], FRAME_ADD_LEN + ICN2053_VSYNC_LEN,
      rows_per_frame, dma_buff.frame_prefix_len, 0);

  frame_offset_prefix %= dma_buff.frame_suffix_len;
  frame_offset_prefix *= SIZE_DMA_TYPE;
  int desk_idx_prefixin = frame_offset_prefix / DMA_MAX;
  int offset_bufer_prefixin = frame_offset_prefix % DMA_MAX;
  ;
  dmadesc_prefix[desc_prefix_cnt - 1].eof = true;
  dmadesc_prefix[desc_prefix_cnt - 1].qe.stqe_next =
      &dmadesc_ext[ICN2053_EXT_PREFIX];
  icn2053setOEaddrBuffer(dma_buff.rowBits[offset_suffix], 0, rows_per_frame,
                         dma_buff.frame_suffix_len, 0);
  for (int desc_suffix = ICN2053_DSUFFIX_CNT - 1; desc_suffix >= 0;
       desc_suffix--) {
    dmadesc_suffix[desc_suffix][desc_suffix_cnt - 1].eof = true;
    dmadesc_suffix[desc_suffix][desc_suffix_cnt - 1].qe.stqe_next =
        &dmadesc_suffix[desc_suffix][0];
  }

  dmadesc_ext[ICN2053_EXT_DATA].buf =
      &dmadesc_suffix[0][desk_idx_datain].buf[offset_bufer_datain];
  dmadesc_ext[ICN2053_EXT_DATA].size =
      dmadesc_suffix[0][desk_idx_datain].size - offset_bufer_datain;
  dmadesc_ext[ICN2053_EXT_DATA].length = dmadesc_ext[ICN2053_EXT_DATA].size;
  if (desk_idx_datain == (desc_suffix_cnt - 1)) {
    data_to_suffix = 0;
  } else
    data_to_suffix = desk_idx_datain + 1;
  dmadesc_ext[ICN2053_EXT_DATA].qe.stqe_next =
      &dmadesc_suffix[0][data_to_suffix];
  dmadesc_ext[ICN2053_EXT_PREFIX].buf =
      &dmadesc_suffix[0][desk_idx_prefixin].buf[offset_bufer_prefixin];
  dmadesc_ext[ICN2053_EXT_PREFIX].size =
      dmadesc_suffix[0][desk_idx_prefixin].size - offset_bufer_prefixin;
  dmadesc_ext[ICN2053_EXT_PREFIX].length = dmadesc_ext[ICN2053_EXT_PREFIX].size;
  if (desk_idx_prefixin == (desc_suffix_cnt - 1)) {
    prefix_to_suffix = 0;
  } else
    prefix_to_suffix = desk_idx_prefixin + 1;
  dmadesc_ext[ICN2053_EXT_PREFIX].qe.stqe_next =
      &dmadesc_suffix[0][prefix_to_suffix];
}

void setDriverReg(driver_reg_t &driver_reg, driver_reg_t value,
                  driver_reg_t mask, uint8_t offset) {
  driver_reg = (driver_reg & (~mask)) | ((value << offset) & mask);
}

void setDriverRegRGB(driver_rgb_t *driver_reg, driver_reg_t value,
                     driver_reg_t mask, uint8_t offset) {
  setDriverReg(driver_reg->r, value, mask, offset);
  setDriverReg(driver_reg->g, value, mask, offset);
  setDriverReg(driver_reg->b, value, mask, offset);
}

ICN2053_Display *MatrixPanel = NULL;

void ICN2053_Display::buffersFree() {
  int i;
  if (dma_buff.rowBits != NULL) {
    i = dma_buff.all_row_data_cnt + dma_buff.frame_prefix_cnt +
        dma_buff.frame_suffix_cnt;
    while (i > 0) {
      i--;
      if (dma_buff.rowBits[i] != NULL) {
        heap_caps_free(dma_buff.rowBits[i]);
      }
    }
    heap_caps_free(dma_buff.rowBits);
  }
  for (i = -1; i >= 0; i--) {
    if (frame_buffer.frameBits[i] != NULL) {
      heap_caps_free(frame_buffer.frameBits[i]);
    }
  }
  if (dmadesc_data[0] != NULL) {
    heap_caps_free(dmadesc_data[0]);
  }
  if (brightness_table != NULL) {
    heap_caps_free(brightness_table);
  }
}

inline void ICN2053_Display::getRGBColor16(int offset_y, int offset_x,
                                           rgb888_t &rgb888) {
  uint16_t *addr =
      (uint16_t *)&frame_buffer.frameBits[back_buffer_id ^ 0][offset_y];
  rgb888.r = ((((addr[offset_x] >> 11)) * 527) + 23) >> 6;
  rgb888.g = ((((addr[offset_x] >> 5) & 0x3F) * 259) + 33) >> 6;
  rgb888.b = (((addr[offset_x] & 0x1F) * 527) + 23) >> 6;
}

IRAM_ATTR void ICN2053_Display::prepareDmaRows(uint8_t row_offset,
                                               uint8_t dma_buff_id) {
  rgb888_t pixel888_1;
  rgb888_t pixel888_2;
  driver_rgb_t pixel1;
  driver_rgb_t pixel2;

  if (icn2053_clear)
    return;

  row_offset += scroll_y;
  for (int row_buf_id = 0; row_buf_id < dma_buff.row_data_cnt; row_buf_id++) {
    if (row_offset >= rows_per_frame)
      row_offset -= rows_per_frame;
    int offset_y = row_offset * frame_buffer.row_len;
    int dma_buff_offset = 0;
    int dma_buff_offset_next = 0;
    int driver_idx = DRIVER_BITS;
    int offset_x = pixels_per_row + scroll_x - 1;
    if (offset_x >= pixels_per_row)
      offset_x -= pixels_per_row;
    int row_dma_offset =
        (dma_buff_id) ? (row_buf_id + dma_buff.row_data_cnt) : row_buf_id;

    int8_t color_bits = dma_buff.color_bits;
    for (int i = pixels_per_row; i > 0; i--) {
      getRGBColor16(offset_y, offset_x, pixel888_1);
      getRGBColor16(offset_y + (frame_buffer.subframe_len >> 1), offset_x,
                    pixel888_2);
      pixel1.r = brightness_table[pixel888_1.r];
      pixel1.g = brightness_table[pixel888_1.g];
      pixel1.b = brightness_table[pixel888_1.b];
      pixel2.r = brightness_table[pixel888_2.r];
      pixel2.g = brightness_table[pixel888_2.g];
      pixel2.b = brightness_table[pixel888_2.b];

      ESP32_I2S_DMA_STORAGE_TYPE mask_rgb12;
      for (int j = DRIVER_BITS - 1; j >= 0; j--) {
        mask_rgb12 = 0;
        if (pixel1.r < 0)
          mask_rgb12 |= BIT_R2;
        pixel1.r <<= 1;
        if (pixel1.g < 0)
          mask_rgb12 |= BIT_G2;
        pixel1.g <<= 1;
        if (pixel1.b < 0)
          mask_rgb12 |= BIT_B2;
        pixel1.b <<= 1;
        if (pixel2.r < 0)
          mask_rgb12 |= BIT_R1;
        pixel2.r <<= 1;
        if (pixel2.g < 0)
          mask_rgb12 |= BIT_G1;
        pixel2.g <<= 1;
        if (pixel2.b < 0)
          mask_rgb12 |= BIT_B1;
        pixel2.b <<= 1;

        int offset_dma = dma_buff_offset ^ 1;
        dma_buff.rowBits[row_dma_offset][offset_dma] =
            (dma_buff.rowBits[row_dma_offset][offset_dma] & (~BITMASK_RGB12)) |
            mask_rgb12;
        dma_buff_offset++;
      }
      driver_idx--;
      if (driver_idx == 0) {
        driver_idx = DRIVER_BITS;
        dma_buff_offset_next += DRIVER_BITS;
        dma_buff_offset = dma_buff_offset_next;
      } else {
        dma_buff_offset += pixels_per_row + SUBROW_ADD_LEN - DRIVER_BITS;
      }
      if (offset_x == 0)
        offset_x = pixels_per_row - 1;
      else
        offset_x--;
    }
    row_offset--;
  }
}

void ICN2053_Display::panelShowOff() {
  waitDmaReady();
  icn2053setVSync(false, false);
  sendVsync();
  waitDmaReady();
  icn2053setVSync(false, true);
}

void ICN2053_Display::panelShowOn() {
  waitDmaReady();
  icn2053setVSync(true, false);
  sendVsync();
  waitDmaReady();
  icn2053setVSync(true, true);
}

IRAM_ATTR void ICN2053_Display::sendCBVsync() {
  icn2053setReg(driver_cur_reg, (&driver_reg[driver_cur_reg]));
  driver_cur_reg++;
  if (driver_cur_reg >= ICN2053_REG_CNT)
    driver_cur_reg = 0;
  dmadesc_ext[ICN2053_EXT_PREFIX].qe.stqe_next =
      &dmadesc_suffix[cur_suffix_id][prefix_to_suffix];
  dmadesc_suffix[cur_suffix_id][desc_suffix_cnt - 1].qe.stqe_next =
      dmadesc_suffix[cur_suffix_id];
  dmadesc_suffix[cur_suffix_id][desc_suffix_cnt - 1].eof = true;
  cur_suffix_id ^= 1;
  dmadesc_suffix[cur_suffix_id][desc_suffix_cnt - 1].qe.stqe_next =
      dmadesc_prefix;
  dma_int_cnt = -1;
}

IRAM_ATTR void ICN2053_Display::sendCBRow(uint8_t buff_id) {
  dmadesc_ext[ICN2053_EXT_DATA].qe.stqe_next =
      &dmadesc_suffix[cur_suffix_id][data_to_suffix];
  dmadesc_suffix[cur_suffix_id][desc_suffix_cnt - 1].qe.stqe_next =
      dmadesc_suffix[cur_suffix_id];
  dmadesc_suffix[cur_suffix_id][desc_suffix_cnt - 1].eof = true;
  cur_suffix_id ^= 1;
  dmadesc_suffix[cur_suffix_id][desc_suffix_cnt - 1].qe.stqe_next =
      dmadesc_data[buff_id];
  dma_int_cnt = -1;
}

IRAM_ATTR void ICN2053_Display::sendCallback() {
  static uint8_t dma_buffer_id = 0;
  if (bufferBusy || bufferReady || dma_int_cnt > 0)
    return;
  bufferBusy = true;
  dma_int_cnt++;

  if (dma_int_cnt > 0) {
    dmadesc_suffix[cur_suffix_id ^ 1][desc_suffix_cnt - 1].eof = false;
    if (rows_send_cnt > 0) {
      int row_send_idx = rows_send_cnt - 1;

      prepareDmaRows(row_send_idx, 0);
      sendCBRow(0);

      rows_send_cnt -= dma_buff.row_data_cnt;
      if (rows_send_cnt <= 0) {
        icn2053_clear = false;
        if (icn2053_auto_vsync)
          rows_send_cnt = 0;
        else
          rows_send_cnt = -1;
        dma_int_cnt = -4;
      }
    } else if (rows_send_cnt == 0) {
      sendCBVsync();
      rows_send_cnt = -1;
    } else {
      bufferReady = true;
    }
  }
  bufferBusy = false;
}

IRAM_ATTR void icn2053i2sCallback() { MatrixPanel->sendCallback(); }

void ICN2053_Display::waitDmaReady() {
  if (frame_buffer.len == 0)
    return;
  while (!bufferReady) {
    delay(1);
  }
}

void ICN2053_Display::sendVsync() {
  waitDmaReady();
  bufferReady = false;
  bufferBusy = true;
  sendCBVsync();
  bufferBusy = false;
}

void ICN2053_Display::sendFrame(bool waitSend, bool autoVsync) {
  waitDmaReady();
  bufferReady = false;
  dma_int_cnt = 0;
  icn2053_auto_vsync = autoVsync;
  dmadesc_suffix[cur_suffix_id ^ 1][desc_suffix_cnt - 1].eof = true;
  rows_send_cnt = rows_per_frame;
  if (waitSend)
    waitDmaReady();
}

void ICN2053_Display::icn2053init() {
  icn2053setVSync(false, true);
  for (int i = 0; i < ICN2053_REG_CNT; i++) {
    sendVsync();
  }
  icn2053_clear = true;
  sendFrame();
  icn2053_clear = true;
  sendFrame();
  icn2053setVSync(true, true);
  sendVsync();
}

bool ICN2053_Display::allocateDMAmemory() {

  size_t matrix_buffer_size = 0;
  size_t subrow_data_len = pixels_per_row + SUBROW_ADD_LEN;
  size_t gamma_table_len = 0;
  uint8_t buff_cnt;
  back_buffer_id = 0;
  gamma_table_len = BRIGHT_TABLE_SIZE;

  dma_buff.frame_prefix_len =
      FRAME_ADD_LEN + ICN2053_PREFIX_START_LEN + pixels_per_row;
  dma_buff.frame_prefix_cnt = ICN2053_PREFIX_CNT;
  dma_buff.frame_suffix_len = ICN2053_ROW_OE_LEN * rows_per_frame;
  dma_buff.frame_suffix_cnt = ICN2053_SUFFIX_CNT;
  dma_buff.row_data_len = subrow_data_len * DRIVER_BITS + ROW_ADD_LEN;
  dma_buff.row_data_cnt = 4;
  if (dma_buff.row_data_cnt > rows_per_frame)
    dma_buff.row_data_cnt = rows_per_frame;
  dma_buff.color_bits = -16;
  frame_buffer.row_len = (pixels_per_row * 2 + VB_SIZE - 1) / VB_SIZE;
  frame_buffer.subframe_len = frame_buffer.row_len * m_cfg.mx_height;
  frame_buffer.len = frame_buffer.subframe_len;
  buff_cnt = ICN2053_PREFIX_CNT + ICN2053_SUFFIX_CNT;
  matrix_buffer_size = (dma_buff.frame_prefix_len * ICN2053_PREFIX_CNT +
                        dma_buff.frame_suffix_cnt * ICN2053_SUFFIX_CNT) *
                       SIZE_DMA_TYPE;
  matrix_buffer_size <<= 0;

  dma_buff.all_row_data_cnt = dma_buff.row_data_cnt << 0;
  buff_cnt += dma_buff.all_row_data_cnt;
  matrix_buffer_size +=
      dma_buff.row_data_len * dma_buff.all_row_data_cnt * SIZE_DMA_TYPE +
      gamma_table_len;

  size_t heap_free_size = heap_caps_get_free_size(MALLOC_CAP_DMA);
  if (heap_free_size < matrix_buffer_size) {

    return false;
  }
  dma_buff.rowBits = (ESP32_I2S_DMA_STORAGE_TYPE **)heap_caps_calloc(
      buff_cnt, sizeof(void *), MALLOC_CAP_32BIT | MALLOC_CAP_DMA);

  size_t data_len = dma_buff.row_data_len;
  for (int malloc_num = 0; malloc_num < buff_cnt; malloc_num++) {
    if (malloc_num >= dma_buff.all_row_data_cnt) {
      if (malloc_num <
          (dma_buff.all_row_data_cnt + dma_buff.frame_prefix_cnt)) {
        data_len = dma_buff.frame_prefix_len;

      } else {
        data_len = dma_buff.frame_suffix_len;
      }
    }
    dma_buff.rowBits[malloc_num] =
        (ESP32_I2S_DMA_STORAGE_TYPE *)heap_caps_calloc(data_len, SIZE_DMA_TYPE,
                                                       MALLOC_CAP_DMA);

    if (dma_buff.rowBits[malloc_num] == NULL) {

      buffersFree();
      return false;
    }
  }
  if (frame_buffer.len > 0) {
    for (int frame_id = 0; frame_id >= 0; frame_id--) {

      frame_buffer.frameBits[frame_id] = (vbuffer_t *)heap_caps_calloc(
          1, frame_buffer.len * VB_SIZE, MALLOC_CAP_8BIT);
      if (frame_buffer.frameBits[frame_id] == NULL) {

        buffersFree();
        return false;
      }
    }
  }
  if (gamma_table_len > 0) {

    brightness_table = (uint16_t *)heap_caps_calloc(
        1, gamma_table_len * sizeof(uint16_t), MALLOC_CAP_8BIT);
    if (brightness_table == NULL) {

      buffersFree();
      return false;
    }
  }

  desc_data_cnt =
      (dma_buff.row_data_len * SIZE_DMA_TYPE + DMA_MAX - 1) / DMA_MAX;
  desc_data_cnt *= dma_buff.row_data_cnt;
  desc_prefix_cnt =
      (dma_buff.frame_prefix_len * SIZE_DMA_TYPE + DMA_MAX - 1) / DMA_MAX;
  desc_prefix_cnt *= dma_buff.frame_prefix_cnt;
  desc_suffix_cnt =
      (dma_buff.frame_suffix_len * SIZE_DMA_TYPE + DMA_MAX - 1) / DMA_MAX;
  desc_suffix_cnt *= dma_buff.frame_suffix_cnt;
  int desc_cnt = desc_suffix_cnt;
  int desc_ext_cnt = 0;
  desc_ext_cnt = ICN2053_DESCEXT_CNT;
  desc_cnt *= ICN2053_DSUFFIX_CNT;

  desc_cnt += (desc_data_cnt << 0) + desc_prefix_cnt + desc_ext_cnt;
  dmadesc_data[0] =
      (lldesc_t *)heap_caps_calloc(desc_cnt, sizeof(lldesc_t), MALLOC_CAP_DMA);
  if (dmadesc_data[0] == NULL) {

    buffersFree();
    return false;
  }
  size_t _dma_capable_memory_reserved = desc_cnt * sizeof(lldesc_t);

  dmadesc_data[1] = dmadesc_data[0];
  if (dma_buff.frame_prefix_cnt > 0)
    dmadesc_prefix = &dmadesc_data[1][desc_data_cnt];
  else
    dmadesc_prefix = &dmadesc_data[0][0];
  dmadesc_suffix[0] = &dmadesc_prefix[desc_prefix_cnt];

  dmadesc_suffix[1] = &dmadesc_suffix[0][desc_suffix_cnt];

  dmadesc_ext = &dmadesc_suffix[1][desc_suffix_cnt];

  return true;
}

lldesc_t *linkDmaDesc(lldesc_t *dmadesc, lldesc_t *previous_dmadesc,
                      int dmadesk_cnt, uint8_t *dma_buffer, int bufer_size,
                      int max_dma_size) {
  size_t dma_size;
  while ((dmadesk_cnt > 0) && (bufer_size > 0)) {
    dma_size = (bufer_size >= max_dma_size) ? max_dma_size : bufer_size;
    link_dma_desc(dmadesc, previous_dmadesc, dma_buffer, dma_size);
    previous_dmadesc = dmadesc;
    dmadesc = &dmadesc[1];

    dma_buffer += dma_size;
    bufer_size -= dma_size;
    dmadesk_cnt--;
  }
  return previous_dmadesc;
}

lldesc_t *linkDmaDesc_n(lldesc_t *dmadesc, lldesc_t *previous_dmadesc,
                        int single_dmadesk_cnt, uint8_t **dma_buffer,
                        int single_bufer_size, int max_dma_size,
                        int buffers_cnt) {
  int i = 0;
  while (i < buffers_cnt) {
    previous_dmadesc =
        linkDmaDesc(dmadesc, previous_dmadesc, single_dmadesk_cnt,
                    dma_buffer[i], single_bufer_size, max_dma_size);
    dmadesc = &previous_dmadesc[1];
    i++;
  }
  return previous_dmadesc;
}

void ICN2053_Display::configureDMA() {

  linkDmaDesc_n(dmadesc_prefix, NULL, desc_prefix_cnt,
                (uint8_t **)&dma_buff.rowBits[dma_buff.all_row_data_cnt],
                dma_buff.frame_prefix_len * SIZE_DMA_TYPE, DMA_MAX,
                dma_buff.frame_prefix_cnt);

  linkDmaDesc_n(dmadesc_suffix[0], NULL, desc_suffix_cnt,
                (uint8_t **)&dma_buff.rowBits[dma_buff.all_row_data_cnt +
                                              dma_buff.frame_prefix_cnt],
                dma_buff.frame_suffix_len * SIZE_DMA_TYPE, DMA_MAX,
                dma_buff.frame_suffix_cnt);

  linkDmaDesc_n(dmadesc_data[0], NULL, desc_data_cnt,
                (uint8_t **)dma_buff.rowBits,
                dma_buff.row_data_len * SIZE_DMA_TYPE, DMA_MAX,
                dma_buff.all_row_data_cnt);

  linkDmaDesc_n(dmadesc_suffix[1], NULL, desc_suffix_cnt,
                (uint8_t **)&dma_buff.rowBits[dma_buff.all_row_data_cnt +
                                              dma_buff.frame_prefix_cnt],
                dma_buff.frame_suffix_len * SIZE_DMA_TYPE, DMA_MAX, 1);
  for (int i = ICN2053_DESCEXT_CNT - 1; i >= 0; i--) {
    link_dma_desc(&dmadesc_ext[i], NULL, NULL, 0);
  }
  driver_reg = (driver_rgb_t *)heap_caps_calloc(
      ICN2053_REG_CNT, sizeof(driver_rgb_t), MALLOC_CAP_8BIT);
  if (driver_reg == NULL) {
    buffersFree();
    return;
  }
  memcpy(driver_reg, ICN2053_REG_VALUE, ICN2053_REG_CNT * sizeof(driver_rgb_t));
  setDriverRegRGB(&driver_reg[ICN2053_CFG1], (rows_per_frame - 1),
                  ICN2053_CFG1_LC_MASK, ICN2053_CFG1_LC_OFFSET);
  icn2053initBuffers();
  bufferReady = false;
  bufferBusy = false;
  icn2053_clear = false;
  icn2053_auto_vsync = false;
  driver_cur_reg = 0;
  cur_suffix_id = 0;
  rows_send_cnt = 0;
  dma_int_cnt = 0;

  i2s_parallel_config_t cfg = {
      .gpio_bus = {m_cfg.gpio.r1, m_cfg.gpio.g1, m_cfg.gpio.b1, m_cfg.gpio.r2,
                   m_cfg.gpio.g2, m_cfg.gpio.b2, m_cfg.gpio.lat, m_cfg.gpio.oe,
                   m_cfg.gpio.a, m_cfg.gpio.b, m_cfg.gpio.c, m_cfg.gpio.d,
                   m_cfg.gpio.e, -1, -1, -1},
      .gpio_clk = m_cfg.gpio.clk,
      .sample_rate = m_cfg.i2sspeed,
      .sample_width = ESP32_I2S_DMA_MODE,
      .desccount_a = desc_data_cnt,
      .lldesc_a = dmadesc_data[0],
      .desccount_b = desc_data_cnt,
      .lldesc_b = dmadesc_data[1],
      .clkphase = 0};

  MatrixPanel = this;
  esp_err_t res = i2s_parallel_driver_install(I2S_NUM_1, &cfg);
  if (res != ESP_OK) {

    buffersFree();
    return;
  }
  scroll_y = 0;
  scroll_x = 0;
  CurColor = 0;

  initialized = true;

  setShiftCompleteCallback(&icn2053i2sCallback);
  i2s_parallel_send_dma(I2S_NUM_1, dmadesc_suffix[0]);
  icn2053init();

  initialized = true;
  setPanelBrightness(brightness);
}

void ICN2053_Display::fillRectFrameBuffer(int16_t x1, int16_t y1, int16_t x2,
                                          int16_t y2) {
  uniptr_t addr;
  int offset_y = y1 * frame_buffer.row_len;
  int x;
  y2 -= y1;
  x2 -= x1;
  int16_t _x2 = x2;
  while (y2 >= 0) {
    addr.pXX = &frame_buffer.frameBits[back_buffer_id][offset_y];
    x = x1;
    while (x2 >= 0) {
      addr.p16[x] = (uint16_t)CurColor;
      x++;
      x2--;
    }
    y1++;
    offset_y += frame_buffer.row_len;
    x2 = _x2;
    y2--;
  }
}

void ICN2053_Display::fillRectBuffer(int16_t x, int16_t y, int16_t w,
                                     int16_t h) {
  if (!initialized) {

    return;
  }
  if (x >= pixels_per_row || y >= m_cfg.mx_height) {
    return;
  }
  int16_t x2 = x + w - 1;
  int16_t y2 = y + h - 1;
  if (x2 < 0 || y2 < 0) {
    return;
  }
  if (x < 0)
    x = 0;
  if (x2 >= pixels_per_row)
    x2 = pixels_per_row - 1;
  if (y2 >= m_cfg.mx_height)
    y2 = m_cfg.mx_height - 1;

  if (frame_buffer.len != 0) {
    waitDmaReady();
    fillRectFrameBuffer(x, y, x2, y2);
  }
}

void ICN2053_Display::clearDmaBuffer(uint8_t _buff_id) {
  int y_max, y_min;
  _buff_id &= 0;
  if (_buff_id) {
    y_max = dma_buff.all_row_data_cnt - 1;
    y_min = dma_buff.all_row_data_cnt >> 1;
  } else {
    y_max = (dma_buff.all_row_data_cnt >> 1) - 1;
    y_min = 0;
  }
  for (int y = y_max; y >= y_min; y--) {
    for (int x = dma_buff.row_data_len - 1; x >= 0; x--) {
      dma_buff.rowBits[y][x] &= ~BITMASK_RGB12;
    }
  }
}

void ICN2053_Display::clearBuffer(uint8_t _buff_id) {
  _buff_id &= 1;
  uint8_t clear_buff_idx = 0 & (back_buffer_id ^ _buff_id ^ 1);
  if (frame_buffer.len != 0) {
    if (initialized & (_buff_id == 0)) {
      panelShowOff();
      waitDmaReady();
      icn2053_clear = true;
      sendFrame();
    }
    waitDmaReady();
    memset((uint8_t *)frame_buffer.frameBits[clear_buff_idx], 0,
           frame_buffer.len * VB_SIZE);
    if (initialized & (_buff_id == 0)) {
      waitDmaReady();
      icn2053_clear = true;
      sendFrame();
      panelShowOn();
    }
  }
}

ICN2053_Display::ICN2053_Display(const hub75_i2s_cfg_t &opts) : m_cfg(opts) {}

bool ICN2053_Display::begin() {

  pixels_per_row = m_cfg.mx_width * m_cfg.chain_length_x * m_cfg.chain_length_y;
  rows_per_frame = m_cfg.mx_height / 2;
  driver_cnt = pixels_per_row / DRIVER_BITS;
  brightness = 255;

  if (!allocateDMAmemory()) {
    return false;
  }
  configureDMA();

  return initialized;
}

void ICN2053_Display::stopDMAoutput() {
  clearScreen();
  i2s_parallel_stop_dma(I2S_NUM_1);
}


IRAM_ATTR void ICN2053_Display::flipDMABuffer() {
  if (frame_buffer.len != 0)
    sendFrame();
}

void ICN2053_Display::setPanelBrightness(int b) {
  if (!initialized)
    return;
  brightness = b;
  waitDmaReady();

  if (brightness_table != NULL) {
    uint32_t g;
    for (int i = BRIGHT_TABLE_SIZE - 1; i >= 0; i--) {
      brightness_table[i] =
          (lumConvTab[b] | (lumConvTab[b] << 8)) * i / (BRIGHT_TABLE_SIZE - 1);
    }
  }
}

void ICN2053_Display::setColor(uint8_t r, uint8_t g, uint8_t b) {
  CurColor = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void ICN2053_Display::clearScreen() { clearBuffer(0); }

void ICN2053_Display::drawPixelRGB888(int16_t x, int16_t y, uint8_t r,
                                      uint8_t g, uint8_t b) {
  setColor(g, b, r);
  fillRectBuffer(x, y, 1, 1);
}
