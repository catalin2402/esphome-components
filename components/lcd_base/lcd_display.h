#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/time.h"

#include <initializer_list>
#include <map>
#include <vector>

namespace esphome::lcd_base {

class LCDDisplay;
class LCDDisplayPage;

using display_writer_t = std::function<void(LCDDisplay &)>;

class LCDDisplay : public PollingComponent {
 public:
  /// `buffer` holds `columns * rows` characters, a static array from code generation
  void set_dimensions(uint8_t columns, uint8_t rows, uint8_t *buffer) {
    this->columns_ = columns;
    this->rows_ = rows;
    this->buffer_ = buffer;
  }
  ESPDEPRECATED("Pass a columns * rows buffer, e.g. from cg.static_buffer(). Removed in 2027.6.0.", "2026.12.0")
  void set_dimensions(uint8_t columns, uint8_t rows) {
    this->set_dimensions(columns, rows, new uint8_t[columns * rows]);  // NOLINT
  }

  void set_user_defined_char(uint8_t pos, const std::vector<uint8_t> &data) { this->user_defined_chars_[pos] = data; }

  void setup() override;
  float get_setup_priority() const override;
  void update() override;
  void display();
  //// Clear LCD display
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

  void show_page(LCDDisplayPage *page);
  void show_next_page();
  void show_prev_page();

  void set_pages(std::initializer_list<LCDDisplayPage *> pages);
  void set_auto_cycle_interval(uint32_t auto_cycle_interval) { this->auto_cycle_interval_ = auto_cycle_interval; }

  const LCDDisplayPage *get_active_page() const { return this->page_; }

 protected:
  virtual bool is_four_bit_mode() = 0;
  virtual void write_n_bits(uint8_t value, uint8_t n) = 0;
  virtual void send(uint8_t value, bool rs) = 0;

  void setup_big_font_();
  void command_(uint8_t value);
  virtual void call_writer() = 0;

  uint8_t columns_;
  uint8_t rows_;
  uint8_t *buffer_{nullptr};
  std::map<uint8_t, std::vector<uint8_t> > user_defined_chars_;
  LCDDisplayPage *page_{nullptr};
  uint32_t auto_cycle_interval_{0};
  bool big_font_available_{true};
};

class LCDDisplayPage final {
 public:
  explicit LCDDisplayPage(display_writer_t writer);
  void show();
  void show_next();
  void show_prev();
  void set_parent(LCDDisplay *parent);
  void set_prev(LCDDisplayPage *prev);
  void set_next(LCDDisplayPage *next);
  const display_writer_t &get_writer() const;

 protected:
  LCDDisplay *parent_{nullptr};
  display_writer_t writer_;
  LCDDisplayPage *prev_{nullptr};
  LCDDisplayPage *next_{nullptr};
};

template<typename... Ts> class LCDDisplayPageShowAction final : public Action<Ts...> {
 public:
  TEMPLATABLE_VALUE(LCDDisplayPage *, page)

  void play(Ts... x) override {
    auto *page = this->page_.value(x...);
    if (page != nullptr) {
      page->show();
    }
  }
};

}  // namespace esphome::lcd_base
