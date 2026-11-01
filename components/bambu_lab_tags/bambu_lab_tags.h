#pragma once

#include <utility>

#include "spool.h"
#include "esphome/components/mifare_classic/mifare_classic.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome::bambu_lab_tags {

class BambuLabTags : public Component, public mifare_classic::MifareClassicListener {
 public:
  void dump_config() override;
  void on_mifare_classic(mifare_classic::MifareClassicReader &reader, const nfc::NfcTagUid &uid) override;
  template<typename F>
  void add_on_tag_callback(F &&callback) {
    this->callbacks_.add(std::forward<F>(callback));
  }

 protected:
  LazyCallbackManager<void(const Spool &)> callbacks_;
};

}  // namespace esphome::bambu_lab_tags
