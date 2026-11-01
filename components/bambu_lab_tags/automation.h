#pragma once

#include "bambu_lab_tags.h"
#include "esphome/core/automation.h"

namespace esphome::bambu_lab_tags {

class SpoolTrigger : public Trigger<const Spool &> {
 public:
  explicit SpoolTrigger(BambuLabTags *parent) {
    parent->add_on_tag_callback([this](const Spool &spool) { this->trigger(spool); });
  }
};

}  // namespace esphome::bambu_lab_tags
