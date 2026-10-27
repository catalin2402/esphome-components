#pragma once

#include "usb_audio_device.h"
#include "esphome/core/automation.h"

namespace esphome::usb_audio_device {

class ConnectedTrigger : public Trigger<> {
 public:
  explicit ConnectedTrigger(USBAudioDevice *parent) {
    parent->add_connection_callback([this](bool active) {
      if (active == true)
        this->trigger();
    });
  }
};

class DisconnectedTrigger : public Trigger<> {
 public:
  explicit DisconnectedTrigger(USBAudioDevice *parent) {
    parent->add_connection_callback([this](bool active) {
      if (active == false)
        this->trigger();
    });
  }
};

class PlaybackStartedTrigger : public Trigger<> {
 public:
  explicit PlaybackStartedTrigger(USBAudioDevice *parent) {
    parent->add_playback_callback([this](bool active) {
      if (active == true)
        this->trigger();
    });
  }
};

class PlaybackStoppedTrigger : public Trigger<> {
 public:
  explicit PlaybackStoppedTrigger(USBAudioDevice *parent) {
    parent->add_playback_callback([this](bool active) {
      if (active == false)
        this->trigger();
    });
  }
};

class CaptureStartedTrigger : public Trigger<> {
 public:
  explicit CaptureStartedTrigger(USBAudioDevice *parent) {
    parent->add_capture_callback([this](bool active) {
      if (active == true)
        this->trigger();
    });
  }
};

class CaptureStoppedTrigger : public Trigger<> {
 public:
  explicit CaptureStoppedTrigger(USBAudioDevice *parent) {
    parent->add_capture_callback([this](bool active) {
      if (active == false)
        this->trigger();
    });
  }
};

class PlaybackActivityTrigger : public Trigger<bool> {
 public:
  explicit PlaybackActivityTrigger(USBAudioDevice *parent) {
    parent->add_playback_activity_callback([this](bool active) { this->trigger(active); });
  }
};

class MicrophoneMuteTrigger : public Trigger<bool> {
 public:
  explicit MicrophoneMuteTrigger(USBAudioDevice *parent) {
    parent->add_microphone_mute_callback([this](bool active) { this->trigger(active); });
  }
};

}  // namespace esphome::usb_audio_device
