#ifdef USE_ARDUINO

#include "ota_http.h"

namespace esphome {
namespace ota_http {

static const char *const TAG = "ota_http";

void OtaHttpComponent::dump_config() { ESP_LOGCONFIG(TAG, "OTA_http:"); }

void OtaHttpComponent::setup() {
  ESPhttpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  ESPhttpUpdate.rebootOnUpdate(false);

  ESPhttpUpdate.onStart([]() { ESP_LOGD(TAG, "Update started"); });
  ESPhttpUpdate.onProgress([](int cur, int total) {
    ESP_LOGD(TAG, "Update process at %.2f%%", (float)cur * 100 / (float)total);
  });

  ESPhttpUpdate.onEnd([]() {
    ESP_LOGD(TAG, "Update finished, restarting");
    esphome::App.safe_reboot();
  });

  ESPhttpUpdate.onError([](int err) {
    ESP_LOGD(TAG, "Update Error(% d): % s\n ", ESPhttpUpdate.getLastError(),
             ESPhttpUpdate.getLastErrorString().c_str());
  });
}

void OtaHttpComponent::set_url(std::string url) {
  this->url_ = url;
  this->secure_ = this->url_.compare(0, 6, "https:") == 0;
}

WiFiClient *OtaHttpComponent::get_client() {
  if (this->secure_) {
    BearSSL::WiFiClientSecure *client = new BearSSL::WiFiClientSecure();
    client->setInsecure();
    client->setBufferSizes(512, 512);
    return client;
  } else {
    WiFiClient *client = new WiFiClient();
    return client;
  }
}

void OtaHttpComponent::flash() {
  ESP_LOGD(TAG, "Fetching update from: %s", this->url_.c_str());
  ESPhttpUpdate.update(*this->get_client(), this->url_.c_str());
}

} // namespace ota_http
} // namespace esphome

#endif // USE_ARDUINO