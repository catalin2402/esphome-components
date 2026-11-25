#pragma once

#include "esphome/components/display/display.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/time.h"

#include <initializer_list>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace esphome::lcd_pcf8574_optimised {

class PCF8574OptimisedLCDDisplay;
class OptimisedLCDDisplayPage;

using display_writer_t = std::function<void(PCF8574OptimisedLCDDisplay &)>;

class PCF8574OptimisedLCDDisplay final : public PollingComponent, public i2c::I2CDevice {
 public:
  using writer_t = display::DisplayWriter<PCF8574OptimisedLCDDisplay>;
  void set_writer(writer_t &&writer) { this->writer_ = std::move(writer); }
  void set_previous_buffer(uint8_t *buffer) { this->previous_buffer_ = buffer; }
  void invalidate() { this->previous_valid_ = false; }
  void dump_config() override;
  void backlight();
  void no_backlight();

  /// `buffer` holds `columns * rows` characters, a static array from code generation
  void set_dimensions(uint8_t columns, uint8_t rows, uint8_t *buffer) {
    this->columns_ = columns;
    this->rows_ = rows;
    this->buffer_ = buffer;
  }

  void set_user_defined_char(uint8_t pos, const std::vector<uint8_t> &data) { this->user_defined_chars_[pos] = data; }

  void setup() override;
  float get_setup_priority() const override;
  void update() override;
  void display();
  /// Clear the rendering buffer; call display() to flush it to the LCD.
  void clear();

  /// Print the given text at the specified column and row.
  void print(uint8_t column, uint8_t row, const char *str);
  /// Print the given string at the specified column and row.
  void print(uint8_t column, uint8_t row, const std::string &str);
  /// Print the given text at column=0 and row=0.
  void print(const char *str);
  /// Print the given string at column=0 and row=0.
  void print(const std::string &str);
  /// Evaluate the printf-format and print the text at the specified column and row.
  void printf(uint8_t column, uint8_t row, const char *format, ...) __attribute__((format(printf, 4, 5)));
  /// Evaluate the printf-format and print the text at column=0 and row=0.
  void printf(const char *format, ...) __attribute__((format(printf, 2, 3)));

  /// Evaluate the strftime-format and print the text at the specified column and row.
  void strftime(uint8_t column, uint8_t row, const char *format, ESPTime time) __attribute__((format(strftime, 4, 0)));
  /// Evaluate the strftime-format and print the text at column=0 and row=0.
  void strftime(const char *format, ESPTime time) __attribute__((format(strftime, 2, 0)));

  /// Load custom char to given location
  void loadchar(uint8_t location, uint8_t charmap[]);

  void print_digit(int column, int row, int digit);
  void print_big_time(ESPTime time, bool show_seconds = true);

  void show_page(OptimisedLCDDisplayPage *page);
  void show_next_page();
  void show_prev_page();

  void set_pages(std::initializer_list<OptimisedLCDDisplayPage *> pages);
  void set_auto_cycle_interval(uint32_t auto_cycle_interval) { this->auto_cycle_interval_ = auto_cycle_interval; }

  const OptimisedLCDDisplayPage *get_active_page() const { return this->page_; }

 protected:
  void write_n_bits(uint8_t value, uint8_t n);
  void send(uint8_t value, bool rs);
  void write_expander_(uint8_t value);

  void setup_big_font_();
  void command_(uint8_t value);
  void call_writer() { this->writer_(*this); }

  writer_t writer_;
  uint8_t *previous_buffer_{nullptr};
  bool previous_valid_{false};
  bool write_failed_{false};
  uint8_t backlight_value_{0x08};

  uint8_t columns_{0};
  uint8_t rows_{0};
  uint8_t *buffer_{nullptr};
  std::map<uint8_t, std::vector<uint8_t> > user_defined_chars_;
  OptimisedLCDDisplayPage *page_{nullptr};
  uint32_t auto_cycle_interval_{0};
  bool big_font_available_{true};
};

class OptimisedLCDDisplayPage final {
 public:
  explicit OptimisedLCDDisplayPage(display_writer_t writer);
  void show();
  void show_next();
  void show_prev();
  void set_parent(PCF8574OptimisedLCDDisplay *parent);
  void set_prev(OptimisedLCDDisplayPage *prev);
  void set_next(OptimisedLCDDisplayPage *next);
  const display_writer_t &get_writer() const;

 protected:
  PCF8574OptimisedLCDDisplay *parent_{nullptr};
  display_writer_t writer_;
  OptimisedLCDDisplayPage *prev_{nullptr};
  OptimisedLCDDisplayPage *next_{nullptr};
};

template<typename... Ts> class OptimisedLCDDisplayPageShowAction final : public Action<Ts...> {
 public:
  TEMPLATABLE_VALUE(OptimisedLCDDisplayPage *, page)

  void play(const Ts &...x) override {
    auto *page = this->page_.value(x...);
    if (page != nullptr) {
      page->show();
    }
  }
};

}  // namespace esphome::lcd_pcf8574_optimised
