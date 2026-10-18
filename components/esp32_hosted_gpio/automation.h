#pragma once

#include "esp32_hosted_gpio.h"
#include "esphome/core/automation.h"

namespace esphome::esp32_hosted_gpio {

template<typename... Ts> class ReconfigureAction : public Action<Ts...> {
 public:
  explicit ReconfigureAction(ESP32HostedGPIOComponent *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->reconfigure(); }

 protected:
  ESP32HostedGPIOComponent *parent_;
};

template<typename... Ts> class IsReadyCondition : public Condition<Ts...> {
 public:
  explicit IsReadyCondition(ESP32HostedGPIOComponent *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_gpio_ready(); }

 protected:
  ESP32HostedGPIOComponent *parent_;
};

template<typename... Ts> class HasPendingCondition : public Condition<Ts...> {
 public:
  explicit HasPendingCondition(ESP32HostedGPIOComponent *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->has_pending(); }

 protected:
  ESP32HostedGPIOComponent *parent_;
};

class ReadyTrigger : public Trigger<> {
 public:
  explicit ReadyTrigger(ESP32HostedGPIOComponent *parent) {
    parent->add_on_ready_callback([this](bool ready) {
      if (ready)
        this->trigger();
    });
  }
};

class NotReadyTrigger : public Trigger<> {
 public:
  explicit NotReadyTrigger(ESP32HostedGPIOComponent *parent) {
    parent->add_on_ready_callback([this](bool ready) {
      if (!ready)
        this->trigger();
    });
  }
};

}  // namespace esphome::esp32_hosted_gpio
