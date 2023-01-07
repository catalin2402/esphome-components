#pragma once

#define ARDUINOJSON_USE_LONG_LONG 1 

#include "ArduinoJson.h" 
#include "esphome/components/uart/uart.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace esp_player {

class EspPlayer : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void set_current_song_sensor(text_sensor::TextSensor *current_song_sensor) { current_song_sensor_ = current_song_sensor; }
  void set_play_state_sensor(binary_sensor::BinarySensor *play_state_sensor) { play_state_sensor_ = play_state_sensor; }
  void play_pause();

 protected:
  void process_message();
  void requestInitialData();
  void sendData(std::string);
  text_sensor::TextSensor *current_song_sensor_{nullptr};
  binary_sensor::BinarySensor *play_state_sensor_{nullptr};
  uint8_t rx_message_[256];
  int pos_ = 0;
  bool is_playing_ = false;
};

}  // namespace esp_player
}  // namespace esphome
