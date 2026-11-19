#pragma once

#include <utility>

#include "spool.h"
#include "esphome/components/mifare_classic/mifare_classic.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome::bambu_lab_tags {

class BambuLabTags final : public Component, public mifare_classic::MifareClassicListener {
 public:
  void setup() override;
  void dump_config() override;
#ifdef USE_BAMBU_LAB_TAGS_TAG_PUBLISHER
  void set_tag_publisher(void (*publisher)(const Spool &)) { this->tag_publisher_ = publisher; }

  void set_dump_publisher(void (*publisher)(const char *)) { this->dump_publisher_ = publisher; }
#endif
  void on_mifare_classic(mifare_classic::MifareClassicReader &reader, const nfc::NfcTagUid &uid) override;
  template<typename F> void add_on_tag_callback(F &&callback) { this->callbacks_.add(std::forward<F>(callback)); }

 protected:
#ifdef USE_BAMBU_LAB_TAGS_TAG_PUBLISHER
  void (*tag_publisher_)(const Spool &){nullptr};
  void (*dump_publisher_)(const char *){nullptr};
#endif
  Spool spool_;
  LazyCallbackManager<void(const Spool &)> callbacks_;
};

}  // namespace esphome::bambu_lab_tags
