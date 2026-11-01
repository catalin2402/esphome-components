#include "bambu_lab_tags_sensor.h"

#include "esphome/core/log.h"

namespace esphome::bambu_lab_tags {

static const char *const TAG = "bambu_lab_tags.sensor";

void BambuLabTagsSensor::setup() {
  this->parent_->add_on_tag_callback([this](const Spool &spool) { this->publish_spool_(spool); });
}

void BambuLabTagsSensor::dump_config() { LOG_SENSOR("", "Bambu Lab Tags", this); }

void BambuLabTagsSensor::publish_spool_(const Spool &spool) {
  switch (this->type_) {
    case SensorType::WEIGHT:
      this->publish_state(spool.weight);
      break;
    case SensorType::DRYING_TEMPERATURE:
      this->publish_state(spool.drying_temperature);
      break;
    case SensorType::DRYING_TIME:
      this->publish_state(spool.drying_time);
      break;
    case SensorType::BED_TEMPERATURE:
      this->publish_state(spool.bed_temperature);
      break;
    case SensorType::MIN_NOZZLE_TEMPERATURE:
      this->publish_state(spool.min_nozzle_temperature);
      break;
    case SensorType::MAX_NOZZLE_TEMPERATURE:
      this->publish_state(spool.max_nozzle_temperature);
      break;
  }
}

}  // namespace esphome::bambu_lab_tags
