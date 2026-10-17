#include "icn2053_transport.h"

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_intr_alloc.h"
#include "esp_private/periph_ctrl.h"
#include "esp_rom_gpio.h"
#include "freertos/FreeRTOS.h"
#include "soc/gpio_periph.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2s_periph.h"
#include "soc/i2s_struct.h"
#include "soc/soc.h"

typedef struct {
  intr_handle_t interrupt_handle;
} icn2053_transport_state_t;

static icn2053_transport_state_t *s_state;
static icn2053_transport_callback_t s_callback;
static portMUX_TYPE s_callback_mutex = portMUX_INITIALIZER_UNLOCKED;

static void IRAM_ATTR interrupt_handler(void *argument) {
  (void) argument;
  SET_PERI_REG_BITS(I2S_INT_CLR_REG(1), I2S_OUT_EOF_INT_CLR_V, 1, I2S_OUT_EOF_INT_CLR_S);
  portENTER_CRITICAL_ISR(&s_callback_mutex);
  if (s_callback != NULL) {
    s_callback();
  }
  portEXIT_CRITICAL_ISR(&s_callback_mutex);
}

static void configure_output(const int gpio, const int signal) {
  if (gpio < 0) {
    return;
  }
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[gpio], PIN_FUNC_GPIO);
  gpio_set_direction((gpio_num_t) gpio, GPIO_MODE_OUTPUT);
  esp_rom_gpio_connect_out_signal(gpio, signal, false, false);
}

static void reset_dma(i2s_dev_t *device) {
  device->lc_conf.in_rst = 1;
  device->lc_conf.in_rst = 0;
  device->lc_conf.out_rst = 1;
  device->lc_conf.out_rst = 0;
}

static void reset_fifo(i2s_dev_t *device) {
  device->conf.rx_fifo_reset = 1;
  device->conf.rx_fifo_reset = 0;
  device->conf.tx_fifo_reset = 1;
  device->conf.tx_fifo_reset = 0;
}

static void reset_device(i2s_dev_t *device) {
  reset_fifo(device);
  reset_dma(device);
  device->conf.rx_reset = 1;
  device->conf.tx_reset = 1;
  device->conf.rx_reset = 0;
  device->conf.tx_reset = 0;
}

void icn2053_link_dma_descriptor(volatile lldesc_t *descriptor, volatile lldesc_t *previous, const void *memory,
                                 size_t size) {
  if (size > ICN2053_DMA_MAX_BYTES) {
    size = ICN2053_DMA_MAX_BYTES;
  }

  descriptor->size = size;
  descriptor->length = size;
  descriptor->buf = memory;
  descriptor->eof = 0;
  descriptor->sosf = 0;
  descriptor->owner = 1;
  descriptor->qe.stqe_next = NULL;
  descriptor->offset = 0;

  if (previous != NULL) {
    previous->qe.stqe_next = (lldesc_t *) descriptor;
  }
}

esp_err_t icn2053_transport_install(const icn2053_transport_config_t *configuration) {
  if (configuration == NULL || s_state != NULL || configuration->clock_hz == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  const uint32_t clock_divider = 80000000UL / configuration->clock_hz / 2U;
  if (clock_divider < 2 || clock_divider > 0xff) {
    return ESP_ERR_INVALID_ARG;
  }

  s_state = heap_caps_calloc(1, sizeof(icn2053_transport_state_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (s_state == NULL) {
    return ESP_ERR_NO_MEM;
  }

  periph_module_reset(PERIPH_I2S1_MODULE);
  periph_module_enable(PERIPH_I2S1_MODULE);

  i2s_dev_t *device = &I2S1;
  reset_device(device);

  device->conf2.val = 0;
  device->conf2.lcd_en = 1;
  device->sample_rate_conf.val = 0;
  device->sample_rate_conf.rx_bits_mod = 16;
  device->sample_rate_conf.tx_bits_mod = 16;
  device->sample_rate_conf.rx_bck_div_num = 1;
  device->sample_rate_conf.tx_bck_div_num = 2;
  device->clkm_conf.val = 0;
  device->clkm_conf.clka_en = 0;
  device->clkm_conf.clkm_div_b = 0;
  device->clkm_conf.clkm_div_a = 1;
  device->clkm_conf.clkm_div_num = clock_divider;
  device->fifo_conf.val = 0;
  device->fifo_conf.rx_fifo_mod_force_en = 1;
  device->fifo_conf.tx_fifo_mod_force_en = 1;
  device->fifo_conf.tx_fifo_mod = 1;
  device->fifo_conf.rx_data_num = 32;
  device->fifo_conf.tx_data_num = 32;
  device->fifo_conf.dscr_en = 1;
  device->conf1.val = 0;
  device->conf1.tx_stop_en = 0;
  device->conf1.tx_pcm_bypass = 1;
  device->conf_chan.val = 0;
  device->conf_chan.tx_chan_mod = 1;
  device->conf_chan.rx_chan_mod = 1;
  device->conf.tx_right_first = 0;
  device->conf.rx_right_first = 0;
  device->timing.val = 0;

  const esp_err_t interrupt_result = esp_intr_alloc(ETS_I2S1_INTR_SOURCE, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1,
                                                    interrupt_handler, NULL, &s_state->interrupt_handle);
  if (interrupt_result != ESP_OK) {
    heap_caps_free(s_state);
    s_state = NULL;
    periph_module_disable(PERIPH_I2S1_MODULE);
    return interrupt_result;
  }

  device->int_ena.out_eof = 1;
  for (int index = 0; index < 16; ++index) {
    configure_output(configuration->gpio_bus[index], I2S1O_DATA_OUT8_IDX + index);
  }
  configure_output(configuration->gpio_clk, I2S1O_WS_OUT_IDX);
  if (configuration->clock_phase_inverted) {
    GPIO.func_out_sel_cfg[configuration->gpio_clk].inv_sel = 1;
  }
  return ESP_OK;
}

esp_err_t icn2053_transport_start(lldesc_t *descriptor) {
  if (s_state == NULL || descriptor == NULL) {
    return ESP_ERR_INVALID_STATE;
  }

  i2s_dev_t *device = &I2S1;
  device->out_link.stop = 1;
  device->out_link.start = 0;
  device->conf.tx_start = 0;
  reset_device(device);

  device->lc_conf.val = I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN;
  device->out_link.addr = (uint32_t) descriptor;
  device->out_link.start = 1;
  device->conf.tx_start = 1;
  return ESP_OK;
}

void icn2053_transport_stop(void) {
  if (s_state == NULL) {
    return;
  }
  I2S1.out_link.stop = 1;
  I2S1.out_link.start = 0;
  I2S1.conf.tx_start = 0;
}

void icn2053_transport_uninstall(void) {
  if (s_state == NULL) {
    return;
  }

  icn2053_transport_set_callback(NULL);
  icn2053_transport_stop();
  esp_intr_disable(s_state->interrupt_handle);
  I2S1.int_ena.val = 0;
  I2S1.int_clr.val = I2S1.int_raw.val;
  esp_intr_free(s_state->interrupt_handle);
  heap_caps_free(s_state);
  s_state = NULL;
  s_callback = NULL;
  periph_module_disable(PERIPH_I2S1_MODULE);
}

void icn2053_transport_set_callback(const icn2053_transport_callback_t callback) {
  portENTER_CRITICAL(&s_callback_mutex);
  s_callback = callback;
  portEXIT_CRITICAL(&s_callback_mutex);
}
