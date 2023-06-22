#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"

#include <memory>
#include <string>
#include <utility>

#ifdef USE_ESP32
#include <HTTPClient.h>
#endif
#ifdef USE_ESP8266
#include <ESP8266HTTPClient.h>
#ifdef USE_HTTP_REQUEST_ESP8266_HTTPS
#include <WiFiClientSecure.h>
#endif
#endif

namespace esphome {
namespace ota_http {

class OtaHttpComponent : public Component {
public:
  void dump_config() override;
  float get_setup_priority() const override {
    return setup_priority::AFTER_WIFI;
  }
  void set_url(std::string url);
  void set_timeout(uint64_t timeout) { this->timeout_ = timeout; }
  bool http_connect();
  void flash();

protected:
  HTTPClient client_{};
  std::string url_;
  bool secure_;
  uint64_t timeout_{1000 * 60 * 10};
#ifdef USE_ESP8266
  std::shared_ptr<WiFiClient> wifi_client_;
#ifdef USE_HTTP_REQUEST_ESP8266_HTTPS
  std::shared_ptr<BearSSL::WiFiClientSecure> wifi_client_secure_;
#endif
  std::shared_ptr<WiFiClient> get_wifi_client_();
#endif
};

} // namespace ota_http
} // namespace esphome

#endif // USE_ARDUINO