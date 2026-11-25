#include "pcf8574_optimised_display.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::lcd_pcf8574_optimised {

ESPHOME_LOG_TAG(TAG, "lcd_pcf8574_optimised");

// First set bit determines command, bits after that are the data.
static const uint8_t LCD_DISPLAY_COMMAND_CLEAR_DISPLAY = 0x01;
static const uint8_t LCD_DISPLAY_COMMAND_ENTRY_MODE_SET = 0x04;
static const uint8_t LCD_DISPLAY_COMMAND_DISPLAY_CONTROL = 0x08;
static const uint8_t LCD_DISPLAY_COMMAND_FUNCTION_SET = 0x20;
static const uint8_t LCD_DISPLAY_COMMAND_SET_CGRAM_ADDR = 0x40;
static const uint8_t LCD_DISPLAY_COMMAND_SET_DDRAM_ADDR = 0x80;

static const uint8_t LCD_DISPLAY_ENTRY_LEFT = 0x02;

static const uint8_t LCD_DISPLAY_DISPLAY_ON = 0x04;

static const uint8_t LCD_DISPLAY_FUNCTION_2_LINE = 0x08;

void PCF8574OptimisedLCDDisplay::setup() {
  if (!this->setup_check(this->write_bytes(this->backlight_value_, nullptr, 0)))
    return;
  this->clear();
  this->setup_big_font_();

  const uint32_t now = millis();
  if (now < 40)
    delay(40u - now);

  // Initialise the controller into 4-bit mode before configuring CGRAM.
  this->write_n_bits(0x03, 4);
  delay(5);
  this->write_n_bits(0x03, 4);
  delay(5);
  this->write_n_bits(0x03, 4);
  delayMicroseconds(150);
  this->write_n_bits(0x02, 4);

  this->command_(LCD_DISPLAY_COMMAND_FUNCTION_SET | (this->rows_ > 1 ? LCD_DISPLAY_FUNCTION_2_LINE : 0));
  this->command_(LCD_DISPLAY_COMMAND_DISPLAY_CONTROL | LCD_DISPLAY_DISPLAY_ON);
  this->command_(LCD_DISPLAY_COMMAND_CLEAR_DISPLAY);
  delay(2);
  this->command_(LCD_DISPLAY_COMMAND_ENTRY_MODE_SET | LCD_DISPLAY_ENTRY_LEFT);

  for (const auto &character : this->user_defined_chars_) {
    this->command_(LCD_DISPLAY_COMMAND_SET_CGRAM_ADDR | (character.first << 3));
    for (auto data : character.second)
      this->send(data, true);
  }
  this->command_(LCD_DISPLAY_COMMAND_SET_DDRAM_ADDR);
  this->invalidate();
  if (this->write_failed_)
    this->status_set_warning();

  if (this->auto_cycle_interval_ != 0) {
    this->set_interval(this->auto_cycle_interval_, [this, first = true]() mutable {
      if (first) {
        first = false;
        return;
      }
      this->show_next_page();
    });
  }
}

void PCF8574OptimisedLCDDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "Optimised PCF8574 LCD Display:\n  Columns: %u, Rows: %u", this->columns_, this->rows_);
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
}

float PCF8574OptimisedLCDDisplay::get_setup_priority() const { return setup_priority::PROCESSOR; }
void PCF8574OptimisedLCDDisplay::display() {
  if (this->is_failed() || this->previous_buffer_ == nullptr)
    return;

  this->write_failed_ = false;
  for (uint8_t row = 0; row < this->rows_; row++) {
    // HD44780 row starts: 0x00, 0x40, columns, 0x40 + columns.
    const uint8_t row_address = (row % 2 == 0 ? 0x00 : 0x40) + (row / 2) * this->columns_;
    bool cursor_positioned = false;
    for (uint8_t column = 0; column < this->columns_; column++) {
      const unsigned pos = row * this->columns_ + column;
      if (this->previous_valid_ && this->buffer_[pos] == this->previous_buffer_[pos]) {
        cursor_positioned = false;
        continue;
      }
      // One address command per contiguous run of changed characters.
      if (!cursor_positioned) {
        this->command_(0x80 | (row_address + column));
        cursor_positioned = true;
      }
      this->send(this->buffer_[pos], true);
    }
  }

  if (this->write_failed_) {
    this->invalidate();
    this->status_set_warning();
    return;
  }
  std::memcpy(this->previous_buffer_, this->buffer_, this->columns_ * this->rows_);
  this->previous_valid_ = true;
  this->status_clear_warning();
}

void PCF8574OptimisedLCDDisplay::update() {
  this->clear();
  if (this->page_ != nullptr) {
    this->page_->get_writer()(*this);
  } else {
    this->call_writer();
  }
  this->display();
}
void PCF8574OptimisedLCDDisplay::command_(uint8_t value) { this->send(value, false); }
void PCF8574OptimisedLCDDisplay::print(uint8_t column, uint8_t row, const char *str) {
  if (column >= this->columns_ || row >= this->rows_)
    return;
  size_t pos = column + row * this->columns_;
  for (; *str != '\0'; str++) {
    if (*str == '\n') {
      pos = ((pos / this->columns_) + 1) * this->columns_;
      continue;
    }
    if (pos >= this->rows_ * this->columns_) {
      ESP_LOGW(TAG, "PCF8574OptimisedLCDDisplay writing out of range!");
      break;
    }

    this->buffer_[pos] = *reinterpret_cast<const uint8_t *>(str);
    pos++;
  }
}
void PCF8574OptimisedLCDDisplay::print(uint8_t column, uint8_t row, const std::string &str) { this->print(column, row, str.c_str()); }
void PCF8574OptimisedLCDDisplay::print(const char *str) { this->print(0, 0, str); }
void PCF8574OptimisedLCDDisplay::print(const std::string &str) { this->print(0, 0, str.c_str()); }
void PCF8574OptimisedLCDDisplay::printf(uint8_t column, uint8_t row, const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buffer[256];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    this->print(column, row, buffer);
}
void PCF8574OptimisedLCDDisplay::printf(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buffer[256];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    this->print(0, 0, buffer);
}
void PCF8574OptimisedLCDDisplay::clear() { memset(this->buffer_, ' ', this->rows_ * this->columns_); }
void PCF8574OptimisedLCDDisplay::strftime(uint8_t column, uint8_t row, const char *format, ESPTime time) {
  char buffer[64];
  size_t ret = time.strftime(buffer, sizeof(buffer), format);
  if (ret > 0)
    this->print(column, row, buffer);
}
void PCF8574OptimisedLCDDisplay::strftime(const char *format, ESPTime time) { this->strftime(0, 0, format, time); }
void PCF8574OptimisedLCDDisplay::loadchar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7;  // we only have 8 locations 0-7
  this->command_(LCD_DISPLAY_COMMAND_SET_CGRAM_ADDR | (location << 3));
  for (int i = 0; i < 8; i++) {
    this->send(charmap[i], true);
  }
}

void PCF8574OptimisedLCDDisplay::setup_big_font_() {
  static constexpr uint8_t BIG_FONT[8][8] PROGMEM = {
      {0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}, {0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}, {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F}, {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1C},
      {0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x1F}, {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F},
  };
  std::vector<uint8_t> glyph(8);
  for (uint8_t position = 0; position < 8; position++) {
    progmem_memcpy(glyph.data(), BIG_FONT[position], glyph.size());
    const auto configured = this->user_defined_chars_.find(position);
    if (configured == this->user_defined_chars_.end()) {
      this->set_user_defined_char(position, glyph);
    } else if (configured->second != glyph) {
      this->big_font_available_ = false;
    }
  }
  if (!this->big_font_available_)
    ESP_LOGW(TAG, "Custom characters conflict with the clock font; large digit/time rendering disabled");
}

void PCF8574OptimisedLCDDisplay::print_digit(int column, int row, int digit) {
  if (!this->big_font_available_ || column < 0 || row < 0 || column > int(this->columns_) - 3 ||
      row > int(this->rows_) - 2 || digit < 0 || digit > 9)
    return;
  switch (digit) {
    case 0:
      this->print(column, row, "\x08\x01\x02");
      this->print(column, row + 1, "\x03\x04\x05");
      break;
    case 1:
      this->print(column, row, "\x01\x02 ");
      this->print(column, row + 1, "\x04\x07\x04");
      break;
    case 2:
      this->print(column, row, "\x06\x06\x02");
      this->print(column, row + 1, "\x03\x04\x04");
      break;
    case 3:
      this->print(column, row, "\x06\x06\x02");
      this->print(column, row + 1, "\x04\x04\x05");
      break;
    case 4:
      this->print(column, row, "\x03\x04\x07");
      this->print(column, row + 1, "  \x07");
      break;
    case 5:
      this->print(column, row, "\x03\x06\x06");
      this->print(column, row + 1, "\x04\x04\x05");
      break;
    case 6:
      this->print(column, row, "\x08\x06\x06");
      this->print(column, row + 1, "\x03\x04\x05");
      break;
    case 7:
      this->print(column, row, "\x01\x01\x02");
      this->print(column, row + 1, "  \x07");
      break;
    case 8:
      this->print(column, row, "\x08\x06\x02");
      this->print(column, row + 1, "\x03\x04\x05");
      break;
    case 9:
      this->print(column, row, "\x08\x06\x02");
      this->print(column, row + 1, "\x04\x04\x05");
      break;
  }
}

void PCF8574OptimisedLCDDisplay::print_big_time(ESPTime time, bool show_seconds) {
  if (!this->big_font_available_ || this->columns_ < 14 || this->rows_ < 2)
    return;
  const int columns_center = this->columns_ / 2 - 1;
  const int rows_center = this->rows_ / 2 - 1;

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
    char digit[2] = {static_cast<char>('0' + time.second / 10), '\0'};
    this->print(this->columns_ - 1, rows_center, digit);
    digit[0] = static_cast<char>('0' + time.second % 10);
    this->print(this->columns_ - 1, rows_center + 1, digit);
  }
}

void PCF8574OptimisedLCDDisplay::set_pages(std::initializer_list<OptimisedLCDDisplayPage *> pages) {
  if (pages.size() == 0)
    return;
  auto *first = *pages.begin();
  auto *previous = *(pages.end() - 1);
  for (auto *page : pages) {
    page->set_parent(this);
    page->set_prev(previous);
    previous->set_next(page);
    previous = page;
  }
  this->show_page(first);
}
void PCF8574OptimisedLCDDisplay::show_page(OptimisedLCDDisplayPage *page) { this->page_ = page; }
void PCF8574OptimisedLCDDisplay::show_next_page() {
  if (this->page_ != nullptr)
    this->page_->show_next();
}
void PCF8574OptimisedLCDDisplay::show_prev_page() {
  if (this->page_ != nullptr)
    this->page_->show_prev();
}

OptimisedLCDDisplayPage::OptimisedLCDDisplayPage(display_writer_t writer) : writer_(std::move(writer)) {}
void OptimisedLCDDisplayPage::show() { this->parent_->show_page(this); }
void OptimisedLCDDisplayPage::show_next() { this->next_->show(); }
void OptimisedLCDDisplayPage::show_prev() { this->prev_->show(); }
void OptimisedLCDDisplayPage::set_parent(PCF8574OptimisedLCDDisplay *parent) { this->parent_ = parent; }
void OptimisedLCDDisplayPage::set_prev(OptimisedLCDDisplayPage *prev) { this->prev_ = prev; }
void OptimisedLCDDisplayPage::set_next(OptimisedLCDDisplayPage *next) { this->next_ = next; }
const display_writer_t &OptimisedLCDDisplayPage::get_writer() const { return this->writer_; }

void PCF8574OptimisedLCDDisplay::write_expander_(uint8_t value) {
  if (this->write_bytes(value, nullptr, 0) != i2c::ERROR_OK) {
    this->write_failed_ = true;
    this->invalidate();
  }
}

void PCF8574OptimisedLCDDisplay::write_n_bits(uint8_t value, uint8_t n) {
  if (n == 4)
    value <<= 4;
  const uint8_t data = value | this->backlight_value_;
  this->write_expander_(data);
  this->write_expander_(data | 0x04);
  delayMicroseconds(1);
  this->write_expander_(data);
  // Preserve the standard driver's timing for HD44780-compatible controllers.
  delayMicroseconds(100);
}

void PCF8574OptimisedLCDDisplay::send(uint8_t value, bool rs) {
  this->write_n_bits((value & 0xF0) | rs, 0);
  this->write_n_bits(((value << 4) & 0xF0) | rs, 0);
}

void PCF8574OptimisedLCDDisplay::backlight() {
  this->backlight_value_ = 0x08;
  this->write_expander_(this->backlight_value_);
}

void PCF8574OptimisedLCDDisplay::no_backlight() {
  this->backlight_value_ = 0x00;
  this->write_expander_(this->backlight_value_);
}

}  // namespace esphome::lcd_pcf8574_optimised
