#pragma once

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/component.h"

#include <cinttypes>

namespace esphome {
namespace remote_receiver {

struct RemoteReceiverComponentStore {
  static void gpio_intr(RemoteReceiverComponentStore *arg);

  /// Stores the time (in micros) that the leading/falling edge happened at
  ///  * An even index means a falling edge appeared at the time stored at the index
  ///  * An uneven index means a rising edge appeared at the time stored at the index
  volatile uint32_t *buffer{nullptr};
  /// The position last written to
  volatile uint32_t buffer_write_at;
  /// The position last read from
  uint32_t buffer_read_at{0};
  bool overflow{false};
  uint32_t buffer_size{1000};
  uint32_t filter_us{10};
  ISRInternalGPIOPin pin;
};

class RemoteReceiverComponent : public remote_base::RemoteReceiverBase,
                                public Component

{
 public:
  RemoteReceiverComponent(InternalGPIOPin *pin) : RemoteReceiverBase(pin) {}
  void setup() override;
  void dump_config() override;
  void loop() override;
#if defined(USE_ESP32)
  // receiver ISR setup must run after rmt transmitter setup to allow the same GPIO to be used by both
  // (related: rmt_transmitter setup must run AFTER rmt receiver setup)
  float get_setup_priority() const override { return setup_priority::DATA - 2; }
#endif

  void set_buffer_size(uint32_t buffer_size) { this->buffer_size_ = buffer_size; }
  void set_filter_us(uint32_t filter_us) { this->filter_us_ = filter_us; }
  void set_idle_us(uint32_t idle_us) { this->idle_us_ = idle_us; }
  void set_no_init_pin(bool no_init_pin) { this->no_init_pin_ = no_init_pin; }

 protected:
#if defined(USE_ESP8266) || defined(USE_LIBRETINY) || defined(USE_ESP32)
  RemoteReceiverComponentStore store_;
  HighFrequencyLoopRequester high_freq_;

  uint32_t buffer_size_{};
  uint32_t filter_us_{10};
  uint32_t idle_us_{10000};

  bool no_init_pin_{false};
};
#endif

}  // namespace remote_receiver
}  // namespace esphome