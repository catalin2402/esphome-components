#include "pcf8574_display.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace lcd_pcf8574 {

static const char *const TAG = "lcd_pcf8574";

static const uint8_t LCD_DISPLAY_BACKLIGHT_ON = 0x08;
static const uint8_t LCD_DISPLAY_BACKLIGHT_OFF = 0x00;

void PCF8574LCDDisplay::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PCF8574 LCD Display...");
  this->backlight_value_ = LCD_DISPLAY_BACKLIGHT_ON;
  if (!this->write_bytes(this->backlight_value_, nullptr, 0)) {
    this->mark_failed();
    return;
  }
  this->set_user_defined_char(0,
                              {0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F});
  this->set_user_defined_char(1,
                              {0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00});
  this->set_user_defined_char(2,
                              {0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F});
  this->set_user_defined_char(3,
                              {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07});
  this->set_user_defined_char(4,
                              {0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F});
  this->set_user_defined_char(5,
                              {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1C});
  this->set_user_defined_char(6,
                              {0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x1F});
  this->set_user_defined_char(7,
                              {0x1F, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F});
  LCDDisplay::setup();
}
void PCF8574LCDDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "PCF8574 LCD Display:");
  ESP_LOGCONFIG(TAG, "  Columns: %u, Rows: %u", this->columns_, this->rows_);
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with LCD Display failed!");
  }
}
void PCF8574LCDDisplay::write_n_bits(uint8_t value, uint8_t n) {
  if (n == 4) {
    // Ugly fix: in the super setup() with n == 4 value needs to be shifted left
    value <<= 4;
  }
  uint8_t data = value | this->backlight_value_; // Set backlight state
  this->write_bytes(data, nullptr, 0);
  // Pulse ENABLE
  this->write_bytes(data | 0x04, nullptr, 0);
  delayMicroseconds(1); // >450ns
  this->write_bytes(data, nullptr, 0);
  delayMicroseconds(100); // >37us
}
void PCF8574LCDDisplay::send(uint8_t value, bool rs) {
  this->write_n_bits((value & 0xF0) | rs, 0);
  this->write_n_bits(((value << 4) & 0xF0) | rs, 0);
}
void PCF8574LCDDisplay::backlight() {
  this->backlight_value_ = LCD_DISPLAY_BACKLIGHT_ON;
  this->write_bytes(this->backlight_value_, nullptr, 0);
}
void PCF8574LCDDisplay::no_backlight() {
  this->backlight_value_ = LCD_DISPLAY_BACKLIGHT_OFF;
  this->write_bytes(this->backlight_value_, nullptr, 0);
}

void PCF8574LCDDisplay::print_digit(int column, int row, int digit) {
  switch (digit) {
  case 0:
    this->print(column, row, "\x08\x01\x02");
    this->print(column, row + 1, "\x03\x04\x05");
    break;
  case 1:
    this->print(column, row, "\x01\x02 ");
    this->print(column, row + 1, " \xff ");
    break;
  case 2:
    this->print(column, row, "\x06\x06\x02");
    this->print(column, row + 1, "\x03\x07\x07");
    break;
  case 3:
    this->print(column, row, "\x06\x06\x02");
    this->print(column, row + 1, "\x07\x07\x05");
    break;
  case 4:
    this->print(column, row, "\x03\x04\x02");
    this->print(column, row + 1, "  \xff");
    break;
  case 5:
    this->print(column, row, "\xff\x06\x06");
    this->print(column, row + 1, "\x07\x07\x05");
    break;
  case 6:
    this->print(column, row, "\x08\x06\x06");
    this->print(column, row + 1, "\x03\x07\x05");
    break;
  case 7:
    this->print(column, row, "\x01\x01\x02");
    this->print(column, row + 1, " \x08 ");
    break;
  case 8:
    this->print(column, row, "\x08\x06\x02");
    this->print(column, row + 1, "\x03\x07\x05");
    break;
  case 9:
    this->print(column, row, "\x08\x06\x02");
    this->print(column, row + 1, " \x04\x05");
    break;
  }
}
void PCF8574LCDDisplay::print_big_time(ESPTime time, bool show_seconds) {
  int columns_center = this->columns_ / 2 - 1;
  int rows_center = this->rows_ / 2 - 1;

  this->print_digit(columns_center - 6, rows_center, time.hour / 10);
  this->print_digit(columns_center - 3, rows_center, time.hour % 10);
  this->print_digit(columns_center + 1, rows_center, time.minute / 10);
  this->print_digit(columns_center + 4, rows_center, time.minute % 10);

  if (time.second % 2) {
    this->print(columns_center, rows_center, "\xa5");
    this->print(columns_center, rows_center + 1, "\xa5");
  } else {
    this->print(columns_center, rows_center, " ");
    this->print(columns_center, rows_center + 1, " ");
  }

  if (show_seconds) {
    this->print(this->columns_ - 1, rows_center, to_string(time.second / 10));
    this->print(this->columns_ - 1, rows_center + 1,
                to_string(time.second % 10));
  }
}
} // namespace lcd_pcf8574
} // namespace esphome
