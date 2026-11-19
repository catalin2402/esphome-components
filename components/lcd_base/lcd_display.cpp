#include "lcd_display.h"

#include <cstring>

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::lcd_base {

ESPHOME_LOG_TAG(TAG, "lcd");

// First set bit determines command, bits after that are the data.
static const uint8_t LCD_DISPLAY_COMMAND_CLEAR_DISPLAY = 0x01;
static const uint8_t LCD_DISPLAY_COMMAND_RETURN_HOME = 0x02;
static const uint8_t LCD_DISPLAY_COMMAND_ENTRY_MODE_SET = 0x04;
static const uint8_t LCD_DISPLAY_COMMAND_DISPLAY_CONTROL = 0x08;
static const uint8_t LCD_DISPLAY_COMMAND_CURSOR_SHIFT = 0x10;
static const uint8_t LCD_DISPLAY_COMMAND_FUNCTION_SET = 0x20;
static const uint8_t LCD_DISPLAY_COMMAND_SET_CGRAM_ADDR = 0x40;
static const uint8_t LCD_DISPLAY_COMMAND_SET_DDRAM_ADDR = 0x80;

static const uint8_t LCD_DISPLAY_ENTRY_SHIFT_INCREMENT = 0x01;
static const uint8_t LCD_DISPLAY_ENTRY_LEFT = 0x02;

static const uint8_t LCD_DISPLAY_DISPLAY_BLINK_ON = 0x01;
static const uint8_t LCD_DISPLAY_DISPLAY_CURSOR_ON = 0x02;
static const uint8_t LCD_DISPLAY_DISPLAY_ON = 0x04;

static const uint8_t LCD_DISPLAY_FUNCTION_8_BIT_MODE = 0x10;
static const uint8_t LCD_DISPLAY_FUNCTION_2_LINE = 0x08;
static const uint8_t LCD_DISPLAY_FUNCTION_5X10_DOTS = 0x04;

void LCDDisplay::setup() {
  this->clear();

  uint8_t display_function = 0;

  this->setup_big_font_();

  if (!this->is_four_bit_mode())
    display_function |= LCD_DISPLAY_FUNCTION_8_BIT_MODE;

  if (this->rows_ > 1)
    display_function |= LCD_DISPLAY_FUNCTION_2_LINE;

  // TODO dotsize

  // Commands can only be sent 40ms after boot-up, so let's wait if we're close
  const uint32_t now = millis();
  if (now < 40)
    delay(40u - now);

  if (this->is_four_bit_mode()) {
    this->write_n_bits(0x03, 4);
    delay(5);  // 4.1ms
    this->write_n_bits(0x03, 4);
    delay(5);
    this->write_n_bits(0x03, 4);
    delayMicroseconds(150);
    this->write_n_bits(0x02, 4);
  } else {
    this->command_(LCD_DISPLAY_COMMAND_FUNCTION_SET | display_function);
    delay(5);  // 4.1ms
    this->command_(LCD_DISPLAY_COMMAND_FUNCTION_SET | display_function);
    delayMicroseconds(150);
    this->command_(LCD_DISPLAY_COMMAND_FUNCTION_SET | display_function);
  }

  // store user defined characters
  for (auto &user_defined_char : this->user_defined_chars_) {
    this->command_(LCD_DISPLAY_COMMAND_SET_CGRAM_ADDR | (user_defined_char.first << 3));
    for (auto data : user_defined_char.second)
      this->send(data, true);
  }

  this->command_(LCD_DISPLAY_COMMAND_FUNCTION_SET | display_function);
  uint8_t display_control = LCD_DISPLAY_DISPLAY_ON;
  this->command_(LCD_DISPLAY_COMMAND_DISPLAY_CONTROL | display_control);

  // clear display, also sets DDRAM address to 0 (home)
  this->command_(LCD_DISPLAY_COMMAND_CLEAR_DISPLAY);
  delay(2);  // 1.52ms

  uint8_t entry_mode = LCD_DISPLAY_ENTRY_LEFT;
  this->command_(LCD_DISPLAY_COMMAND_ENTRY_MODE_SET | entry_mode);  // 37µs

  this->command_(LCD_DISPLAY_COMMAND_RETURN_HOME);
  delay(2);  // 1.52ms

  if (this->auto_cycle_interval_ != 0) {
    // The scheduler staggers the first interval callback. Skip it so the first
    // page stays visible for at least one complete cycle period.
    this->set_interval(this->auto_cycle_interval_, [this, first = true]() mutable {
      if (first) {
        first = false;
        return;
      }
      this->show_next_page();
    });
  }
}

float LCDDisplay::get_setup_priority() const { return setup_priority::PROCESSOR; }
void HOT LCDDisplay::display() {
  this->command_(LCD_DISPLAY_COMMAND_SET_DDRAM_ADDR | 0);

  for (uint8_t i = 0; i < this->columns_; i++)
    this->send(this->buffer_[i], true);

  if (this->rows_ >= 3) {
    for (uint8_t i = 0; i < this->columns_; i++)
      this->send(this->buffer_[this->columns_ * 2 + i], true);
  }

  if (this->rows_ >= 2) {
    this->command_(LCD_DISPLAY_COMMAND_SET_DDRAM_ADDR | 0x40);

    for (uint8_t i = 0; i < this->columns_; i++)
      this->send(this->buffer_[this->columns_ + i], true);

    if (this->rows_ >= 4) {
      for (uint8_t i = 0; i < this->columns_; i++)
        this->send(this->buffer_[this->columns_ * 3 + i], true);
    }
  }
}
void LCDDisplay::update() {
  this->clear();
  if (this->page_ != nullptr) {
    this->page_->get_writer()(*this);
  } else {
    this->call_writer();
  }
  this->display();
}
void LCDDisplay::command_(uint8_t value) { this->send(value, false); }
void LCDDisplay::print(uint8_t column, uint8_t row, const char *str) {
  uint8_t pos = column + row * this->columns_;
  for (; *str != '\0'; str++) {
    if (*str == '\n') {
      pos = ((pos / this->columns_) + 1) * this->columns_;
      continue;
    }
    if (pos >= this->rows_ * this->columns_) {
      ESP_LOGW(TAG, "LCDDisplay writing out of range!");
      break;
    }

    this->buffer_[pos] = *reinterpret_cast<const uint8_t *>(str);
    pos++;
  }
}
void LCDDisplay::print(uint8_t column, uint8_t row, const std::string &str) { this->print(column, row, str.c_str()); }
void LCDDisplay::print(const char *str) { this->print(0, 0, str); }
void LCDDisplay::print(const std::string &str) { this->print(0, 0, str.c_str()); }
void LCDDisplay::printf(uint8_t column, uint8_t row, const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buffer[256];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    this->print(column, row, buffer);
}
void LCDDisplay::printf(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buffer[256];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    this->print(0, 0, buffer);
}
void LCDDisplay::clear() { memset(this->buffer_, ' ', this->rows_ * this->columns_); }
void LCDDisplay::strftime(uint8_t column, uint8_t row, const char *format, ESPTime time) {
  char buffer[64];
  size_t ret = time.strftime(buffer, sizeof(buffer), format);
  if (ret > 0)
    this->print(column, row, buffer);
}
void LCDDisplay::strftime(const char *format, ESPTime time) { this->strftime(0, 0, format, time); }
void LCDDisplay::loadchar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7;  // we only have 8 locations 0-7
  this->command_(LCD_DISPLAY_COMMAND_SET_CGRAM_ADDR | (location << 3));
  for (int i = 0; i < 8; i++) {
    this->send(charmap[i], true);
  }
}

void LCDDisplay::setup_big_font_() {
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

void LCDDisplay::print_digit(int column, int row, int digit) {
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

void LCDDisplay::print_big_time(ESPTime time, bool show_seconds) {
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

void LCDDisplay::set_pages(std::initializer_list<LCDDisplayPage *> pages) {
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
void LCDDisplay::show_page(LCDDisplayPage *page) { this->page_ = page; }
void LCDDisplay::show_next_page() {
  if (this->page_ != nullptr)
    this->page_->show_next();
}
void LCDDisplay::show_prev_page() {
  if (this->page_ != nullptr)
    this->page_->show_prev();
}

LCDDisplayPage::LCDDisplayPage(display_writer_t writer) : writer_(std::move(writer)) {}
void LCDDisplayPage::show() { this->parent_->show_page(this); }
void LCDDisplayPage::show_next() { this->next_->show(); }
void LCDDisplayPage::show_prev() { this->prev_->show(); }
void LCDDisplayPage::set_parent(LCDDisplay *parent) { this->parent_ = parent; }
void LCDDisplayPage::set_prev(LCDDisplayPage *prev) { this->prev_ = prev; }
void LCDDisplayPage::set_next(LCDDisplayPage *next) { this->next_ = next; }
const display_writer_t &LCDDisplayPage::get_writer() const { return this->writer_; }

}  // namespace esphome::lcd_base
