#include "a2dp_sink.h"
#include <algorithm>
#include <cstring>
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#if __has_include("esp_a2dp_legacy_api.h")
#include "esp_a2dp_legacy_api.h"
#endif

namespace esphome::a2dp_sink {
static const char *const TAG = "a2dp_sink";
using media_source::MediaSourceState;
using media_source::MediaSourceCommand;
A2DPSink *A2DPSink::instance_ = nullptr;

bool A2DPSink::check_(esp_err_t error) {
  if (error == ESP_OK)
    return true;
  ESP_LOGE(TAG, "Bluetooth initialization failed: %s", esp_err_to_name(error));
  this->running_.store(false);
  this->mark_failed();
  return false;
}

void A2DPSink::setup() {
  if (instance_ != nullptr) {
    ESP_LOGE(TAG, "Only one A2DP sink is supported");
    this->mark_failed();
    return;
  }
  instance_ = this;
  this->buffer_ = xRingbufferCreate(this->buffer_size_, RINGBUF_TYPE_NOSPLIT);
  if (this->buffer_ == nullptr) {
    this->mark_failed();
    return;
  }
  this->running_.store(true);
  if (xTaskCreate(audio_task_, "a2dp_output", 4096, this, 5, nullptr) != pdPASS) {
    this->running_.store(false);
    vRingbufferDelete(this->buffer_);
    this->buffer_ = nullptr;
    this->mark_failed();
    return;
  }
#ifdef USE_A2DP_SHARED_BLUETOOTH
  // ESP32BLE brings the shared controller and Bluedroid up from its loop().
  // Wait for it there; never release BLE memory or initialize the stack twice.
  return;
#else
  if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE) {
    ESP_LOGE(TAG, "Bluetooth controller is already owned by another component");
    this->mark_failed();
    this->running_.store(false);
    return;
  }
  if (!this->check_(esp_bt_controller_mem_release(ESP_BT_MODE_BLE)))
    return;
  esp_bt_controller_config_t config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  if (!this->check_(esp_bt_controller_init(&config)) ||
      !this->check_(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) ||
      !this->check_(esp_bluedroid_init()) || !this->check_(esp_bluedroid_enable()))
    return;
  this->initialize_profiles_();
#endif
}

void A2DPSink::initialize_profiles_() {
  if (!this->check_(esp_bt_gap_register_callback(gap_callback_)) ||
      !this->check_(esp_bt_dev_set_device_name(this->name_.c_str())) ||
      !this->check_(esp_avrc_ct_init()) ||
      !this->check_(esp_avrc_ct_register_callback(avrc_callback_)) ||
      !this->check_(esp_a2d_register_callback(a2dp_callback_)) ||
      !this->check_(esp_a2d_sink_register_data_callback(pcm_callback_)) ||
      !this->check_(esp_a2d_sink_init()))
    return;
  esp_bt_io_cap_t capability = ESP_BT_IO_CAP_NONE;
  esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &capability, sizeof(capability));
  this->initialized_ = true;
  this->set_enabled(this->enabled_.load());
}

void A2DPSink::set_enabled(bool enabled) {
  this->enabled_.store(enabled);
  if (!this->initialized_ || this->is_failed())
    return;
  esp_bt_gap_set_scan_mode(enabled ? ESP_BT_CONNECTABLE : ESP_BT_NON_CONNECTABLE,
                           enabled ? ESP_BT_GENERAL_DISCOVERABLE : ESP_BT_NON_DISCOVERABLE);
  if (!enabled) {
    this->selected_.store(false);
    this->generation_.fetch_add(1);
    if (this->connected_.load()) {
      esp_bd_addr_t peer;
      portENTER_CRITICAL(&this->peer_lock_);
      std::memcpy(peer, this->peer_, sizeof(peer));
      portEXIT_CRITICAL(&this->peer_lock_);
      esp_a2d_sink_disconnect(peer);
    }
    this->set_state_(MediaSourceState::IDLE);
  }
}

void A2DPSink::a2dp_callback_(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
  auto *self = instance_;
  if (self == nullptr)
    return;
  switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT: {
      const bool connected = param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED;
      if (connected) {
        portENTER_CRITICAL(&self->peer_lock_);
        std::memcpy(self->peer_, param->conn_stat.remote_bda, sizeof(esp_bd_addr_t));
        portEXIT_CRITICAL(&self->peer_lock_);
      }
      self->connected_.store(connected);
      if (!connected) {
        self->streaming_.store(false);
        self->selected_.store(false);
        self->generation_.fetch_add(1);
      } else if (!self->enabled_.load()) {
        esp_a2d_sink_disconnect(param->conn_stat.remote_bda);
      }
      break;
    }
    case ESP_A2D_AUDIO_STATE_EVT:
      self->streaming_.store(param->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED);
      if (!self->streaming_.load())
        self->generation_.fetch_add(1);
      break;
    case ESP_A2D_AUDIO_CFG_EVT: {
      if (param->audio_cfg.mcc.type != ESP_A2D_MCT_SBC)
        break;
      // The first SBC capability octet is sampling frequency and channel mode.
      const uint8_t octet = reinterpret_cast<const uint8_t *>(&param->audio_cfg.mcc.cie)[0];
      self->sample_rate_.store((octet & 0x40) ? 32000 : (octet & 0x20) ? 44100 :
                               (octet & 0x10) ? 48000 : 16000);
      self->channels_.store((octet & 0x08) ? 1 : 2);
      self->generation_.fetch_add(1);
      break;
    }
    default:
      break;
  }
}

void A2DPSink::gap_callback_(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (event == ESP_BT_GAP_CFM_REQ_EVT)
    esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
  else if (event == ESP_BT_GAP_PIN_REQ_EVT) {
    // Legacy pairing only; modern phones use SSP Just Works.
    esp_bt_pin_code_t pin = {'0', '0', '0', '0'};
    esp_bt_gap_pin_reply(param->pin_req.bda, !param->pin_req.min_16_digit, 4, pin);
  }
}

void A2DPSink::avrc_callback_(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param) {
  if (instance_ != nullptr && event == ESP_AVRC_CT_CONNECTION_STATE_EVT)
    instance_->avrc_connected_.store(param->conn_stat.connected);
}

void A2DPSink::send_key_(uint8_t key) {
  if (!this->avrc_connected_.load())
    return;
  const uint8_t label = this->transaction_++ & 0x0f;
  esp_avrc_ct_send_passthrough_cmd(label, key, ESP_AVRC_PT_CMD_STATE_PRESSED);
  esp_avrc_ct_send_passthrough_cmd(this->transaction_++ & 0x0f, key, ESP_AVRC_PT_CMD_STATE_RELEASED);
}

void A2DPSink::loop() {
#ifdef USE_A2DP_SHARED_BLUETOOTH
  if (!this->initialized_) {
    if (this->ble_parent_->is_active())
      this->initialize_profiles_();
    return;
  }
#endif
  const bool streaming = this->streaming_.load() && this->connected_.load() && this->enabled_.load();
  if (!this->connected_.load()) {
    this->blocked_ = false;
    this->set_state_(MediaSourceState::IDLE);
  }
  if (streaming != this->was_streaming_) {
    this->was_streaming_ = streaming;
    if (streaming && !this->blocked_) {
      if (!this->selected_.load())
        this->request_play_uri_("a2dp://");
      if (this->selected_.load())
        this->set_state_(MediaSourceState::PLAYING);
    } else if (!streaming && this->selected_.load()) {
      this->set_state_(this->connected_.load() ? MediaSourceState::PAUSED : MediaSourceState::IDLE);
    }
  }
  if (millis() - this->last_log_ >= 5000) {
    this->last_log_ = millis();
    const auto dropped = this->dropped_.exchange(0);
    if (dropped)
      ESP_LOGW(TAG, "Audio output fell behind; dropped %u bytes", static_cast<unsigned>(dropped));
  }
}

bool A2DPSink::play_uri(const std::string &uri) {
  if (!this->can_handle(uri) || this->is_failed() || !this->initialized_ || !this->enabled_.load() || !this->connected_.load())
    return false;
  this->blocked_ = false;
  this->selected_.store(true);
  this->set_state_(this->streaming_.load() ? MediaSourceState::PLAYING : MediaSourceState::PAUSED);
  if (!this->streaming_.load())
    this->send_key_(ESP_AVRC_PT_CMD_PLAY);
  return true;
}

void A2DPSink::handle_command(MediaSourceCommand command) {
  switch (command) {
    case MediaSourceCommand::PLAY:
      this->play_uri("a2dp://");
      this->send_key_(ESP_AVRC_PT_CMD_PLAY);
      break;
    case MediaSourceCommand::PAUSE:
      this->send_key_(ESP_AVRC_PT_CMD_PAUSE);
      break;
    case MediaSourceCommand::STOP:
      this->blocked_ = true;
      this->selected_.store(false);
      this->generation_.fetch_add(1);
      this->send_key_(ESP_AVRC_PT_CMD_PAUSE);
      this->set_state_(MediaSourceState::IDLE);
      break;
    case MediaSourceCommand::NEXT: this->send_key_(ESP_AVRC_PT_CMD_FORWARD); break;
    case MediaSourceCommand::PREVIOUS: this->send_key_(ESP_AVRC_PT_CMD_BACKWARD); break;
    default: break;
  }
}

void A2DPSink::pcm_callback_(const uint8_t *data, uint32_t length) {
  auto *self = instance_;
  if (self == nullptr || !self->running_.load() || !self->enabled_.load() || !self->selected_.load())
    return;
  const Packet header{self->generation_.load(), self->sample_rate_.load(), self->channels_.load()};
  while (length != 0) {
    const size_t bytes = std::min<size_t>(length, 1024);
    void *item = nullptr;
    // Never block the Bluetooth task on speaker output or allocate per packet.
    if (xRingbufferSendAcquire(self->buffer_, &item, sizeof(Packet) + bytes, 0) != pdTRUE) {
      self->dropped_.fetch_add(length);
      return;
    }
    std::memcpy(item, &header, sizeof(header));
    std::memcpy(static_cast<uint8_t *>(item) + sizeof(header), data, bytes);
    xRingbufferSendComplete(self->buffer_, item);
    data += bytes;
    length -= bytes;
  }
}

void A2DPSink::audio_task_(void *arg) {
  auto *self = static_cast<A2DPSink *>(arg);
  while (self->running_.load()) {
    size_t size = 0;
    auto *packet = static_cast<Packet *>(xRingbufferReceive(self->buffer_, &size, pdMS_TO_TICKS(20)));
    if (packet == nullptr)
      continue;
    const size_t length = size - sizeof(Packet);
    size_t offset = 0;
    const audio::AudioStreamInfo info(16, packet->channels, packet->rate);
    uint32_t last_progress = millis();
    while (offset < length && self->running_.load() && self->selected_.load() && self->enabled_.load() &&
           self->generation_.load() == packet->generation) {
      const size_t written = self->write_output(reinterpret_cast<uint8_t *>(packet + 1) + offset,
                                               length - offset, 20, info);
      offset += written;
      if (written)
        last_progress = millis();
      else if (millis() - last_progress >= 200)
        break;
      else
        vTaskDelay(1);
    }
    if (offset < length && self->selected_.load() && self->generation_.load() == packet->generation)
      self->dropped_.fetch_add(length - offset);
    vRingbufferReturnItem(self->buffer_, packet);
  }
  vTaskDelete(nullptr);
}

void A2DPSink::on_shutdown() {
  this->set_enabled(false);
  this->running_.store(false);
}

#ifdef USE_A2DP_SHARED_BLUETOOTH
void A2DPSink::ble_before_disabled_event_handler() {
  // Called before ESP32BLE tears down the entire shared stack, including profiles.
  // Keep the output task alive, but invalidate all audio from this connection.
  this->initialized_ = false;
  this->selected_.store(false);
  this->connected_.store(false);
  this->streaming_.store(false);
  this->avrc_connected_.store(false);
  this->generation_.fetch_add(1);
  this->was_streaming_ = false;
  this->blocked_ = false;
  this->set_state_(MediaSourceState::IDLE);
}
#endif

void A2DPSink::dump_config() {
  ESP_LOGCONFIG(TAG, "Native ESP-IDF A2DP sink: %s\n  PCM buffer: %u bytes\n  SBC only",
                this->name_.c_str(), static_cast<unsigned>(this->buffer_size_));
#ifdef USE_A2DP_SHARED_BLUETOOTH
  ESP_LOGCONFIG(TAG, "  Bluetooth controller shared with ESP32 BLE");
#else
  ESP_LOGCONFIG(TAG, "  Classic-only Bluetooth controller");
#endif
}
}  // namespace esphome::a2dp_sink
