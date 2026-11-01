#pragma once

#include "esphome/components/bambu_lab_tags/bambu_lab_tags.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome::bambu_lab_tags {

enum class SensorType : uint8_t {
  WEIGHT,
  DRYING_TEMPERATURE,
  DRYING_TIME,
  BED_TEMPERATURE,
  MIN_NOZZLE_TEMPERATURE,
  MAX_NOZZLE_TEMPERATURE
};

class BambuLabTagsSensor : public sensor::Sensor, public Component, public Parented<BambuLabTags> {
 public:
  void setup() override;
  void dump_config() override;
  void set_type(SensorType type) { this->type_ = type; }

 protected:
  void publish_spool_(const Spool &spool);

  SensorType type_{SensorType::WEIGHT};
};

}  // namespace esphome::bambu_lab_tags
