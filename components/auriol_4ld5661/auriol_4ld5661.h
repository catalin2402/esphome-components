#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "auriol_protocol.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

namespace esphome::auriol_4ld5661 {

class Auriol4LD5661Base : public remote_base::RemoteReceiverListener, public Component {
 public:
  void set_auriol_id(uint8_t id) {
    this->auriol_id_ = id;
    this->has_auriol_id_ = true;
  }
  void dump_config() override;

 protected:
  optional<AuriolData> decode_packet_(remote_base::RemoteReceiveData data);
  uint8_t auriol_id_{0};
  bool has_auriol_id_{false};
};

#ifdef USE_SENSOR
enum class AuriolSensorType : uint8_t { TEMPERATURE, RAIN, RAIN_TIPS };

class AuriolSensor : public Auriol4LD5661Base, public sensor::Sensor {
 public:
  void set_type(AuriolSensorType type) { this->type_ = type; }
  void set_mm_per_tip(float mm_per_tip) { this->mm_per_tip_ = mm_per_tip; }
  bool on_receive(remote_base::RemoteReceiveData data) override;
  void dump_config() override;

 protected:
  AuriolSensorType type_{AuriolSensorType::TEMPERATURE};
  float mm_per_tip_{0.3f};
};
#endif

}  // namespace esphome::auriol_4ld5661
