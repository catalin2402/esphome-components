#pragma once

#include "esphome/core/automation.h"
#include "esp_player.h"

namespace esphome {
namespace esp_player {


template<typename... Ts> class PlayPauseAction : public Action<Ts...> {
 public:
  explicit PlayPauseAction(EspPlayer *a_esp_player) : esp_player_(a_esp_player) {}

  void play(Ts... x) override { this->esp_player_->play_pause(); }

 protected:
  EspPlayer *esp_player_;
};


}  // namespace esp_player
}  // namespace esphome