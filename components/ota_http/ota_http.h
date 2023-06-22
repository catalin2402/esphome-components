#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

#ifdef USE_ESP8266
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#endif

namespace esphome {
namespace ota_http {

class OtaHttpComponent : public Component {
public:
  void dump_config() override;
  void setup() override;
  float get_setup_priority() const override {
    return setup_priority::AFTER_WIFI;
  }
  void set_url(std::string url){ this->url_ = url;};
  void flash();

protected:
  WiFiClient client_;
  std::string url_;
};

} // namespace ota_http
} // namespace esphome

#endif // USE_ARDUINO