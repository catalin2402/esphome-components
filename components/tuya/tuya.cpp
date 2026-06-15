#include "tuya.h"
#include "esphome/components/network/util.h"
#include "esphome/core/gpio.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"

#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif

#ifdef USE_CAPTIVE_PORTAL
#include "esphome/components/captive_portal/captive_portal.h"
#endif

namespace esphome::tuya {

static const char *const TAG = "tuya";
static constexpr uint8_t TUYA_MODULE_FRAME_VERSION = 0x00;
static constexpr uint8_t TUYA_PROTOCOL_VERSION_3 = 0x03;
static constexpr uint32_t INITIAL_HEARTBEAT_INTERVAL_MS = 1000;
static constexpr uint32_t NORMAL_HEARTBEAT_INTERVAL_MS = 15000;
static constexpr uint32_t DATAPOINT_SYNC_TIMEOUT_MS = 3000;
static constexpr uint32_t CONF_QUERY_DEFER_MS = 10;
static constexpr uint32_t COMMAND_DELAY_MS = 10;
static constexpr uint32_t RECEIVE_TIMEOUT_MS = 300;
static constexpr uint8_t MAX_COMMAND_RETRIES = 5;
// Max bytes to log for datapoint values (larger values are truncated)
static constexpr size_t MAX_DATAPOINT_LOG_BYTES = 16;

static const char *network_mode_to_string(TuyaNetworkMode mode) {
  switch (mode) {
    case TuyaNetworkMode::COORDINATED:
      return "coordinated";
    case TuyaNetworkMode::SELF_PROCESSING:
      return "self-processing";
    default:
      return "unknown";
  }
}

static const char *network_status_to_string(TuyaNetworkStatus status) {
  switch (status) {
    case TuyaNetworkStatus::EZ_PAIRING:
      return "EZ pairing";
    case TuyaNetworkStatus::AP_PAIRING:
      return "AP pairing";
    case TuyaNetworkStatus::CONFIGURED_NOT_CONNECTED:
      return "configured, router disconnected";
    case TuyaNetworkStatus::ROUTER_CONNECTED:
      return "router connected";
    case TuyaNetworkStatus::CLOUD_CONNECTED:
      return "cloud connected";
    case TuyaNetworkStatus::LOW_POWER:
      return "low power";
    case TuyaNetworkStatus::EZ_AND_AP_PAIRING:
      return "EZ and AP pairing";
    default:
      return "unknown";
  }
}

void Tuya::setup() {
  this->heartbeat_interval_ms_ = INITIAL_HEARTBEAT_INTERVAL_MS;
  this->set_interval("heartbeat", INITIAL_HEARTBEAT_INTERVAL_MS, [this] {
    if (!this->heartbeat_enabled_ || !this->command_queue_.empty() || this->expected_response_.has_value()) {
      return;
    }
    if (millis() - this->last_heartbeat_timestamp_ >= this->heartbeat_interval_ms_) {
      this->send_empty_command_(TuyaCommandType::HEARTBEAT);
    }
  });

  // Network status is proactive: report it after MCU initialization and every
  // time the actual status changes. This callback is inert in self-processing
  // mode, where command 0x03 is not used.
  this->set_interval("network_status", 1000, [this] {
    if (this->wifi_status_reporting_enabled_) {
      this->send_wifi_status_();
    }
  });

  if (this->status_pin_ != nullptr) {
    this->status_pin_->digital_write(false);
  }

  // Tuya specifies one-second heartbeats until the MCU replies. Start the first
  // exchange immediately instead of waiting for the first interval tick.
  this->send_empty_command_(TuyaCommandType::HEARTBEAT);
}

void Tuya::loop() {
  // Read all available bytes in batches to reduce UART call overhead.
  size_t avail = this->available();
  uint8_t buf[64];
  while (avail > 0) {
    size_t to_read = std::min(avail, sizeof(buf));
    if (!this->read_array(buf, to_read)) {
      break;
    }
    avail -= to_read;

    for (size_t i = 0; i < to_read; i++) {
      this->handle_char_(buf[i]);
    }
  }
  process_command_queue_();
}

void Tuya::dump_config() {
  ESP_LOGCONFIG(TAG, "Tuya:");
  if (this->protocol_version_.has_value()) {
    ESP_LOGCONFIG(TAG, "  MCU protocol version: 0x%02X", *this->protocol_version_);
  } else {
    ESP_LOGCONFIG(TAG, "  MCU protocol version: unknown");
  }
  ESP_LOGCONFIG(TAG, "  Network mode: %s", network_mode_to_string(this->network_mode_));

  if (this->init_state_ != TuyaInitState::INIT_DONE) {
    if (this->init_failed_) {
      ESP_LOGCONFIG(TAG, "  Initialization failed. Current init_state: %u", static_cast<uint8_t>(this->init_state_));
    } else {
      ESP_LOGCONFIG(TAG, "  Configuration will be reported when setup is complete. Current init_state: %u",
                    static_cast<uint8_t>(this->init_state_));
    }
    ESP_LOGCONFIG(TAG, "  If no further output is received, confirm that this is a supported Tuya device.");
    return;
  }
  for (auto &info : this->datapoints_) {
    if (info.type == TuyaDatapointType::RAW) {
      char hex_buf[format_hex_pretty_size(MAX_DATAPOINT_LOG_BYTES)];
      ESP_LOGCONFIG(TAG, "  Datapoint %u: raw (value: %s)", info.id,
                    format_hex_pretty_to(hex_buf, info.value_raw.data(), info.value_raw.size()));
    } else if (info.type == TuyaDatapointType::BOOLEAN) {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: switch (value: %s)", info.id, ONOFF(info.value_bool));
    } else if (info.type == TuyaDatapointType::INTEGER) {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: int value (value: %d)", info.id, info.value_int);
    } else if (info.type == TuyaDatapointType::STRING) {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: string value (value: %s)", info.id, info.value_string.c_str());
    } else if (info.type == TuyaDatapointType::ENUM) {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: enum (value: %d)", info.id, info.value_enum);
    } else if (info.type == TuyaDatapointType::BITMASK) {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: bitmask (value: %" PRIx32 ")", info.id, info.value_bitmask);
    } else {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: unknown", info.id);
    }
  }
  if ((this->status_pin_reported_ != -1) || (this->reset_pin_reported_ != -1)) {
    ESP_LOGCONFIG(TAG, "  GPIO Configuration: status: pin %d, reset: pin %d", this->status_pin_reported_,
                  this->reset_pin_reported_);
  }
  if (this->bluetooth_status_pin_reported_ != -1) {
    ESP_LOGCONFIG(TAG, "  Bluetooth status pin: %d", this->bluetooth_status_pin_reported_);
  }
  LOG_PIN("  Status Pin: ", this->status_pin_);
  ESP_LOGCONFIG(TAG, "  Product: '%s'", this->product_.c_str());
}

bool Tuya::validate_message_() {
  uint32_t at = this->rx_message_.size() - 1;
  auto *data = &this->rx_message_[0];
  uint8_t new_byte = data[at];

  // Byte 0: HEADER1 (always 0x55)
  if (at == 0)
    return new_byte == 0x55;
  // Byte 1: HEADER2 (always 0xAA)
  if (at == 1)
    return new_byte == 0xAA;

  // Byte 2: VERSION
  // no validation for the following fields:
  uint8_t version = data[2];
  if (at == 2)
    return true;
  // Byte 3: COMMAND
  uint8_t command = data[3];
  if (at == 3)
    return true;

  // Byte 4: LENGTH1
  // Byte 5: LENGTH2
  if (at <= 5) {
    // no validation for these fields
    return true;
  }

  uint16_t length = (uint16_t(data[4]) << 8) | (uint16_t(data[5]));

  // wait until all data is read
  if (at - 6 < length)
    return true;

  // Byte 6+LEN: CHECKSUM - sum of all bytes (including header) modulo 256
  uint8_t rx_checksum = new_byte;
  uint8_t calc_checksum = 0;
  for (uint32_t i = 0; i < 6 + length; i++)
    calc_checksum += data[i];

  if (rx_checksum != calc_checksum) {
    ESP_LOGW(TAG, "Tuya Received invalid message checksum %02X!=%02X", rx_checksum, calc_checksum);
    return false;
  }

  // valid message
  const uint8_t *message_data = data + 6;
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  char hex_buf[format_hex_pretty_size(MAX_DATAPOINT_LOG_BYTES)];
  ESP_LOGV(TAG, "Received Tuya: CMD=0x%02X VERSION=%u DATA=[%s] INIT_STATE=%u", command, version,
           format_hex_pretty_to(hex_buf, message_data, length), static_cast<uint8_t>(this->init_state_));
#endif
  this->handle_command_(command, version, message_data, length);

  // return false to reset rx buffer
  return false;
}

void Tuya::handle_char_(uint8_t c) {
  this->rx_message_.push_back(c);
  if (!this->validate_message_()) {
    this->rx_message_.clear();
  } else {
    this->last_rx_char_timestamp_ = millis();
  }
}

void Tuya::handle_command_(uint8_t command, uint8_t version, const uint8_t *buffer, size_t len) {
  const auto command_type = static_cast<TuyaCommandType>(command);
  this->update_protocol_version_(version);

  if (this->expected_response_.has_value() && *this->expected_response_ == command_type) {
    this->expected_response_.reset();
    if (!this->command_queue_.empty()) {
      this->command_queue_.erase(this->command_queue_.begin());
    }
    this->command_retries_ = 0;
  }

  switch (command_type) {
    case TuyaCommandType::HEARTBEAT: {
      if (len < 1) {
        ESP_LOGW(TAG, "MCU heartbeat response has no status byte");
        break;
      }
      ESP_LOGV(TAG, "MCU Heartbeat (0x%02X)", buffer[0]);
      this->heartbeat_interval_ms_ = NORMAL_HEARTBEAT_INTERVAL_MS;

      if (buffer[0] == 0x00) {
        ESP_LOGI(TAG, "MCU restarted");
        this->start_initialization_();
      } else if (this->init_state_ == TuyaInitState::INIT_HEARTBEAT) {
        // The ESP/network module can restart while the MCU remains powered. A
        // normal 0x01 heartbeat still starts a complete module initialization.
        this->start_initialization_();
      }
      break;
    }

    case TuyaCommandType::PRODUCT_QUERY: {
      bool valid = true;
      for (size_t i = 0; i < len; i++) {
        if (!std::isprint(buffer[i])) {
          valid = false;
          break;
        }
      }
      if (valid) {
        this->product_ = std::string(reinterpret_cast<const char *>(buffer), len);
      } else {
        this->product_ = R"({"p":"INVALID"})";
      }

      if (this->init_state_ == TuyaInitState::INIT_PRODUCT) {
        this->init_state_ = TuyaInitState::INIT_CONF;
        // Protocol v3 allows the MCU to send command 0x37 after the product
        // response and before the working-mode query. Defer 0x02 so an already
        // buffered feature notification can be parsed and acknowledged first.
        this->set_timeout("tuya_conf_query", CONF_QUERY_DEFER_MS,
                          [this] { this->send_empty_command_(TuyaCommandType::CONF_QUERY); });
      }
      break;
    }

    case TuyaCommandType::CONF_QUERY: {
      this->status_pin_reported_ = -1;
      this->reset_pin_reported_ = -1;
      this->bluetooth_status_pin_reported_ = -1;

      if (len == 0) {
        this->network_mode_ = TuyaNetworkMode::COORDINATED;
      } else if (len == 2 || len == 3) {
        this->network_mode_ = TuyaNetworkMode::SELF_PROCESSING;
        this->status_pin_reported_ = buffer[0];
        this->reset_pin_reported_ = buffer[1];
        if (len == 3) {
          this->bluetooth_status_pin_reported_ = buffer[2];
        }
      } else {
        ESP_LOGW(TAG, "Unexpected working-mode response length %zu; assuming coordinated mode", len);
        this->network_mode_ = TuyaNetworkMode::COORDINATED;
      }

      if (this->init_state_ != TuyaInitState::INIT_CONF) {
        break;
      }

      if (this->network_mode_ == TuyaNetworkMode::SELF_PROCESSING) {
        this->wifi_status_reporting_enabled_ = false;
        if (this->status_pin_ != nullptr) {
          if (this->status_pin_reported_ >= 0 && this->status_pin_->get_pin() != this->status_pin_reported_) {
            ESP_LOGW(TAG, "Supplied status_pin does not equal the reported pin %i. Using supplied pin anyway.",
                     this->status_pin_reported_);
          }
          ESP_LOGV(TAG, "Configured status pin %i", this->status_pin_->get_pin());
          this->set_interval("tuya_status_pin", 1000, [this] { this->set_status_pin_(); });
        } else if (this->status_pin_reported_ >= 0) {
          ESP_LOGW(TAG, "MCU reported status_pin %i but no status_pin was configured; running in limited mode.",
                   this->status_pin_reported_);
        }
        this->begin_datapoint_sync_();
      } else {
        // In coordinated mode command 0x03 is mandatory. Invalidate the cached
        // status so the MCU receives the current state even after a warm module
        // restart where the actual network state did not change.
        this->init_state_ = TuyaInitState::INIT_WIFI;
        this->wifi_status_.reset();
        this->wifi_status_reporting_enabled_ = true;
        this->send_wifi_status_();
      }
      break;
    }

    case TuyaCommandType::WIFI_STATE:
      // Command 0x03 is synchronous. The MCU ACK has no payload; after the
      // initial current-status report is acknowledged, query all DP states.
      if (this->init_state_ == TuyaInitState::INIT_WIFI) {
        this->begin_datapoint_sync_();
      }
      break;

    case TuyaCommandType::WIFI_RESET:
    case TuyaCommandType::WIFI_SELECT: {
      if (command_type == TuyaCommandType::WIFI_SELECT && (len != 1 || buffer[0] > 0x01)) {
        ESP_LOGW(TAG, "Invalid WIFI_SELECT payload");
        break;
      }
      this->send_response_(command_type, {});
      this->wifi_status_.reset();
      ESP_LOGW(TAG,
               "%s requested by the Tuya MCU. The request was acknowledged, but ESPHome does not currently "
               "translate Tuya EZ/AP pairing commands into WiFi credential reset actions.",
               command_type == TuyaCommandType::WIFI_SELECT ? "WIFI_SELECT" : "WIFI_RESET");
      break;
    }

    case TuyaCommandType::DATAPOINT_DELIVER:
      // 0x06 is module-to-MCU in the base protocol. No response is required here.
      break;

    case TuyaCommandType::DATAPOINT_REPORT_ASYNC:
    case TuyaCommandType::DATAPOINT_REPORT_SYNC: {
      this->handle_datapoints_(buffer, len);
      if (command_type == TuyaCommandType::DATAPOINT_REPORT_SYNC) {
        this->send_response_(TuyaCommandType::DATAPOINT_REPORT_ACK, {0x01});
      }
      if (this->init_state_ == TuyaInitState::INIT_DATAPOINT) {
        this->complete_initialization_();
      }
      break;
    }

    case TuyaCommandType::DATAPOINT_QUERY:
      break;

    case TuyaCommandType::WIFI_TEST:
      this->send_response_(TuyaCommandType::WIFI_TEST, {0x00, 0x00});
      break;

    case TuyaCommandType::WIFI_RSSI:
      this->send_response_(TuyaCommandType::WIFI_RSSI, {this->get_wifi_rssi_()});
      break;

    case TuyaCommandType::HEARTBEAT_STOP:
      this->send_response_(TuyaCommandType::HEARTBEAT_STOP, {});
      this->heartbeat_enabled_ = false;
      ESP_LOGI(TAG, "MCU requested heartbeat transmission to stop");
      break;

    case TuyaCommandType::LOCAL_TIME_QUERY:
#ifdef USE_TIME
      if (this->time_id_ != nullptr) {
        this->send_local_time_(true);
        if (!this->time_sync_callback_registered_) {
          this->time_id_->add_on_time_sync_callback([this] { this->send_local_time_(); });
          this->time_sync_callback_registered_ = true;
        }
      } else
#endif
      {
        ESP_LOGW(TAG, "LOCAL_TIME_QUERY is not handled because time is not configured");
      }
      break;

    case TuyaCommandType::VACUUM_MAP_UPLOAD:
      this->send_response_(TuyaCommandType::VACUUM_MAP_UPLOAD, {0x01});
      ESP_LOGW(TAG, "Vacuum map upload requested, responding that it is not enabled.");
      break;

    case TuyaCommandType::GET_NETWORK_STATUS: {
      const auto status = this->get_wifi_status_();
      this->send_response_(TuyaCommandType::GET_NETWORK_STATUS, {static_cast<uint8_t>(status)});
      ESP_LOGV(TAG, "Network status requested, reported as 0x%02X (%s)", static_cast<uint8_t>(status),
               network_status_to_string(status));
      break;
    }

    case TuyaCommandType::FEATURE_CONFIGURATION:
      this->handle_feature_configuration_(buffer, len);
      break;

    case TuyaCommandType::EXTENDED_SERVICES: {
      if (len < 1) {
        ESP_LOGW(TAG, "Extended-services command has no subcommand");
        break;
      }
      const auto subcommand = static_cast<TuyaExtendedServicesCommandType>(buffer[0]);
      switch (subcommand) {
        case TuyaExtendedServicesCommandType::RESET_NOTIFICATION:
          this->send_response_(
              TuyaCommandType::EXTENDED_SERVICES,
              {static_cast<uint8_t>(TuyaExtendedServicesCommandType::RESET_NOTIFICATION), 0x00});
          ESP_LOGV(TAG, "Reset status notification enabled");
          break;
        case TuyaExtendedServicesCommandType::MODULE_RESET:
          ESP_LOGE(TAG, "EXTENDED_SERVICES::MODULE_RESET is not handled");
          break;
        case TuyaExtendedServicesCommandType::UPDATE_IN_PROGRESS:
          ESP_LOGE(TAG, "EXTENDED_SERVICES::UPDATE_IN_PROGRESS is not handled");
          break;
        default:
          ESP_LOGE(TAG, "Invalid extended services subcommand (0x%02X) received", buffer[0]);
          break;
      }
      break;
    }

    default:
      ESP_LOGE(TAG, "Invalid command (0x%02X) received", command);
      break;
  }
}

void Tuya::handle_datapoints_(const uint8_t *buffer, size_t len) {
  while (len >= 4) {
    TuyaDatapoint datapoint{};
    datapoint.id = buffer[0];
    datapoint.type = (TuyaDatapointType) buffer[1];
    datapoint.value_uint = 0;

    size_t data_size = (buffer[2] << 8) + buffer[3];
    const uint8_t *data = buffer + 4;
    size_t data_len = len - 4;
    if (data_size > data_len) {
      ESP_LOGW(TAG, "Datapoint %u is truncated and cannot be parsed (%zu > %zu)", datapoint.id, data_size, data_len);
      return;
    }

    datapoint.len = data_size;

    switch (datapoint.type) {
      case TuyaDatapointType::RAW:
        datapoint.value_raw = std::vector<uint8_t>(data, data + data_size);
        {
          char hex_buf[format_hex_pretty_size(MAX_DATAPOINT_LOG_BYTES)];
          ESP_LOGD(TAG, "Datapoint %u update to %s", datapoint.id,
                   format_hex_pretty_to(hex_buf, datapoint.value_raw.data(), datapoint.value_raw.size()));
        }
        break;
      case TuyaDatapointType::BOOLEAN:
        if (data_size != 1) {
          ESP_LOGW(TAG, "Datapoint %u has bad boolean len %zu", datapoint.id, data_size);
          return;
        }
        datapoint.value_bool = data[0];
        ESP_LOGD(TAG, "Datapoint %u update to %s", datapoint.id, ONOFF(datapoint.value_bool));
        break;
      case TuyaDatapointType::INTEGER:
        if (data_size != 4) {
          ESP_LOGW(TAG, "Datapoint %u has bad integer len %zu", datapoint.id, data_size);
          return;
        }
        datapoint.value_uint = encode_uint32(data[0], data[1], data[2], data[3]);
        ESP_LOGD(TAG, "Datapoint %u update to %d", datapoint.id, datapoint.value_int);
        break;
      case TuyaDatapointType::STRING:
        datapoint.value_string = std::string(reinterpret_cast<const char *>(data), data_size);
        ESP_LOGD(TAG, "Datapoint %u update to %s", datapoint.id, datapoint.value_string.c_str());
        break;
      case TuyaDatapointType::ENUM:
        if (data_size != 1) {
          ESP_LOGW(TAG, "Datapoint %u has bad enum len %zu", datapoint.id, data_size);
          return;
        }
        datapoint.value_enum = data[0];
        ESP_LOGD(TAG, "Datapoint %u update to %d", datapoint.id, datapoint.value_enum);
        break;
      case TuyaDatapointType::BITMASK:
        switch (data_size) {
          case 1:
            datapoint.value_bitmask = encode_uint32(0, 0, 0, data[0]);
            break;
          case 2:
            datapoint.value_bitmask = encode_uint32(0, 0, data[0], data[1]);
            break;
          case 4:
            datapoint.value_bitmask = encode_uint32(data[0], data[1], data[2], data[3]);
            break;
          default:
            ESP_LOGW(TAG, "Datapoint %u has bad bitmask len %zu", datapoint.id, data_size);
            return;
        }
        ESP_LOGD(TAG, "Datapoint %u update to %#08" PRIX32, datapoint.id, datapoint.value_bitmask);
        break;
      default:
        ESP_LOGW(TAG, "Datapoint %u has unknown type %#02hhX", datapoint.id, static_cast<uint8_t>(datapoint.type));
        return;
    }

    len -= data_size + 4;
    buffer = data + data_size;

    // drop update if datapoint is in ignore_mcu_datapoint_update list
    bool skip = false;
    for (auto i : this->ignore_mcu_update_on_datapoints_) {
      if (datapoint.id == i) {
        ESP_LOGV(TAG, "Datapoint %u found in ignore_mcu_update_on_datapoints list, dropping MCU update", datapoint.id);
        skip = true;
        break;
      }
    }
    if (skip)
      continue;

    // Update internal datapoints
    bool found = false;
    for (auto &other : this->datapoints_) {
      if (other.id == datapoint.id) {
        other = datapoint;
        found = true;
      }
    }
    if (!found) {
      this->datapoints_.push_back(datapoint);
    }

    // Run through listeners
    for (auto &listener : this->listeners_) {
      if (listener.datapoint_id == datapoint.id)
        listener.on_datapoint(datapoint);
    }
  }
}

void Tuya::handle_feature_configuration_(const uint8_t *buffer, size_t len) {
  uint8_t result = 0x01;
  uint8_t subcommand = 0x00;

  if (len >= 1) {
    subcommand = buffer[0];
    if (subcommand == static_cast<uint8_t>(TuyaFeatureConfigurationSubcommand::NOTIFY)) {
      result = 0x00;
      if (len > 1) {
        const std::string features(reinterpret_cast<const char *>(buffer + 1), len - 1);
        ESP_LOGD(TAG, "MCU feature configuration: %s", features.c_str());
      } else {
        ESP_LOGD(TAG, "MCU sent an empty feature configuration");
      }
    } else {
      ESP_LOGW(TAG, "Unsupported feature-configuration subcommand 0x%02X", subcommand);
    }
  } else {
    ESP_LOGW(TAG, "Feature-configuration command has no subcommand");
  }

  this->send_response_(TuyaCommandType::FEATURE_CONFIGURATION, {subcommand, result});
}

void Tuya::update_protocol_version_(uint8_t version) {
  if (!this->protocol_version_.has_value() || *this->protocol_version_ != version) {
    if (version != TUYA_MODULE_FRAME_VERSION && version != TUYA_PROTOCOL_VERSION_3) {
      ESP_LOGW(TAG, "MCU uses unrecognized Tuya protocol version 0x%02X", version);
    } else {
      ESP_LOGI(TAG, "MCU Tuya protocol version: 0x%02X", version);
    }
    this->protocol_version_ = version;
  }
}

void Tuya::start_initialization_() {
  this->cancel_timeout("tuya_conf_query");
  this->cancel_timeout("tuya_datapoint_sync");
  this->command_queue_.clear();
  this->expected_response_.reset();
  this->command_retries_ = 0;
  this->init_failed_ = false;
  this->network_mode_ = TuyaNetworkMode::UNKNOWN;
  this->wifi_status_reporting_enabled_ = false;
  this->wifi_status_.reset();
  this->init_state_ = TuyaInitState::INIT_PRODUCT;
  this->send_empty_command_(TuyaCommandType::PRODUCT_QUERY);
}

void Tuya::begin_datapoint_sync_() {
  if (this->init_state_ == TuyaInitState::INIT_DATAPOINT || this->init_state_ == TuyaInitState::INIT_DONE) {
    return;
  }

  this->init_state_ = TuyaInitState::INIT_DATAPOINT;
  // Command 0x08 is asynchronous. It is queued once and does not claim 0x07 as
  // a synchronous response, so unrelated commands remain usable while the MCU
  // publishes one or more DP report frames.
  this->send_empty_command_(TuyaCommandType::DATAPOINT_QUERY);
  this->set_timeout("tuya_datapoint_sync", DATAPOINT_SYNC_TIMEOUT_MS, [this] {
    if (this->init_state_ == TuyaInitState::INIT_DATAPOINT) {
      ESP_LOGW(TAG, "MCU did not report an initial datapoint snapshot; continuing initialization");
      this->complete_initialization_();
    }
  });
}

void Tuya::complete_initialization_() {
  if (this->init_state_ == TuyaInitState::INIT_DONE) {
    return;
  }
  this->cancel_timeout("tuya_conf_query");
  this->cancel_timeout("tuya_datapoint_sync");
  this->init_state_ = TuyaInitState::INIT_DONE;
  this->init_failed_ = false;
  this->command_retries_ = 0;
  this->set_timeout("datapoint_dump", 1000, [this] { this->dump_config(); });
  this->initialized_callback_.call();
}

void Tuya::send_raw_command_(TuyaCommand command) {
  const uint8_t len_hi = static_cast<uint8_t>(command.payload.size() >> 8);
  const uint8_t len_lo = static_cast<uint8_t>(command.payload.size() & 0xFF);
  constexpr uint8_t version = TUYA_MODULE_FRAME_VERSION;

  this->last_command_timestamp_ = millis();
  if (command.cmd == TuyaCommandType::HEARTBEAT) {
    this->last_heartbeat_timestamp_ = this->last_command_timestamp_;
  }

  switch (command.cmd) {
    case TuyaCommandType::HEARTBEAT:
    case TuyaCommandType::PRODUCT_QUERY:
    case TuyaCommandType::CONF_QUERY:
    case TuyaCommandType::WIFI_STATE:
      this->expected_response_ = command.cmd;
      break;
    default:
      // Commands 0x06 and 0x08 are asynchronous in protocol v3. In
      // particular, do not retry DP writes merely because the MCU did not
      // publish a changed value within RECEIVE_TIMEOUT_MS.
      break;
  }

#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  char hex_buf[format_hex_pretty_size(MAX_DATAPOINT_LOG_BYTES)];
  ESP_LOGV(TAG, "Sending Tuya: CMD=0x%02X VERSION=%u DATA=[%s] INIT_STATE=%u", static_cast<uint8_t>(command.cmd),
           version, format_hex_pretty_to(hex_buf, command.payload.data(), command.payload.size()),
           static_cast<uint8_t>(this->init_state_));
#endif

  this->write_array({0x55, 0xAA, version, static_cast<uint8_t>(command.cmd), len_hi, len_lo});
  if (!command.payload.empty()) {
    this->write_array(command.payload.data(), command.payload.size());
  }

  uint8_t checksum = 0x55 + 0xAA + version + static_cast<uint8_t>(command.cmd) + len_hi + len_lo;
  for (const auto data : command.payload) {
    checksum += data;
  }
  this->write_byte(checksum);
}

void Tuya::send_response_(TuyaCommandType command, const std::vector<uint8_t> &payload) {
  // Responses to MCU-originated synchronous commands take priority over the
  // normal outbound queue. send_raw_command_ only installs an expected
  // response for module-originated request commands, so the in-flight queue
  // state is preserved.
  this->send_raw_command_(TuyaCommand{.cmd = command, .payload = payload});
}

void Tuya::process_command_queue_() {
  const uint32_t now = millis();
  const uint32_t delay = now - this->last_command_timestamp_;

  if (now - this->last_rx_char_timestamp_ > RECEIVE_TIMEOUT_MS) {
    this->rx_message_.clear();
  }

  if (this->expected_response_.has_value() && delay > RECEIVE_TIMEOUT_MS) {
    const auto timed_out_command = *this->expected_response_;
    this->expected_response_.reset();

    if (timed_out_command == TuyaCommandType::HEARTBEAT) {
      // The protocol specifies one heartbeat per second until a response, not
      // a burst of generic 300 ms command retries.
      if (!this->command_queue_.empty()) {
        this->command_queue_.erase(this->command_queue_.begin());
      }
      this->command_retries_ = 0;
    } else if (++this->command_retries_ >= MAX_COMMAND_RETRIES) {
      ESP_LOGE(TAG, "Tuya command 0x%02X timed out after %u attempts", static_cast<uint8_t>(timed_out_command),
               MAX_COMMAND_RETRIES);
      if (!this->command_queue_.empty()) {
        this->command_queue_.erase(this->command_queue_.begin());
      }
      this->command_retries_ = 0;
      if (this->init_state_ != TuyaInitState::INIT_DONE) {
        this->init_failed_ = true;
        this->init_state_ = TuyaInitState::INIT_HEARTBEAT;
        this->heartbeat_interval_ms_ = INITIAL_HEARTBEAT_INTERVAL_MS;
        this->wifi_status_reporting_enabled_ = false;
        this->wifi_status_.reset();
      }
    }
  }

  if (delay > COMMAND_DELAY_MS && !this->command_queue_.empty() && this->rx_message_.empty() &&
      !this->expected_response_.has_value()) {
    this->send_raw_command_(this->command_queue_.front());
    if (!this->expected_response_.has_value()) {
      this->command_queue_.erase(this->command_queue_.begin());
      this->command_retries_ = 0;
    }
  }
}

void Tuya::send_command_(const TuyaCommand &command) {
  this->command_queue_.push_back(command);
  this->process_command_queue_();
}

void Tuya::send_empty_command_(TuyaCommandType command) {
  this->send_command_(TuyaCommand{.cmd = command, .payload = std::vector<uint8_t>{}});
}

void Tuya::set_status_pin_() {
  const auto status = this->get_wifi_status_();
  const bool on = status == TuyaNetworkStatus::ROUTER_CONNECTED || status == TuyaNetworkStatus::CLOUD_CONNECTED;
  this->status_pin_->digital_write(on);
}

TuyaNetworkStatus Tuya::get_wifi_status_() {
  if (network::is_connected()) {
    if (this->protocol_version_.has_value() && *this->protocol_version_ >= TUYA_PROTOCOL_VERSION_3 &&
        remote_is_connected()) {
      return TuyaNetworkStatus::CLOUD_CONNECTED;
    }
    return TuyaNetworkStatus::ROUTER_CONNECTED;
  }

#ifdef USE_CAPTIVE_PORTAL
  if (captive_portal::global_captive_portal != nullptr && captive_portal::global_captive_portal->is_active()) {
    return TuyaNetworkStatus::AP_PAIRING;
  }
#endif

  return TuyaNetworkStatus::CONFIGURED_NOT_CONNECTED;
}

uint8_t Tuya::get_wifi_rssi_() {
#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr) {
    return static_cast<uint8_t>(wifi::global_wifi_component->wifi_rssi());
  }
#endif
  return 0;
}

void Tuya::send_wifi_status_() {
  if (!this->wifi_status_reporting_enabled_ || this->network_mode_ != TuyaNetworkMode::COORDINATED) {
    return;
  }

  const auto status = this->get_wifi_status_();
  if (this->wifi_status_.has_value() && *this->wifi_status_ == status) {
    return;
  }

  ESP_LOGD(TAG, "Reporting network status 0x%02X (%s)", static_cast<uint8_t>(status),
           network_status_to_string(status));
  this->wifi_status_ = status;
  this->send_command_(TuyaCommand{
      .cmd = TuyaCommandType::WIFI_STATE,
      .payload = std::vector<uint8_t>{static_cast<uint8_t>(status)},
  });
}

#ifdef USE_TIME
void Tuya::send_local_time_(bool immediate) {
  std::vector<uint8_t> payload;
  ESPTime now = this->time_id_->now();
  if (now.is_valid()) {
    uint8_t year = now.year - 2000;
    uint8_t month = now.month;
    uint8_t day_of_month = now.day_of_month;
    uint8_t hour = now.hour;
    uint8_t minute = now.minute;
    uint8_t second = now.second;
    // Tuya days starts from Monday, esphome uses Sunday as day 1
    uint8_t day_of_week = now.day_of_week - 1;
    if (day_of_week == 0) {
      day_of_week = 7;
    }
    ESP_LOGD(TAG, "Sending local time");
    payload = std::vector<uint8_t>{0x01, year, month, day_of_month, hour, minute, second, day_of_week};
  } else {
    // By spec we need to notify MCU that the time was not obtained if this is a response to a query
    ESP_LOGW(TAG, "Sending missing local time");
    payload = std::vector<uint8_t>{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  }
  if (immediate) {
    this->send_response_(TuyaCommandType::LOCAL_TIME_QUERY, payload);
  } else {
    this->send_command_(TuyaCommand{.cmd = TuyaCommandType::LOCAL_TIME_QUERY, .payload = payload});
  }
}
#endif

void Tuya::set_raw_datapoint_value(uint8_t datapoint_id, const std::vector<uint8_t> &value) {
  this->set_raw_datapoint_value_(datapoint_id, value, false);
}

void Tuya::set_boolean_datapoint_value(uint8_t datapoint_id, bool value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::BOOLEAN, value, 1, false);
}

void Tuya::set_integer_datapoint_value(uint8_t datapoint_id, uint32_t value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::INTEGER, value, 4, false);
}

void Tuya::set_string_datapoint_value(uint8_t datapoint_id, const std::string &value) {
  this->set_string_datapoint_value_(datapoint_id, value, false);
}

void Tuya::set_enum_datapoint_value(uint8_t datapoint_id, uint8_t value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::ENUM, value, 1, false);
}

void Tuya::set_bitmask_datapoint_value(uint8_t datapoint_id, uint32_t value, uint8_t length) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::BITMASK, value, length, false);
}

void Tuya::force_set_raw_datapoint_value(uint8_t datapoint_id, const std::vector<uint8_t> &value) {
  this->set_raw_datapoint_value_(datapoint_id, value, true);
}

void Tuya::force_set_boolean_datapoint_value(uint8_t datapoint_id, bool value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::BOOLEAN, value, 1, true);
}

void Tuya::force_set_integer_datapoint_value(uint8_t datapoint_id, uint32_t value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::INTEGER, value, 4, true);
}

void Tuya::force_set_string_datapoint_value(uint8_t datapoint_id, const std::string &value) {
  this->set_string_datapoint_value_(datapoint_id, value, true);
}

void Tuya::force_set_enum_datapoint_value(uint8_t datapoint_id, uint8_t value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::ENUM, value, 1, true);
}

void Tuya::force_set_bitmask_datapoint_value(uint8_t datapoint_id, uint32_t value, uint8_t length) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::BITMASK, value, length, true);
}

optional<TuyaDatapoint> Tuya::get_datapoint_(uint8_t datapoint_id) {
  for (auto &datapoint : this->datapoints_) {
    if (datapoint.id == datapoint_id)
      return datapoint;
  }
  return {};
}

void Tuya::set_numeric_datapoint_value_(uint8_t datapoint_id, TuyaDatapointType datapoint_type, const uint32_t value,
                                        uint8_t length, bool forced) {
  ESP_LOGD(TAG, "Setting datapoint %u to %" PRIu32, datapoint_id, value);
  optional<TuyaDatapoint> datapoint = this->get_datapoint_(datapoint_id);
  if (!datapoint.has_value()) {
    ESP_LOGW(TAG, "Setting unknown datapoint %u", datapoint_id);
  } else if (datapoint->type != datapoint_type) {
    ESP_LOGE(TAG, "Attempt to set datapoint %u with incorrect type", datapoint_id);
    return;
  } else if (!forced && datapoint->value_uint == value) {
    ESP_LOGV(TAG, "Not sending unchanged value");
    return;
  }

  std::vector<uint8_t> data;
  switch (length) {
    case 4:
      data.push_back(value >> 24);
      data.push_back(value >> 16);
      [[fallthrough]];
    case 2:
      data.push_back(value >> 8);
      [[fallthrough]];
    case 1:
      data.push_back(value >> 0);
      break;
    default:
      ESP_LOGE(TAG, "Unexpected datapoint length %u", length);
      return;
  }
  this->send_datapoint_command_(datapoint_id, datapoint_type, data);
}

void Tuya::set_raw_datapoint_value_(uint8_t datapoint_id, const std::vector<uint8_t> &value, bool forced) {
  char hex_buf[format_hex_pretty_size(MAX_DATAPOINT_LOG_BYTES)];
  ESP_LOGD(TAG, "Setting datapoint %u to %s", datapoint_id, format_hex_pretty_to(hex_buf, value.data(), value.size()));
  optional<TuyaDatapoint> datapoint = this->get_datapoint_(datapoint_id);
  if (!datapoint.has_value()) {
    ESP_LOGW(TAG, "Setting unknown datapoint %u", datapoint_id);
  } else if (datapoint->type != TuyaDatapointType::RAW) {
    ESP_LOGE(TAG, "Attempt to set datapoint %u with incorrect type", datapoint_id);
    return;
  } else if (!forced && datapoint->value_raw == value) {
    ESP_LOGV(TAG, "Not sending unchanged value");
    return;
  }
  this->send_datapoint_command_(datapoint_id, TuyaDatapointType::RAW, value);
}

void Tuya::set_string_datapoint_value_(uint8_t datapoint_id, const std::string &value, bool forced) {
  ESP_LOGD(TAG, "Setting datapoint %u to %s", datapoint_id, value.c_str());
  optional<TuyaDatapoint> datapoint = this->get_datapoint_(datapoint_id);
  if (!datapoint.has_value()) {
    ESP_LOGW(TAG, "Setting unknown datapoint %u", datapoint_id);
  } else if (datapoint->type != TuyaDatapointType::STRING) {
    ESP_LOGE(TAG, "Attempt to set datapoint %u with incorrect type", datapoint_id);
    return;
  } else if (!forced && datapoint->value_string == value) {
    ESP_LOGV(TAG, "Not sending unchanged value");
    return;
  }
  std::vector<uint8_t> data;
  for (char const &c : value) {
    data.push_back(c);
  }
  this->send_datapoint_command_(datapoint_id, TuyaDatapointType::STRING, data);
}

void Tuya::send_datapoint_command_(uint8_t datapoint_id, TuyaDatapointType datapoint_type, std::vector<uint8_t> data) {
  std::vector<uint8_t> buffer;
  buffer.push_back(datapoint_id);
  buffer.push_back(static_cast<uint8_t>(datapoint_type));
  buffer.push_back(data.size() >> 8);
  buffer.push_back(data.size() >> 0);
  buffer.insert(buffer.end(), data.begin(), data.end());

  this->send_command_(TuyaCommand{.cmd = TuyaCommandType::DATAPOINT_DELIVER, .payload = buffer});
}

void Tuya::register_listener(uint8_t datapoint_id, const std::function<void(TuyaDatapoint)> &func) {
  auto listener = TuyaDatapointListener{
      .datapoint_id = datapoint_id,
      .on_datapoint = func,
  };
  this->listeners_.push_back(listener);

  // Run through existing datapoints
  for (auto &datapoint : this->datapoints_) {
    if (datapoint.id == datapoint_id)
      func(datapoint);
  }
}

TuyaInitState Tuya::get_init_state() { return this->init_state_; }

}  // namespace esphome::tuya
