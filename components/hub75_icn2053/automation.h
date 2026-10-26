#pragma once

#include "hub75_icn2053.h"
#include "esphome/core/automation.h"

namespace esphome::hub75_icn2053 {

template<typename... Ts> class SetBrightnessAction : public Action<Ts...> {
 public:
  explicit SetBrightnessAction(HUB75Display *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, brightness)
  void play(Ts... x) override { this->parent_->set_brightness(this->brightness_.value(x...)); }

 protected:
  HUB75Display *parent_;
};

template<typename... Ts> class IsReadyCondition : public Condition<Ts...> {
 public:
  explicit IsReadyCondition(HUB75Display *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_display_ready(); }

 protected:
  HUB75Display *parent_;
};

template<typename... Ts> class BrightnessInRangeCondition : public Condition<Ts...> {
 public:
  BrightnessInRangeCondition(HUB75Display *parent, uint8_t min, uint8_t max) : parent_(parent), min_(min), max_(max) {}
  bool check(Ts... x) override {
    const auto brightness = this->parent_->get_brightness();
    return brightness >= this->min_ && brightness <= this->max_;
  }

 protected:
  HUB75Display *parent_;
  uint8_t min_;
  uint8_t max_;
};

class BrightnessTrigger : public Trigger<int> {
 public:
  explicit BrightnessTrigger(HUB75Display *parent) {
    parent->add_on_brightness_callback([this](int brightness) { this->trigger(brightness); });
  }
};

class ReadyTrigger : public Trigger<> {
 public:
  explicit ReadyTrigger(HUB75Display *parent) {
    parent->add_on_ready_callback([this]() { this->trigger(); });
  }
};

class ErrorTrigger : public Trigger<> {
 public:
  explicit ErrorTrigger(HUB75Display *parent) {
    parent->add_on_error_callback([this]() { this->trigger(); });
  }
};

}  // namespace esphome::hub75_icn2053
