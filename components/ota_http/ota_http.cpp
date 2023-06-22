#ifdef USE_ARDUINO

#include "ota_http.h"
#include "esphome/components/md5/md5.h"
#include "esphome/components/network/util.h"
#include "esphome/components/ota/ota_backend.h"
#include "esphome/components/ota/ota_backend_arduino_esp32.h"
#include "esphome/components/ota/ota_backend_arduino_esp8266.h"
#include "esphome/components/ota/ota_backend_arduino_rp2040.h"
#include "esphome/components/ota/ota_backend_esp_idf.h"
#include "esphome/core/application.h"
#include "esphome/core/defines.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ota_http {

static const char *const TAG = "ota_http";

void OtaHttpComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "OTA_http:");
  ESP_LOGCONFIG(TAG, "  Timeout: %ums", this->timeout_);
}

void OtaHttpComponent::set_url(std::string url) {
  this->url_ = std::move(url);
  this->secure_ = this->url_.compare(0, 6, "https:") == 0;
  this->client_.setReuse(true);
}

std::unique_ptr<ota::OTABackend> make_ota_backend() {
#ifdef USE_ESP8266
  ESP_LOGD(TAG, "Using ArduinoESP8266OTABackend");
  return make_unique<ota::ArduinoESP8266OTABackend>();
#endif // USE_ESP8266
#ifdef USE_ESP32
  ESP_LOGD(TAG, "Using ArduinoESP32OTABackend");
  return make_unique<ota::ArduinoESP32OTABackend>();
#endif // USE_ESP32
#ifdef USE_ESP_IDF
  ESP_LOGD(TAG, "Using IDFOTABackend");
  return make_unique<ota::IDFOTABackend>();
#endif // USE_ESP_IDF
#ifdef USE_RP2040
  ESP_LOGD(TAG, "Using ArduinoRP2040OTABackend");
  return make_unique<ota::ArduinoRP2040OTABackend>();
#endif // USE_RP2040
  esphome::ESP_LOGE(TAG, "No OTA backend!");
}

bool OtaHttpComponent::http_connect() {
  if (!network::is_connected()) {
    this->client_.end();
    ESP_LOGW(TAG, "OTA Request failed; Not connected to network");
    return false;
  }

  const String url = this->url_.c_str();
  ESP_LOGD(TAG, "Trying to connect to %s", url_.c_str());

  bool begin_status = false;
#if defined(USE_ESP32)
  begin_status = this->client_.begin(url_);
#elif defined(USE_ESP8266)
  // ESP8266 code not tested!
  begin_status = this->client_.begin(*this->get_wifi_client_(), url);
#endif

  if (!begin_status) {
    ESP_LOGW(TAG, "Unable to make http connection");
  }

  return begin_status;
}

#ifdef USE_ESP8266
std::shared_ptr<WiFiClient> OtaHttpComponent::get_wifi_client_() {
#ifdef USE_HTTP_REQUEST_ESP8266_HTTPS
  if (this->secure_) {
    if (this->wifi_client_secure_ == nullptr) {
      this->wifi_client_secure_ = std::make_shared<BearSSL::WiFiClientSecure>();
      this->wifi_client_secure_->setInsecure();
      this->wifi_client_secure_->setBufferSizes(512, 512);
    }
    return this->wifi_client_secure_;
  }
#endif

  if (this->wifi_client_ == nullptr) {
    this->wifi_client_ = std::make_shared<WiFiClient>();
  }
  return this->wifi_client_;
}
#endif

struct Header {
  const char *name;
  const char *value;
};

void OtaHttpComponent::flash() {
  uint32_t update_start_time = millis();
  uint32_t start_time;
  uint32_t duration;
  int http_code;
  const char *headerKeys[] = {"Content-Length", "Content-Type"};
  const size_t headerCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
  const size_t chunk_size = 1024; // HTTP_TCP_BUFFER_SIZE;
  size_t chunk_start = 0;
  size_t chunk_stop = chunk_size;
  size_t chunk_end;
  uint8_t buf[chunk_size + 1];
  char str_range[40];
  bool update_started = false;
  int error_code = 0;
  uint32_t last_progress = 0;
  size_t body_lenght;
  size_t bytes_read = 0;
  esphome::md5::MD5Digest md5_receive;
  char *md5_receive_str = new char[33];
  WiFiClient stream;
  std::unique_ptr<ota::OTABackend> backend;

  if (!network::is_connected()) {
    ESP_LOGW(TAG, "Not connected to network");
    return;
  }

  // connect to http server using this->url_
  if (!http_connect()) {
    this->client_.end();
    ESP_LOGW(TAG, "HTTP Request failed at the begin phase. Please check the "
                  "configuration");
    return;
  }

  // we will compute md5 on the fly
  // TODO: better security if fetched from the http server
  md5_receive.init();

  // returned needed headers must be collected before the requests
  this->client_.collectHeaders(headerKeys, headerCount);

  // http GET chunk
  start_time = millis();
  http_code = client_.GET();
  duration = millis() - start_time;

  if (http_code >= 310) {
    ESP_LOGW(TAG,
             "HTTP Request failed; URL: %s; Error: %s (%d); Duration: %u ms",
             url_.c_str(), HTTPClient::errorToString(http_code).c_str(),
             http_code, duration);
    return;
  }

  body_lenght = this->client_.getSize();

  // flash memory backend
  backend = make_ota_backend();
  error_code = backend->begin(body_lenght);
  if (error_code != 0) {
    ESP_LOGW(TAG, "backend->begin error: %d", error_code);
    goto error; // NOLINT(cppcoreguidelines-avoid-goto)
  }

  // the data is read chunked
  stream = this->client_.getStream();

  while (bytes_read != body_lenght) {
    size_t bufsize = std::min(chunk_size, body_lenght - bytes_read);

    // ESP_LOGD(TAG, "going to read %d/%d", bufsize, body_lenght);
    //  binary read

    while (stream.available() < bufsize) {
      // give other tasks a chance to run while waiting for some data:
      yield();
      delay(1);
    }
    stream.readBytes(buf, bufsize);
    if (bytes_read == 0 and buf[0] != 0xE9) {
      // check magic byte
      ESP_LOGE(TAG, "Firmware magic byte 0xE9 a pos 0 failed! OTA aborted");
      return;
    }
    bytes_read += bufsize;
    buf[bufsize] = '\0'; // not fed to ota
    // ESP_LOGD(TAG, "buf: -%s- read %d/%d", (char *)buf, bytes_read,
    // body_lenght); // string only! ESP_LOGD(TAG, "read %d/%d", bytes_read,
    // body_lenght);

    md5_receive.add(buf, bufsize);

    update_started = true;
    error_code = backend->write(buf, bufsize);
    if (error_code != 0) {
      esphome::ESP_LOGW(TAG,
                        "Error code (%d) writing binary data to flash at "
                        "offset %d and size %s",
                        error_code, chunk_start, body_lenght);
      goto error; // NOLINT(cppcoreguidelines-avoid-goto)
    }

    // show progress, and feed watch dog
    uint32_t now = millis();
    if ((now - last_progress > 1000) or (bytes_read == body_lenght)) {
      last_progress = now;
      esphome::ESP_LOGD(TAG, "Progress: %0.1f%%",
                        bytes_read * 100. / body_lenght);

      // feed watchdog and give other tasks a chance to run
      esphome::App.feed_wdt();
      yield();
    }

  } // while

  esphome::ESP_LOGD(TAG, "Done in %.0f secs",
                    (millis() - update_start_time) / 1000);

  // send md5 to backend (backend will check that the flashed one has the same)
  md5_receive.calculate();
  md5_receive.get_hex(md5_receive_str);
  esphome::ESP_LOGD(TAG, "md5sum recieved: %s (size %d)", md5_receive_str,
                    bytes_read);
  backend->set_update_md5(md5_receive_str);

  this->client_.end();

  delete[] md5_receive_str;

  // feed watchdog and give other tasks a chance to run
  esphome::App.feed_wdt();
  yield();
  delay(100);

  error_code = backend->end();
  if (error_code != 0) {
    esphome::ESP_LOGW(TAG, "Error ending OTA!, error_code: %d", error_code);
    goto error; // NOLINT(cppcoreguidelines-avoid-goto)
  }

  delay(10);
  esphome::ESP_LOGI(TAG, "OTA update finished! Rebooting...");
  delay(100); // NOLINT
  esphome::App.safe_reboot();
  // new firmware flashed!

error:
  if (update_started) {
    backend->abort();
  }
}

} // namespace ota_http
} // namespace esphome

#endif // USE_ARDUINO