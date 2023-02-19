#include "arduino_expander.h"

#define CMD_READ_DIGITAL 0x01
#define CMD_SETUP_PINS 0x02
#define CMD_SETUP_ANALOG_PINS 0x03
#define CMD_SETUP_INPUT_PULLUP_PINS 0x04
#define CMD_WRITE_DIGITAL_HIGH 0x05
#define CMD_WRITE_DIGITAL_LOW 0x06
#define CMD_RESTORE_OUTPUTS 0x07
#define CMD_READ_ANALOG 0x10
static const char *TAGape = "arduino_expander";

namespace esphome {
namespace arduino_expander {

void ArduinoExpander::setup() {
  ESP_LOGD(TAGape, "Setting up ArduinoExpander at %#02x ...", address_);
  this->configure_timeout_ = millis() + 5000;
}

void ArduinoExpander::loop() {
  if (millis() < this->configure_timeout_) {
    bool try_configure = millis() % 100 > 50;
    if (try_configure == this->configure_)
      return;
    this->configure_ = try_configure;
    uint8_t data[2]{0, 0};
    if (i2c::ERROR_OK == this->read_register(CMD_READ_DIGITAL, data, 2)) {
      ESP_LOGD(TAGape, "ArduinoExpander found at %#02x", address_);
      uint8_t pin_data[2];
      pin_data[0] = this->pin_modes_;
      pin_data[1] = this->pin_modes_ >> 8;
      this->write_register(CMD_SETUP_PINS, pin_data, 2);
      pin_data[0] = this->pullup_pins_;
      pin_data[1] = this->pullup_pins_ >> 8;
      this->write_register(CMD_SETUP_INPUT_PULLUP_PINS, pin_data, 2);
      pin_data[0] = this->analog_pins_;
      pin_data[1] = this->analog_pins_;
      this->write_register(CMD_SETUP_ANALOG_PINS, pin_data, 2);
      pin_data[0] = this->output_states_;
      pin_data[1] = this->output_states_ >> 8;
      this->write_register(CMD_RESTORE_OUTPUTS, pin_data, 2);
      this->configure_timeout_ = 0;
      this->status_clear_error();
    }
  }

  if (this->configure_timeout_ != 0 && millis() > this->configure_timeout_) {
    ESP_LOGD(TAGape, "ArduinoExpander NOT found at %#02x", address_);
    this->mark_failed();
    return;
  }
}

void ArduinoExpander::update() {
  if (!this->status_has_error()) {
    uint8_t data[2]{0, 0};
    uint8_t result = this->read_register(CMD_READ_DIGITAL, data, 2);
    if (result == i2c::ERROR_OK) {
      this->input_states_ = ((uint16_t)data[1] << 8) | data[0];
    } else {
      this->status_set_error();
      this->configure_timeout_ = millis() + 5000;
      ESP_LOGD(TAGape, "Reconfiguring ArduinoExpander");
    }
    for (int i = 0; i < 8; i++) {
      if (this->analog_pins_ & (1 << i)) {
        data[0] = 0;
        data[1] = 0;
        uint8_t result = this->read_register(CMD_READ_ANALOG + i, data, 2);
        if (result == i2c::ERROR_OK) {
          this->analog_reads_[i] = ((uint16_t)data[1] << 8) | data[0];
        }
      }
    }
  }
}

bool ArduinoExpander::digital_read(uint8_t pin) {
  return this->input_states_ & (1 << pin);
}

void ArduinoExpander::digital_write(uint8_t pin, bool value) {
  uint16_t state = this->output_states_ |= (value << pin);
  this->output_states_ = state;
  this->write_register(value ? CMD_WRITE_DIGITAL_HIGH : CMD_WRITE_DIGITAL_LOW,
                       &pin, 1);
}

uint16_t ArduinoExpander::read_analog(uint8_t pin) {
  return this->analog_reads_[pin];
}

void ArduinoExpander::set_analog_pin(uint8_t pin) {
  uint8_t state = this->analog_pins_ |= (1 << pin);
  this->analog_pins_ = state;
  uint8_t pin_data[2];
  pin_data[0] = this->analog_pins_;
  pin_data[1] = this->analog_pins_;
  ESP_LOGD(TAGape, "Analog pins %d", pin_data[0]);
  this->write_register(CMD_SETUP_ANALOG_PINS, pin_data, 2);
}

void ArduinoExpander::pin_mode(uint8_t pin, gpio::Flags flags) {
  if (flags == gpio::FLAG_INPUT) {
    uint16_t state = this->pin_modes_ |= (0 << pin);
    this->pin_modes_ = state;
  } else if (flags == gpio::FLAG_OUTPUT) {
    uint16_t state = this->pin_modes_ |= (1 << pin);
    this->pin_modes_ = state;
  } else if (flags == (gpio::FLAG_INPUT | gpio::FLAG_PULLUP)) {
    uint16_t state = this->pullup_pins_ |= (1 << pin);
    this->pullup_pins_ = state;
  }
}

void ArduinoGPIOPin::setup() { pin_mode(flags_); }
void ArduinoGPIOPin::pin_mode(gpio::Flags flags) {
  this->parent_->pin_mode(this->pin_, flags);
}
bool ArduinoGPIOPin::digital_read() {
  return this->parent_->digital_read(this->pin_) != this->inverted_;
}
void ArduinoGPIOPin::digital_write(bool value) {
  this->parent_->digital_write(this->pin_, value != this->inverted_);
}
std::string ArduinoGPIOPin::dump_summary() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%u via ArduinoExpander", pin_);
  return buffer;
}

} // namespace arduino_expander
} // namespace esphome