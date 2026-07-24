#include "arduino_expander.h"
#include "esphome/core/log.h"

#define CMD_READ 0x01
#define CMD_SETUP_PINS 0x02
#define CMD_SETUP_INPUT_PULLUP_PINS 0x03
#define CMD_WRITE_DIGITAL_HIGH 0x04
#define CMD_WRITE_DIGITAL_LOW 0x05
#define CMD_RESTORE_OUTPUTS 0x06
#define CMD_ADC_SOURCE 0x07
static const char *TAG = "arduino_expander";

namespace esphome {
namespace arduino_expander {

void ArduinoExpander::setup() {
  ESP_LOGD(TAG, "Setting up at 0x%02X", this->address_);
  this->configure_pending_ = true;
  this->configure_started_ = millis();
}

void ArduinoExpander::dump_config() {
  ESP_LOGCONFIG(TAG, "Arduino Expander:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  ADC source: %u", this->adc_source_);
}

void ArduinoExpander::loop() {
  const uint32_t now = App.get_loop_component_start_time();
  if (this->configure_pending_) {
    bool try_configure = now % 100 > 50;
    if (try_configure == this->configure_)
      return;
    this->configure_ = try_configure;
    uint8_t data[14];
    if (i2c::ERROR_OK == this->read_register(CMD_READ, data, 14)) {
      ESP_LOGD(TAG, "Found at 0x%02X", this->address_);
      uint8_t pin_data[2];
      pin_data[0] = this->adc_source_;
      pin_data[1] = this->adc_source_;
      this->write_register(CMD_ADC_SOURCE, pin_data, 2);
      pin_data[0] = this->pin_modes_;
      pin_data[1] = this->pin_modes_ >> 8;
      this->write_register(CMD_SETUP_PINS, pin_data, 2);
      pin_data[0] = this->pullup_pins_;
      pin_data[1] = this->pullup_pins_ >> 8;
      this->write_register(CMD_SETUP_INPUT_PULLUP_PINS, pin_data, 2);
      pin_data[0] = this->output_states_;
      pin_data[1] = this->output_states_ >> 8;
      this->write_register(CMD_RESTORE_OUTPUTS, pin_data, 2);
      this->configure_pending_ = false;
      this->status_clear_error();
    }
  }

  if (this->configure_pending_ && now - this->configure_started_ >= 5000) {
    ESP_LOGE(TAG, "Not found at 0x%02X", this->address_);
    this->mark_failed();
  }
}

void ArduinoExpander::update() {
  if (!this->status_has_error()) {
    uint8_t data[14];
    uint8_t result = this->read_register(CMD_READ, data, 14);
    if (result == i2c::ERROR_OK) {
      this->input_states_ = ((uint16_t)data[1] << 8) | data[0];
      for (uint8_t i = 0; i < 6; i++) {
        this->analog_reads_[i] =
            ((uint16_t)data[i * 2 + 3] << 8) | data[i * 2 + 2];
      }
    } else {
      this->status_set_error();
      this->configure_pending_ = true;
      this->configure_started_ = millis();
      ESP_LOGD(TAG, "Reconfiguring");
    }
  }
}

bool ArduinoExpander::digital_read(uint8_t pin) {
  return this->input_states_ & (1 << pin);
}

void ArduinoExpander::digital_write(uint8_t pin, bool value) {
  uint16_t state = (this->output_states_ & ~(1 << pin)) | (value << pin);
  this->output_states_ = state;
  this->write_register(value ? CMD_WRITE_DIGITAL_HIGH : CMD_WRITE_DIGITAL_LOW,
                       &pin, 1);
}

uint16_t ArduinoExpander::read_analog(uint8_t pin) {
  return this->analog_reads_[(pin >= 6) ? pin - 2 : pin];
}

void ArduinoExpander::pin_mode(uint8_t pin, gpio::Flags flags) {
  if (flags == gpio::FLAG_INPUT) {
    this->pin_modes_ &= ~(1U << pin);
    this->pullup_pins_ &= ~(1U << pin);
  } else if (flags == gpio::FLAG_OUTPUT) {
    this->pin_modes_ |= 1U << pin;
    this->pullup_pins_ &= ~(1U << pin);
  } else if (flags == (gpio::FLAG_INPUT | gpio::FLAG_PULLUP)) {
    this->pin_modes_ &= ~(1U << pin);
    this->pullup_pins_ |= 1U << pin;
  }
}

void ArduinoGPIOPin::setup() { this->pin_mode(this->flags_); }
void ArduinoGPIOPin::pin_mode(gpio::Flags flags) {
  this->parent_->pin_mode(this->pin_, flags);
}
bool ArduinoGPIOPin::digital_read() {
  return this->parent_->digital_read(this->pin_) != this->inverted_;
}
void ArduinoGPIOPin::digital_write(bool value) {
  this->parent_->digital_write(this->pin_, value != this->inverted_);
}
size_t ArduinoGPIOPin::dump_summary(char *buffer, size_t len) const {
  return snprintf(buffer, len, "D%u via ArduinoExpander", this->pin_);
}

} // namespace arduino_expander
} // namespace esphome
