#pragma once
#include "esphome/components/a2dp_sink/a2dp_sink.h"
#include "esphome/components/switch/switch.h"

namespace esphome::a2dp_sink {
class BluetoothSwitch : public Component, public switch_::Switch {
 public:
  void set_parent(A2DPSink *parent) { this->parent_ = parent; }
  float get_setup_priority() const override { return setup_priority::DATA - 1; }
  void setup() override {
    this->write_state(this->get_initial_state_with_restore_mode().value_or(false));
    this->disable_loop();
  }
 protected:
  void write_state(bool state) override {
    this->parent_->set_enabled(state);
    this->publish_state(state && !this->parent_->is_failed());
  }
  A2DPSink *parent_;
};
}  // namespace esphome::a2dp_sink
