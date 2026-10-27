#include "usb_media_control.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <cinttypes>
extern "C" bool usb_media_control_mounted(void);
extern "C" bool usb_media_control_ready(void);
extern "C" bool usb_media_control_send_report(uint16_t usage);
namespace esphome::usb_media_control {
static const char *const TAG = "usb_media_control";
static constexpr uint32_t KEY_PULSE_MS = 30;
void USBMediaControl::setup() {
  this->media_keys_ = xQueueCreate(16, sizeof(uint16_t));
  if (!this->media_keys_) {
    ESP_LOGE(TAG, "Could not allocate the HID key queue");
    this->mark_failed();
    return;
  }
}
bool USBMediaControl::is_ready() const {
  return !this->shutting_down_.load() && !this->is_failed() && this->media_keys_ != nullptr &&
         usb_media_control_mounted() && usb_media_control_ready();
}
void USBMediaControl::send_media_key(uint16_t usage) {
  if (this->shutting_down_.load() || this->is_failed())
    return;
  if (!this->media_keys_ || !usb_media_control_mounted()) {
    ESP_LOGW(TAG, "HID key 0x%04X dropped: queue %s, USB mounted %s", usage, this->media_keys_ ? "ready" : "missing",
             usb_media_control_mounted() ? "YES" : "NO");
    return;
  }
  if (xQueueSend(this->media_keys_, &usage, 0) != pdTRUE) {
    ESP_LOGW(TAG, "HID key 0x%04X dropped: queue full", usage);
    return;
  }
  ESP_LOGD(TAG, "HID key queued: 0x%04X", usage);
}
void USBMediaControl::press_media_key(uint16_t usage) {
  if (this->shutting_down_.load() || this->is_failed())
    return;
  if (!usb_media_control_mounted()) {
    ESP_LOGW(TAG, "HID hold 0x%04X dropped: USB not mounted", usage);
    return;
  }
  this->media_held_.store(usage);
  ESP_LOGD(TAG, "HID hold requested: 0x%04X", usage);
}
void USBMediaControl::release_media_key(uint16_t usage) {
  // Releasing an older gesture must not cancel a newer held control.
  const uint16_t released = usage;
  if (this->media_held_.compare_exchange_strong(usage, 0))
    ESP_LOGD(TAG, "HID hold release requested: 0x%04X", released);
}
void USBMediaControl::release_all_media_keys() {
  this->media_held_.store(0);
  if (this->media_keys_)
    xQueueReset(this->media_keys_);
  this->media_waiting_ = 0;
  this->media_pulse_active_ = false;
  // loop() sends the neutral report when the HID endpoint becomes ready.
}
void USBMediaControl::loop() {
  if (!this->media_keys_ || this->shutting_down_.load())
    return;
  if (!usb_media_control_mounted()) {
    this->media_current_ = this->media_waiting_ = 0;
    this->media_held_.store(0);
    this->media_pulse_active_ = false;
    uint16_t discard;
    while (xQueueReceive(this->media_keys_, &discard, 0) == pdTRUE) {
    }
  } else if (usb_media_control_ready()) {
    const uint16_t held = this->media_held_.load();
    if (held) {
      if (this->media_current_ == held || usb_media_control_send_report(held)) {
        if (this->media_current_ != held)
          ESP_LOGD(TAG, "HID hold sent: 0x%04X", held);
        this->media_current_ = held;
        this->media_pulse_active_ = false;
      }
    } else if (this->media_current_) {
      if ((!this->media_pulse_active_ || millis() - this->media_started_ >= KEY_PULSE_MS) &&
          usb_media_control_send_report(0)) {
        if (!this->media_pulse_active_)
          ESP_LOGD(TAG, "HID hold released: 0x%04X (neutral report sent)", this->media_current_);
        this->media_current_ = 0;
        this->media_pulse_active_ = false;
      }
    } else {
      if (!this->media_waiting_)
        xQueueReceive(this->media_keys_, &this->media_waiting_, 0);
      if (this->media_waiting_ && usb_media_control_send_report(this->media_waiting_)) {
        ESP_LOGD(TAG, "HID key sent: 0x%04X", this->media_waiting_);
        this->media_current_ = this->media_waiting_;
        this->media_waiting_ = 0;
        this->media_started_ = millis();
        this->media_pulse_active_ = true;
      }
    }
  }
}
void USBMediaControl::on_shutdown() {
  this->shutting_down_.store(true);
  this->release_all_media_keys();
  if (usb_media_control_ready())
    usb_media_control_send_report(0);
}
void USBMediaControl::dump_config() {
  ESP_LOGCONFIG(TAG, "USB Media Control:");
  ESP_LOGCONFIG(TAG, "  Audio device registered: %s", YESNO(this->parent_ != nullptr));
  ESP_LOGCONFIG(TAG, "  Key pulse: %" PRIu32 " ms", KEY_PULSE_MS);
  ESP_LOGCONFIG(TAG, "  Queue capacity: 16 keys");
}
}  // namespace esphome::usb_media_control
