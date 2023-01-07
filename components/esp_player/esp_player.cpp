#include "esp_player.h"

namespace esphome {
namespace esp_player {

static const char *TAG = "ESP_PLAYER";

void EspPlayer::setup() {
  if (this->current_song_sensor_ != nullptr) {
    this->current_song_sensor_->publish_state("");
  }
  if (this->play_state_sensor_ != nullptr) {
    this->play_state_sensor_->publish_state(false);
  }

  this->requestInitialData();
}

void EspPlayer::loop() {
  int availableBytes = this->available();
  if (availableBytes > 0) {
    for (int i = 0; i < availableBytes; i++) {
      this->read_byte(&rx_message_[pos_]);
      if (rx_message_[pos_] == '\n') {
        process_message();
        break;
      }
      if (pos_ >= 255) {
        pos_ = 0;
        memset(rx_message_, 0, 255);
        break;
      }
      pos_++;
    }
  }
}

void EspPlayer::process_message() {
  std::string str;
  for (int x = 0; x <= pos_; x++) {
    str += ((char) rx_message_[x]);
  }
  str += '\0';
  pos_ = 0;
  memset(rx_message_, 0, 255);

  ESP_LOGD(TAG, "Message: %s", str.c_str());

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, String(str.c_str()));
  if (error.code() != DeserializationError::Ok) {
    ESP_LOGD(TAG, "Deserialize error: %d", error.code());
    return;
  }

  std::string type = doc["type"].as<std::string>();
  std::string data = doc["data"].as<std::string>();
  ESP_LOGD(TAG, "Type: %s", type.c_str());
  ESP_LOGD(TAG, "Data: %s", data.c_str());

  if (type == "current_song") {
    if (this->current_song_sensor_ != nullptr) {
      this->current_song_sensor_->publish_state(data.c_str());
    }
  } else if (type == "play_state") {
    if (this->play_state_sensor_ != nullptr) {
      this->is_playing_ = (data == "playing");
      this->play_state_sensor_->publish_state(this->is_playing_);
    }
  }
}

void EspPlayer::requestInitialData() {
  std::string command;
  DynamicJsonDocument doc(64);
  doc["command"] = "initial_data";
  serializeJson(doc, command);
  this->sendData(command);
}

void EspPlayer::sendData(std::string data) {
  this->write('\r');
  this->write('\n');
  this->write_str(data.c_str());
  this->write('\r');
  this->write('\n');
}

void EspPlayer::play_pause() {
  std::string command;
  DynamicJsonDocument doc(64);
  doc["command"] = "play_pause";
  doc["data"] = (this->is_playing_) ? "pause" : "play";
  serializeJson(doc, command);
  this->sendData(command);
}

}  // namespace esp_player
}  // namespace esphome
