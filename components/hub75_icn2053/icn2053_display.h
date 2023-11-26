#ifndef _ESP32_RGB_64_32_MATRIX_PANEL_I2S_DMA
#define _ESP32_RGB_64_32_MATRIX_PANEL_I2S_DMA

#include "esp32_i2s_parallel_v2.h"

#include <stdbool.h>
#include <stdint.h>

#define ESP32_I2S_DMA_MODE I2S_PARALLEL_WIDTH_16

typedef int16_t driver_reg_t;

enum { DRIVER_BITS = 16 };

typedef struct {
  driver_reg_t r;
  driver_reg_t g;
  driver_reg_t b;
} driver_rgb_t;

enum { ICN2053_CMD_DELAY = 16 };
enum { SUBROW_ADD_LEN = 8 };
enum { ROW_ADD_LEN = 8 };
enum { FRAME_ADD_LEN = 16 };
enum { ROW_ADDR_BITS = 5 };

enum {
  ICN2053_DATA_LATCH = 1,
  ICN2053_WR_DBG = 2,
  ICN2053_V_SYNC = 3,
  ICN2053_WR_CFG1 = 4,
  ICN2053_RD_CFG1 = 5,
  ICN2053_WR_CFG2 = 6,
  ICN2053_RD_CFG2 = 7,
  ICN2053_WR_CFG3 = 8,
  ICN2053_RD_CFG3 = 9,
  ICN2053_WR_CFG4 = 10,
  ICN2053_RD_CFG4 = 11,
  ICN2053_EN_OP = 12,
  ICN2053_DIS_OP = 13,
  ICN2053_PRE_ACT = 14,
};

enum {
  ICN2053_DBG_x = 0x08,
};

enum {
  ICN2053_CFG1_WOBPS_MASK = 0x4000,
  ICN2053_CFG1_WOBPS_OFFSET = 14,
  ICN2053_CFG1_WOBPS = 0,
  ICN2053_CFG1_LC_MASK = 0x1F00,
  ICN2053_CFG1_LC_OFFSET = 8,
  ICN2053_CFG1_LC = 19,
  ICN2053_CFG1_LGS_MASK = 0x00C0,
  ICN2053_CFG1_LGS_OFFSET = 6,
  ICN2053_CFG1_LGS = 1,
  ICN2053_CFG1_RRM_MASK = 0x0030,
  ICN2053_CFG1_RRM_OFFSET = 4,
  ICN2053_CFG1_RRM = 3,
  ICN2053_CFG1_PWMR_MASK = 0x0008,
  ICN2053_CFG1_PWMR_OFFSET = 3,
  ICN2053_CFG1_PWMR = 0,
  ICN2053_CFG1_R = (ICN2053_CFG1_WOBPS << 14) + (ICN2053_CFG1_LC << 8) +
                   (ICN2053_CFG1_LGS << 6) + (ICN2053_CFG1_RRM << 4) +
                   (ICN2053_CFG1_PWMR << 3),
  ICN2053_CFG1_G = ICN2053_CFG1_R,
  ICN2053_CFG1_B = ICN2053_CFG1_R,
};

enum {
  ICN2053_CFG2_BL_MASK = 0x7C00,
  ICN2053_CFG2_BL_OFFSET = 10,
  ICN2053_CFG2_BL_R = 31,
  ICN2053_CFG2_BL_G = 28,
  ICN2053_CFG2_BL_B = 23,
  ICN2053_CFG2_CURRENT_MASK = 0x003E,
  ICN2053_CFG2_CURRENT_OFFSET = 1,
  ICN2053_CFG2_CURRENT_R = 13,
  ICN2053_CFG2_CURRENT_G = 13,
  ICN2053_CFG2_CURRENT_B = 13,

  ICN2053_CFG2_BE_MASK = 0x0001,
  ICN2053_CFG2_BE_OFFSET = 0,
  ICN2053_CFG2_BE_R = 1,
  ICN2053_CFG2_BE_G = 1,
  ICN2053_CFG2_BE_B = 1,
  ICN2053_CFG2_BRIGHT_MASK = 0x03E0,
  ICN2053_CFG2_BRIGHT_OFFSET = 5,
  ICN2053_CFG2_BRIGHT = 14,
  ICN2053_CFG2_R = (ICN2053_CFG2_BL_R << 10) + (ICN2053_CFG2_CURRENT_R << 1) +
                   (ICN2053_CFG2_BE_R << 0) + (ICN2053_CFG2_BRIGHT << 5),
  ICN2053_CFG2_G = (ICN2053_CFG2_BL_G << 10) + (ICN2053_CFG2_CURRENT_G << 1) +
                   (ICN2053_CFG2_BE_G << 0) + (ICN2053_CFG2_BRIGHT << 5),
  ICN2053_CFG2_B = (ICN2053_CFG2_BL_B << 10) + (ICN2053_CFG2_CURRENT_B << 1) +
                   (ICN2053_CFG2_BE_B << 0) + (ICN2053_CFG2_BRIGHT << 5),
};

enum {
  ICN2053_CFG3_LGWB_MASK = 0x00f0,
  ICN2053_CFG3_LGWB_OFFSET = 4,
  ICN2053_CFG3_LGWB_R = 4,
  ICN2053_CFG3_LGWB_G = 4,
  ICN2053_CFG3_LGWB_B = 4,
  ICN2053_CFG3_BLE_MASK = 0x0004,
  ICN2053_CFG3_BLE_OFFSET = 2,
  ICN2053_CFG3_BLE_R = 1,
  ICN2053_CFG3_BLE_G = 1,
  ICN2053_CFG3_BLE_B = 1,
  ICN2053_CFG3_BASE = 0x04003,
  ICN2053_CFG3_R = (ICN2053_CFG3_LGWB_R << 4) + (ICN2053_CFG3_BLE_R << 2) +
                   ICN2053_CFG3_BASE,
  ICN2053_CFG3_G = (ICN2053_CFG3_LGWB_G << 4) + (ICN2053_CFG3_BLE_G << 2) +
                   ICN2053_CFG3_BASE,
  ICN2053_CFG3_B = (ICN2053_CFG3_LGWB_B << 4) + (ICN2053_CFG3_BLE_B << 2) +
                   ICN2053_CFG3_BASE,
};

enum {
  ICN2053_CFG4_LGWBE_MASK = 0x4000,
  ICN2053_CFG4_LGWBE_OFFSET = 14,
  ICN2053_CFG4_LGWBE_R = 0,
  ICN2053_CFG4_LGWBE_G = 0,
  ICN2053_CFG4_LGWBE_B = 0,
  ICN2053_CFG4_FLO_MASK = 0x0070,
  ICN2053_CFG4_FLO_OFFSET = 4,
  ICN2053_CFG4_FLO_R = 4,
  ICN2053_CFG4_FLO_G = 4,
  ICN2053_CFG4_FLO_B = 4,
  ICN2053_CFG4_BASE = 0x0e00,
  ICN2053_CFG4_R = (ICN2053_CFG4_LGWBE_R << 14) + (ICN2053_CFG4_FLO_R << 4) +
                   ICN2053_CFG4_BASE,
  ICN2053_CFG4_G = (ICN2053_CFG4_LGWBE_G << 14) + (ICN2053_CFG4_FLO_G << 4) +
                   ICN2053_CFG4_BASE,
  ICN2053_CFG4_B = (ICN2053_CFG4_LGWBE_B << 14) + (ICN2053_CFG4_FLO_B << 4) +
                   ICN2053_CFG4_BASE,
};

enum {
  ICN2053_CFG1 = 0,
  ICN2053_CFG2,
  ICN2053_CFG3,
  ICN2053_CFG4,
  ICN2053_DBG,
  ICN2053_REG_CNT
};

enum { CMD_DELAY = 10 };

const uint8_t ICN2053_REG_CMD[ICN2053_REG_CNT] = {
    ICN2053_WR_CFG1, ICN2053_WR_CFG2, ICN2053_WR_CFG3,
    ICN2053_WR_CFG4, ICN2053_WR_DBG,
};

const driver_rgb_t ICN2053_REG_VALUE[ICN2053_REG_CNT] = {
    {ICN2053_CFG1_R, ICN2053_CFG1_G, ICN2053_CFG1_B},
    {ICN2053_CFG2_R, ICN2053_CFG2_G, ICN2053_CFG2_B},
    {ICN2053_CFG3_R, ICN2053_CFG3_G, ICN2053_CFG3_B},
    {ICN2053_CFG4_R, ICN2053_CFG4_G, ICN2053_CFG4_B},
    {ICN2053_DBG_x, ICN2053_DBG_x, ICN2053_DBG_x},
};

enum {
  ICN2053_VSYNC_LEN = ((ICN2053_CMD_DELAY + ICN2053_PRE_ACT +
                        ICN2053_CMD_DELAY + ICN2053_EN_OP + ICN2053_CMD_DELAY +
                        ICN2053_V_SYNC + ICN2053_CMD_DELAY + 1) /
                       2) *
                      2,
  ICN2053_PREFIX_START_LEN =
      ((ICN2053_VSYNC_LEN + ICN2053_PRE_ACT + ICN2053_CMD_DELAY + 1) / 2) * 2,
};

enum {
  ICN2053_EXT_PREFIX = 0,
  ICN2053_EXT_DATA,
  ICN2053_DESCEXT_CNT,
};

enum { ICN2053_PREFIX_CNT = 1 };
enum { ICN2053_SUFFIX_CNT = 1 };
enum { ICN2053_DSUFFIX_CNT = 2 };

enum { ICN2053_ROW_OE_CNT = 138 };
enum { ICN2053_ROW_OE_ADD_LEN = 2 };
enum { ICN2053_ROW_OE_LEN = ICN2053_ROW_OE_CNT * 2 + ICN2053_ROW_OE_ADD_LEN };

void setDriverReg(driver_reg_t &driver_reg, driver_reg_t value,
                  driver_reg_t mask, uint8_t offset);
void setDriverRegRGB(driver_rgb_t *driver_reg, driver_reg_t value,
                     driver_reg_t mask, uint8_t offset);

typedef uint32_t vbuffer_t;
typedef uint16_t ESP32_I2S_DMA_STORAGE_TYPE;

const uint8_t lumConvTab[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,
    3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,
    6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   8,   9,   9,   9,
    10,  10,  10,  11,  11,  11,  12,  12,  12,  13,  13,  13,  14,  14,  14,
    15,  15,  16,  16,  17,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,
    22,  22,  23,  23,  24,  24,  25,  25,  26,  27,  27,  28,  28,  29,  30,
    30,  31,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  38,  39,  40,
    41,  41,  42,  43,  44,  45,  45,  46,  47,  48,  49,  50,  51,  51,  52,
    53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,
    68,  69,  70,  71,  73,  74,  75,  76,  77,  78,  80,  81,  82,  83,  84,
    86,  87,  88,  90,  91,  92,  93,  95,  96,  98,  99,  100, 102, 103, 105,
    106, 107, 109, 110, 112, 113, 115, 116, 118, 120, 121, 123, 124, 126, 128,
    129, 131, 133, 134, 136, 138, 139, 141, 143, 145, 146, 148, 150, 152, 154,
    156, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183,
    185, 187, 189, 192, 194, 196, 198, 200, 203, 205, 207, 209, 212, 214, 216,
    218, 221, 223, 226, 228, 230, 233, 235, 238, 240, 243, 245, 248, 250, 253,
    255,
};

enum { BRIGHT_TABLE_SIZE = sizeof(lumConvTab) };

enum {
  VB_SIZE = sizeof(vbuffer_t),
  VB_MBITS = 5,
  VB_MASK = 31,
};

enum clk_clock_t {
  HZ_1M = 1000000,
  HZ_5M = 5000000,
  HZ_10M = 10000000,
  HZ_13M = 13000000,
  HZ_20M = 20000000
};
enum { SIZE_DMA_TYPE = sizeof(ESP32_I2S_DMA_STORAGE_TYPE) };
enum {
  BITS_RGB1_OFFSET = 0,
  BIT_R1 = (1 << 0),
  BIT_G1 = (1 << 1),
  BIT_B1 = (1 << 2),

  BITS_RGB2_OFFSET = 3,
  BIT_R2 = (1 << 3),
  BIT_G2 = (1 << 4),
  BIT_B2 = (1 << 5),

  BIT_LAT = (1 << 6),
  BIT_OE = (1 << 7),

  BITS_ADDR_OFFSET = 8,
  BIT_A = (1 << 8),
  BIT_B = (1 << 9),
  BIT_C = (1 << 10),
  BIT_D = (1 << 11),
  BIT_E = (1 << 12),

  BITMASK_ADDR = BIT_A + BIT_B + BIT_C + BIT_D + BIT_E,
  BITMASK_RGB1 = BIT_R1 + BIT_G1 + BIT_B1,
  BITMASK_RGB2 = BIT_R2 + BIT_G2 + BIT_B2,
  BITMASK_RGB12 = BITMASK_RGB1 + BITMASK_RGB2,
};

typedef struct {
  ESP32_I2S_DMA_STORAGE_TYPE **rowBits;
  size_t row_data_len;
  size_t frame_prefix_len;
  size_t frame_suffix_len;
  uint8_t all_row_data_cnt;
  uint8_t row_data_cnt;
  uint8_t frame_prefix_cnt;
  uint8_t frame_suffix_cnt;
  int8_t color_bits;
} frameStruct_t;

typedef union {
  uint8_t *p8;
  uint16_t *p16;
  uint32_t *p32;
  vbuffer_t *pXX;
} uniptr_t;

typedef struct {
  vbuffer_t *frameBits[2];
  size_t len;
  size_t row_len;
  size_t subframe_len;
  uint8_t subframe_cnt;
} frame_buffer_t;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} rgb888_t;

typedef struct {
  int8_t r1, g1, b1, r2, g2, b2, a, b, c, d, e, lat, oe, clk;
} hub75_pins_t;

typedef struct HUB75_I2S_CFG_S {
  uint16_t mx_width;
  uint16_t mx_height;
  uint16_t chain_length_x;
  uint16_t chain_length_y;
  hub75_pins_t gpio;
  clk_clock_t i2sspeed;
  uint8_t latch_blanking;
} hub75_i2s_cfg_t;

class ICN2053_Display {
public:
  ICN2053_Display(const hub75_i2s_cfg_t &opts);
  uint8_t brightness;
  bool begin();
  void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
  void clearScreen();
  void setColor(uint8_t r, uint8_t g, uint8_t b);
  void flipDMABuffer();
  void setPanelBrightness(int b);
  void stopDMAoutput();
  void sendFrame(bool waitSend = false, bool autoVsync = true);
  void sendVsync();
  void panelShowOn();
  void panelShowOff();
  void waitDmaReady();
  void sendCallback();

protected:
  hub75_i2s_cfg_t m_cfg;
  bool icn2053_clear;
  uint16_t pixels_per_row;
  uint16_t CurColor;
  int16_t rows_send_cnt;
  void clearBuffer(uint8_t _buff_id = 1);
  void clearDmaBuffer(uint8_t _buff_id = 0);
  void sendCBVsync();
  void sendCBRow(uint8_t buff_id);
  void fillRectBuffer(int16_t x, int16_t y, int16_t w, int16_t h);

private:
  frameStruct_t dma_buff;
  frame_buffer_t frame_buffer;
  lldesc_t *dmadesc_data[2];
  lldesc_t *dmadesc_prefix;
  lldesc_t *dmadesc_suffix[2];
  lldesc_t *dmadesc_ext;
  driver_rgb_t *driver_reg;
  bool icn2053_auto_vsync;
  bool bufferReady;
  bool bufferBusy;
  bool initialized;
  uint8_t rows_per_frame;
  uint8_t cur_suffix_id;
  uint8_t back_buffer_id;
  uint8_t driver_cur_reg;
  uint16_t prefix_to_suffix;
  uint16_t data_to_suffix;
  uint16_t desc_data_cnt;
  uint16_t desc_prefix_cnt;
  uint16_t desc_suffix_cnt;
  uint16_t driver_cnt;
  uint16_t *brightness_table;
  int dma_int_cnt;
  int32_t scroll_y;
  int32_t scroll_x;

  void buffersFree();
  bool allocateDMAmemory();
  void configureDMA();
  void icn2053initBuffers();
  void icn2053init();
  void icn2053setReg(uint8_t reg_idx, driver_rgb_t *regs_data);
  void icn2053setVSync(bool leds_enable, bool vsync);
  void fillRectFrameBuffer(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
  void getRGBColor16(int offset_y, int offset_x, rgb888_t &rgb888);
  void prepareDmaRows(uint8_t row_offset, uint8_t dma_buff_id);
};

#endif
