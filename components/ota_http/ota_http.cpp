#ifdef USE_ARDUINO

#include "ota_http.h"

namespace esphome {
namespace ota_http {

static const char *const TAG = "ota_http";

void OtaHttpComponent::dump_config() { ESP_LOGCONFIG(TAG, "OTA_http:"); }

void OtaHttpComponent::setup() {}

void update_started() {
  ESP_LOGD("OTA_HTTP", "CALLBACK:  HTTP update process started");
}

void update_finished() {
  ESP_LOGD("OTA_HTTP", "CALLBACK:  HTTP update process finished");
  esphome::App.safe_reboot();
}

void update_progress(int cur, int total) {
  ESP_LOGD("OTA_HTTP", "CALLBACK:  HTTP update process at %d of %d bytes...\n",
           cur, total);
}

void update_error(int err) {
  ESP_LOGD("OTA_HTTP", "CALLBACK:  HTTP update fatal error code %d\n", err);
  esphome::App.safe_reboot();
}

void OtaHttpComponent::flash() {
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);

  t_httpUpdate_return ret =
      ESPhttpUpdate.update(this->client_, this->url_.c_str());

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    ESP_LOGD("OTA_HTTP", "HTTP_UPDATE_FAILED Error(% d): % s\n ",
             ESPhttpUpdate.getLastError(),
             ESPhttpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    ESP_LOGD("OTA_HTTP", "HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    ESP_LOGD("OTA_HTTP", "HTTP_UPDATE_OK");
    break;
  }
}

} // namespace ota_http
} // namespace esphome

#endif // USE_ARDUINO