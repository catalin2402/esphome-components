/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Copyright (c) 2020 Jerzy Kasenbreg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Composite USB Audio Class + HID Consumer Control descriptors. The UAC
 * portion follows Espressif usb_device_uac 1.3.1. The additional HID interface
 * carries standard two-byte Consumer Page reports for media keys.
 */

#include <string.h>

#include "tusb.h"
#include "uac_descriptors.h"

//--------------------------------------------------------------------+
// Device descriptor
//--------------------------------------------------------------------+

static tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = CONFIG_UAC_TUSB_VID,
    .idProduct = CONFIG_UAC_TUSB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void) { return (uint8_t const *) &desc_device; }

//--------------------------------------------------------------------+
// HID report descriptor
//--------------------------------------------------------------------+

static uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_CONSUMER()
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
  (void) instance;
  return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t requested_length) {
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) requested_length;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t buffer_size) {
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) buffer_size;
}

//--------------------------------------------------------------------+
// Configuration descriptor
//--------------------------------------------------------------------+

enum {
  ITF_NUM_HID_CONSUMER = ITF_NUM_TOTAL,
  ITF_NUM_COMPOSITE_TOTAL,
};

#define CONFIG_TOTAL_LEN \
  (TUD_CONFIG_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO_DEVICE_DESC_LEN + TUD_HID_DESC_LEN)

#define EPNUM_AUDIO_OUT 0x01
#define EPNUM_AUDIO_FB 0x81
#define EPNUM_AUDIO_IN 0x82
#define EPNUM_HID_IN 0x83

static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_COMPOSITE_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_AUDIO_DESCRIPTOR(ITF_NUM_AUDIO_CONTROL, 4, EPNUM_AUDIO_OUT, EPNUM_AUDIO_IN, EPNUM_AUDIO_FB),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID_CONSUMER, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report),
                       EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 10),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void) index;
  return desc_configuration;
}

//--------------------------------------------------------------------+
// String descriptors
//--------------------------------------------------------------------+

static char const *string_desc_arr[] = {
    (const char[]) {0x09, 0x04},
    CONFIG_UAC_TUSB_MANUFACTURER,
    CONFIG_UAC_TUSB_PRODUCT,
    CONFIG_UAC_TUSB_SERIAL_NUM,
    "usb uac",
#if SPEAK_CHANNEL_NUM
    "speaker",
#endif
#if MIC_CHANNEL_NUM
    "microphone",
#endif
};

static uint16_t desc_string[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t language_id) {
  (void) language_id;

  uint8_t character_count;
  if (index == 0) {
    memcpy(&desc_string[1], string_desc_arr[0], 2);
    character_count = 1;
  } else {
    if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
      return NULL;
    }

    const char *string = string_desc_arr[index];
    character_count = (uint8_t) strlen(string);
    if (character_count > 31) {
      character_count = 31;
    }
    for (uint8_t i = 0; i < character_count; i++) {
      desc_string[1 + i] = string[i];
    }
  }

  desc_string[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * character_count + 2));
  return desc_string;
}
