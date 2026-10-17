#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "rom/lldesc.h"

#ifdef __cplusplus
extern "C" {

#endif

#define ICN2053_DMA_MAX_BYTES (4096 - 4)

typedef struct {
  int gpio_bus[16];
  int gpio_clk;
  uint32_t clock_hz;
  bool clock_phase_inverted;
} icn2053_transport_config_t;

void icn2053_link_dma_descriptor(volatile lldesc_t *descriptor, volatile lldesc_t *previous, const void *memory,
                                 size_t size);

esp_err_t icn2053_transport_install(const icn2053_transport_config_t *configuration);

esp_err_t icn2053_transport_start(lldesc_t *descriptor);

void icn2053_transport_stop(void);

void icn2053_transport_uninstall(void);

typedef void (*icn2053_transport_callback_t)(void);

void icn2053_transport_set_callback(icn2053_transport_callback_t callback);

#ifdef __cplusplus
}
#endif
