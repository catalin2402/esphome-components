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

extern void usb_audio_device_connection_changed(bool connected);
extern void usb_audio_device_playback_changed(bool active);
extern void usb_audio_device_capture_changed(bool active);
extern bool usb_audio_device_original_tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *request);
extern bool usb_audio_device_original_tud_audio_set_itf_close_ep_cb(uint8_t rhport,
                                                                    tusb_control_request_t const *request);
extern void usb_audio_device_original_tud_mount_cb(void);
extern void usb_audio_device_original_tud_umount_cb(void);
extern void usb_audio_device_original_tud_suspend_cb(bool remote_wakeup_en);
extern void usb_audio_device_original_tud_resume_cb(void);
static tusb_control_request_t speaker_request;
static bool speaker_active;
#if MIC_CHANNEL_NUM
static tusb_control_request_t microphone_request;
static bool microphone_active;
#endif

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *request) {
  const bool result = usb_audio_device_original_tud_audio_set_itf_cb(rhport, request);
  if (result && tu_u16_low(tu_le16toh(request->wIndex)) == ITF_NUM_AUDIO_STREAMING_SPK) {
    speaker_request = *request;
    speaker_active = tu_le16toh(request->wValue) != 0;
    usb_audio_device_playback_changed(speaker_active);
  }
#if MIC_CHANNEL_NUM
  if (result && tu_u16_low(tu_le16toh(request->wIndex)) == ITF_NUM_AUDIO_STREAMING_MIC) {
    microphone_request = *request;
    microphone_active = tu_le16toh(request->wValue) != 0;
    usb_audio_device_capture_changed(microphone_active);
  }
#endif
  return result;
}

bool tud_audio_set_itf_close_ep_cb(uint8_t rhport, tusb_control_request_t const *request) {
  if (tu_u16_low(tu_le16toh(request->wIndex)) == ITF_NUM_AUDIO_STREAMING_SPK) {
    speaker_active = false;
    usb_audio_device_playback_changed(false);
  }
#if MIC_CHANNEL_NUM
  if (tu_u16_low(tu_le16toh(request->wIndex)) == ITF_NUM_AUDIO_STREAMING_MIC) {
    microphone_active = false;
    usb_audio_device_capture_changed(false);
  }
#endif
  return usb_audio_device_original_tud_audio_set_itf_close_ep_cb(rhport, request);
}

void tud_mount_cb(void) {
  usb_audio_device_connection_changed(true);
  speaker_active = false;
#if MIC_CHANNEL_NUM
  microphone_active = false;
#endif
  usb_audio_device_capture_changed(false);
  usb_audio_device_playback_changed(false);
  usb_audio_device_original_tud_mount_cb();
}
void tud_umount_cb(void) {
  usb_audio_device_connection_changed(false);
  speaker_active = false;
#if MIC_CHANNEL_NUM
  microphone_active = false;
#endif
  usb_audio_device_capture_changed(false);
  usb_audio_device_playback_changed(false);
  usb_audio_device_original_tud_umount_cb();
}
void tud_suspend_cb(bool remote_wakeup_en) {
  usb_audio_device_capture_changed(false);
  usb_audio_device_playback_changed(false);
  usb_audio_device_original_tud_suspend_cb(remote_wakeup_en);
}

void tud_resume_cb(void) {
  usb_audio_device_original_tud_resume_cb();
  // UAC clears spk_active on suspend. The host can resume the existing
  // alternate setting without sending SET_INTERFACE again.
  if (speaker_active && usb_audio_device_original_tud_audio_set_itf_cb(0, &speaker_request))
    usb_audio_device_playback_changed(true);
#if MIC_CHANNEL_NUM
  if (microphone_active && usb_audio_device_original_tud_audio_set_itf_cb(0, &microphone_request))
    usb_audio_device_capture_changed(true);
#endif
}

//--------------------------------------------------------------------+
// Device descriptor
//--------------------------------------------------------------------+

static tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x303A,
    .idProduct = 0x8001,
    .bcdDevice = 0x0103,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void) { return (uint8_t const *) &desc_device; }

//--------------------------------------------------------------------+
// HID report descriptor
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// Configuration descriptor
//--------------------------------------------------------------------+

enum {
  ITF_NUM_COMPOSITE_TOTAL = ITF_NUM_TOTAL + CFG_TUD_HID,
  ITF_NUM_HID_CONSUMER = ITF_NUM_TOTAL,
};

#define CONFIG_TOTAL_LEN \
  (TUD_CONFIG_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO_DEVICE_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

#define EPNUM_AUDIO_OUT 0x01
#define EPNUM_AUDIO_FB 0x81
#define EPNUM_AUDIO_IN 0x82
#define EPNUM_HID_IN 0x83
_Static_assert(EPNUM_AUDIO_IN <= 0x84 && EPNUM_AUDIO_FB <= 0x84 && EPNUM_HID_IN <= 0x84,
               "IN endpoints must have hardware TX FIFOs");

static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_COMPOSITE_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_AUDIO_DESCRIPTOR(ITF_NUM_AUDIO_CONTROL, 4, EPNUM_AUDIO_OUT, EPNUM_AUDIO_IN, EPNUM_AUDIO_FB),
#if CFG_TUD_HID
    TUD_HID_DESCRIPTOR(ITF_NUM_HID_CONSUMER, 0, HID_ITF_PROTOCOL_NONE,
                       sizeof((uint8_t[]){TUD_HID_REPORT_DESC_CONSUMER()}), EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 10),
#endif
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void) index;
  return desc_configuration;
}

//--------------------------------------------------------------------+
// String descriptors
//--------------------------------------------------------------------+

static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},
    "ESPHome",
    "USB Headset",
    "ESPHOME-UAC-1",
    "USB Headset",
#if SPEAK_CHANNEL_NUM
    "speaker",
#endif
#if MIC_CHANNEL_NUM
    "microphone",
#endif
};

// Called once before tusb_init(); the strings are generated static literals.
void usb_audio_device_set_identity(uint16_t vendor, uint16_t product, const char *manufacturer, const char *name,
                                   const char *serial) {
  desc_device.idVendor = vendor;
  desc_device.idProduct = product;
  string_desc_arr[1] = manufacturer;
  string_desc_arr[2] = name;
  string_desc_arr[3] = serial;
  // Windows may use the audio function's iFunction string (index 4) for
  // endpoint names instead of the device's iProduct string (index 2).
  string_desc_arr[4] = name;
}

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
