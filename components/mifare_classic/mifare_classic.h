#pragma once

#include <array>

#include "esphome/components/nfc/nfc_tag.h"

namespace esphome::mifare_classic {
class MifareClassicReader;
class MifareClassicListener {
 public:
  virtual void on_mifare_classic(MifareClassicReader &reader, const nfc::NfcTagUid &uid) = 0;
};
// Operations are valid only during on_mifare_classic, while the target is
// selected.
class MifareClassicReader {
 public:
  void set_mifare_classic_listener(MifareClassicListener *listener) { this->mifare_listener_ = listener; }
  virtual bool authenticate_mifare_classic(const nfc::NfcTagUid &uid, uint8_t block,
                                           const std::array<uint8_t, 6> &key) = 0;
  virtual bool read_mifare_classic(uint8_t block, std::array<uint8_t, 16> &data) = 0;

 protected:
  MifareClassicListener *mifare_listener_{nullptr};
};
}  // namespace esphome::mifare_classic
