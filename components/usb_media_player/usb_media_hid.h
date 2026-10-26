#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool usb_audio_hid_mounted(void);
bool usb_audio_hid_ready(void);
bool usb_audio_hid_send_report(uint16_t usage);

#ifdef __cplusplus
}
#endif
