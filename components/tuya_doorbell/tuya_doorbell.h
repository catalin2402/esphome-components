#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#ifdef USE_TIME
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/time.h"
#endif

namespace esphome {
namespace tuya_doorbell {

enum class TuyaDatapointType : uint8_t {
  RAW = 0x00,     // variable length
  BOOLEAN = 0x01, // 1 byte (0/1)
  INTEGER = 0x02, // 4 byte
  STRING = 0x03,  // variable length
  ENUM = 0x04,    // 1 byte
  BITMASK = 0x05, // 2 bytes
};

struct TuyaDatapoint {
  uint8_t id;
  TuyaDatapointType type;
  union {
    bool value_bool;
    int value_int;
    uint32_t value_uint;
    uint8_t value_enum;
    uint16_t value_bitmask;
  };
  std::vector<uint8_t> value_raw;
};

struct TuyaDatapointListener {
  uint8_t datapoint_id;
  std::function<void(TuyaDatapoint)> on_datapoint;
};

enum class WifiState : uint8_t {
  DISCONNECTED = 1,
  CONNECTED = 3,
  CONNECTED_DATA = 4,
};

enum class TuyaCommandTypeV3 : uint8_t {
  HEARTBEAT = 0x00,
  PRODUCT_QUERY = 0x01,
  CONF_QUERY = 0x02,
  WIFI_STATE = 0x03,
  WIFI_RESET = 0x04,
  WIFI_SELECT = 0x05,
  DATAPOINT_DELIVER = 0x06,
  DATAPOINT_REPORT = 0x07,
  DATAPOINT_QUERY = 0x08,
  WIFI_TEST = 0x0E,
  LOCAL_TIME_QUERY = 0x1C,
};

enum class TuyaInitState : uint8_t {
  INIT_HEARTBEAT = 0x00,
  INIT_PRODUCT,
  INIT_CONF,
  INIT_WIFI,
  INIT_DATAPOINT,
  INIT_DONE,
};

class TuyaDoorbell : public Component, public uart::UARTDevice {
public:
  uint8_t ringMode_ = 0;
  uint8_t volume_ = 10;
  uint8_t sound_ = 1;
  float get_setup_priority() const override { return setup_priority::LATE; }
  void setup() override;
  void loop() override;
  void dump_config() override;
  void register_listener(uint8_t datapoint_id,
                         const std::function<void(TuyaDatapoint)> &func);
  void set_datapoint_value(TuyaDatapoint datapoint, bool force);
  void setRingMode(uint8_t mode);
  void setVolume(uint8_t volume);
  void setSound(uint8_t sound);
  void ring();
#ifdef USE_TIME
  void set_time_id(time::RealTimeClock *time_id) { this->time_id_ = time_id; }
#endif
protected:
  void handle_char_(uint8_t c);
  void handle_datapoints_(const uint8_t *buffer, size_t len);
  bool validate_message_();
  void updateDoorbell();

  void handle_command_(TuyaCommandTypeV3 command, uint8_t version,
                       const uint8_t *buffer, size_t len);
  void send_command_(uint8_t command, const uint8_t *buffer, uint16_t len);
  void send_command_(TuyaCommandTypeV3 command, const uint8_t *buffer,
                     uint16_t len) {
    this->send_command_((uint8_t)command, buffer, len);
  }
  void send_empty_command_(TuyaCommandTypeV3 command) {
    this->send_command_(command, nullptr, 0);
  }

  TuyaInitState init_state_ = TuyaInitState::INIT_HEARTBEAT;
  WifiState wifi_state_ = WifiState::DISCONNECTED;
  int gpio_status_ = -1;
  int gpio_reset_ = -1;
  std::string product_ = "";
  int version_ = 3;
  bool volInit_ = true;
  bool soundInit_ = true;

  std::vector<TuyaDatapointListener> listeners_;
  std::vector<TuyaDatapoint> datapoints_;
  std::vector<uint8_t> rx_message_;

#ifdef USE_TIME
  void send_local_time_();
  optional<time::RealTimeClock *> time_id_{};
#endif
};

} // namespace tuya_doorbell
} // namespace esphome
