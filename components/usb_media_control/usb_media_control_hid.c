#include "tusb.h"

bool usb_media_control_mounted(void) { return tud_mounted(); }

bool usb_media_control_ready(void) { return tud_hid_ready(); }

bool usb_media_control_send_report(uint16_t usage) { return tud_hid_report(0, &usage, sizeof(usage)); }

static uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC_CONSUMER()};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
  (void) instance;
  return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t requested_length) {
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) requested_length;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t buffer_size) {
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) buffer_size;
}
