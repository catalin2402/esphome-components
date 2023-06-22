#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "ota_http.h"

namespace esphome {
namespace ota_http {
template <typename... Ts> class OtaHttpFlashAction : public Action<Ts...> {
public:
  OtaHttpFlashAction(OtaHttpComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, url)

  void play(Ts... x) override {
    this->parent_->set_url(this->url_.value(x...));
    this->parent_->flash();
  }

protected:
  OtaHttpComponent *parent_;
};

} // namespace ota_http
} // namespace esphome