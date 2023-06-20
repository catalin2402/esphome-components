#include "qr_reader.h"
#include "esphome/core/log.h"

namespace esphome {
namespace qr_reader {

static const char ASCII_CR = '\r';
static const char ASCII_LF = '\n';
static const char *const TAG = "qr_reader";

void QrReader::loop() {
  uint8_t data;
  while (this->available() > 0) {
    if (this->read_byte(&data)) {
      buffer_ += (char)data;
      this->check_buffer_();
    }
  }
}

void QrReader::check_buffer_() {
  if (this->buffer_.back() == ASCII_LF) {
    this->buffer_.pop_back();
    if (this->buffer_.back() == ASCII_CR) {
      this->buffer_.pop_back();
      this->sanitize_buffer_();
      ESP_LOGD(TAG, "Read from serial: %s", this->buffer_.c_str());
      this->publish_state(this->buffer_);
      this->buffer_.clear();
      delay(100);
      this->publish_state("");
    }
  }
}

void QrReader::dump_config() { LOG_TEXT_SENSOR("", "QrReader", this); }

void QrReader::sanitize_buffer_() {
  this->buffer_.erase(remove_if(this->buffer_.begin(), this->buffer_.end(),
                                [](char c) { return !(c >= 0 && c < 128); }),
                      this->buffer_.end());
}

} // namespace qr_reader
} // namespace esphome
