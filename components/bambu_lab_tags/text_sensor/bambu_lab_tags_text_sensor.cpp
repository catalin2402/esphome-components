#include "bambu_lab_tags_text_sensor.h"

#include "esphome/core/log.h"

namespace esphome::bambu_lab_tags {

static const char *const TAG = "bambu_lab_tags.text_sensor";

void BambuLabTagsTextSensor::setup() {
  this->parent_->add_on_tag_callback([this](const Spool &spool) { this->publish_spool_(spool); });
}

void BambuLabTagsTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Bambu Lab Tags", this); }

void BambuLabTagsTextSensor::publish_spool_(const Spool &spool) {
  switch (this->type_) {
    case TextSensorType::UID:
      this->publish_state(spool.uid);
      break;
    case TextSensorType::MATERIAL_ID:
      this->publish_state(spool.material_id);
      break;
    case TextSensorType::VARIANT_ID:
      this->publish_state(spool.variant_id);
      break;
    case TextSensorType::FILAMENT_TYPE:
      this->publish_state(spool.filament_type);
      break;
    case TextSensorType::DETAILED_FILAMENT_TYPE:
      this->publish_state(spool.detailed_filament_type);
      break;
    case TextSensorType::COLOR:
      this->publish_state(spool.color_name);
      break;
    case TextSensorType::PRODUCTION_DATE:
      this->publish_state(spool.production_date);
      break;
  }
}

}  // namespace esphome::bambu_lab_tags
