#pragma once

#include "usb_media_player.h"
#include "esphome/core/automation.h"

namespace esphome::usb_media_player {

template<typename... Ts> class MediaKeyAction : public Action<Ts...> {
 public:
  MediaKeyAction(USBMediaPlayer *parent, MediaKey key) : parent_(parent), key_(key) {}
  void play(Ts... x) override { this->parent_->send_media_key(this->key_); }

 protected:
  USBMediaPlayer *parent_;
  MediaKey key_;
};

template<typename... Ts> class MediaKeyPressedAction : public Action<Ts...> {
 public:
  MediaKeyPressedAction(USBMediaPlayer *parent, MediaKey key) : parent_(parent), key_(key) {}
  void play(Ts... x) override { this->parent_->press_media_key(this->key_); }

 protected:
  USBMediaPlayer *parent_;
  MediaKey key_;
};

template<typename... Ts> class MediaKeyReleasedAction : public Action<Ts...> {
 public:
  MediaKeyReleasedAction(USBMediaPlayer *parent, MediaKey key) : parent_(parent), key_(key) {}
  void play(Ts... x) override { this->parent_->release_media_key(this->key_); }

 protected:
  USBMediaPlayer *parent_;
  MediaKey key_;
};

template<typename... Ts> class ConnectedCondition : public Condition<Ts...> {
 public:
  explicit ConnectedCondition(USBMediaPlayer *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_connected(); }

 protected:
  USBMediaPlayer *parent_;
};

template<typename... Ts> class StreamingCondition : public Condition<Ts...> {
 public:
  explicit StreamingCondition(USBMediaPlayer *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_streaming(); }

 protected:
  USBMediaPlayer *parent_;
};

class ConnectedTrigger : public Trigger<> {
 public:
  explicit ConnectedTrigger(USBMediaPlayer *parent) {
    parent->add_on_connection_callback([this](bool state) {
      if (state)
        this->trigger();
    });
  }
};

class DisconnectedTrigger : public Trigger<> {
 public:
  explicit DisconnectedTrigger(USBMediaPlayer *parent) {
    parent->add_on_connection_callback([this](bool state) {
      if (!state)
        this->trigger();
    });
  }
};

class StreamStartedTrigger : public Trigger<> {
 public:
  explicit StreamStartedTrigger(USBMediaPlayer *parent) {
    parent->add_on_stream_callback([this](bool state) {
      if (state)
        this->trigger();
    });
  }
};

class StreamStoppedTrigger : public Trigger<> {
 public:
  explicit StreamStoppedTrigger(USBMediaPlayer *parent) {
    parent->add_on_stream_callback([this](bool state) {
      if (!state)
        this->trigger();
    });
  }
};

template<typename... Ts> class MicrophoneAction : public Action<Ts...> {
 public:
  MicrophoneAction(USBMediaPlayer *parent, bool enabled) : parent_(parent), enabled_(enabled) {}
  void play(Ts... x) override { this->parent_->set_microphone_enabled(this->enabled_); }

 protected:
  USBMediaPlayer *parent_;
  bool enabled_;
};

template<typename... Ts> class MicrophoneEnabledCondition : public Condition<Ts...> {
 public:
  explicit MicrophoneEnabledCondition(USBMediaPlayer *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_microphone_enabled(); }

 protected:
  USBMediaPlayer *parent_;
};

}  // namespace esphome::usb_media_player
