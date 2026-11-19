#pragma once

#include "silvercrest_protocol.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome::silvercrest {

class Silvercrest final : public Component, public remote_base::RemoteTransmittable {
 public:
#ifdef USE_SILVERCREST_DUMP_PUBLISHER
  void set_dump_publisher(void (*publisher)(const char *)) { this->dump_publisher_ = publisher; }
#endif
  void dump_config() override;
  void set_repeat_times(uint8_t repeat_times) { this->repeat_times_ = repeat_times; }
  void control_channel(Channel channel, bool state);
  template<typename F> void add_on_dump_callback(F &&callback) { this->dump_callback_.add(std::forward<F>(callback)); }

 protected:
#ifdef USE_SILVERCREST_DUMP_PUBLISHER
  void (*dump_publisher_)(const char *){nullptr};
#endif
  uint8_t repeat_times_{5};
  LazyCallbackManager<void(const char *)> dump_callback_;
};

}  // namespace esphome::silvercrest
