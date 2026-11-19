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
  HUB_PUBLISHER(tag, const Spool &)
  HUB_PUBLISHER(dump, const char *)
#endif
  void on_mifare_classic(mifare_classic::MifareClassicReader &reader, const nfc::NfcTagUid &uid) override;
  template<typename F> void add_on_tag_callback(F &&callback) { this->callbacks_.add(std::forward<F>(callback)); }

 protected:
  Spool spool_;
  LazyCallbackManager<void(const Spool &)> callbacks_;
};

}  // namespace esphome::bambu_lab_tags
