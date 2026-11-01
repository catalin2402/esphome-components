#pragma once

#include "esphome/components/bambu_lab_tags/bambu_lab_tags.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome::bambu_lab_tags {

enum class TextSensorType : uint8_t {
  UID,
  MATERIAL_ID,
  VARIANT_ID,
  FILAMENT_TYPE,
  DETAILED_FILAMENT_TYPE,
  COLOR,
  PRODUCTION_DATE
};

class BambuLabTagsTextSensor : public text_sensor::TextSensor, public Component, public Parented<BambuLabTags> {
 public:
  void setup() override;
  void dump_config() override;
  void set_type(TextSensorType type) { this->type_ = type; }

 protected:
  void publish_spool_(const Spool &spool);

  TextSensorType type_{TextSensorType::UID};
};

}  // namespace esphome::bambu_lab_tags
