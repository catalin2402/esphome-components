#include "bambu_lab_tags.h"

#include <algorithm>

#include "esphome/core/log.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/md.h"

namespace esphome::bambu_lab_tags {

ESPHOME_LOG_TAG(TAG, "bambu_lab_tags");

void BambuLabTags::setup() { this->spool_.reserve(); }

void BambuLabTags::dump_config() {
  ESP_LOGCONFIG(TAG, "Bambu Lab Tags:");
#ifdef USE_BAMBU_LAB_TAGS_TAG_PUBLISHER
  if (this->dump_publisher_ != nullptr)
    this->dump_publisher_(TAG);
#endif
}

void BambuLabTags::on_mifare_classic(mifare_classic::MifareClassicReader &reader, const nfc::NfcTagUid &uid) {
  if (uid.size() != 4)
    return;
  static constexpr uint8_t SALT[] = {0x9a, 0x75, 0x9c, 0xf2, 0xc4, 0xf7, 0xca, 0xff,
                                     0x22, 0x2c, 0xb9, 0x76, 0x9b, 0x41, 0xbc, 0x96};
  static constexpr uint8_t INFO[] = {'R', 'F', 'I', 'D', '-', 'A', 0};
  std::array<uint8_t, 96> keys{};
  const auto *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (md == nullptr ||
      mbedtls_hkdf(md, SALT, sizeof(SALT), uid.data(), uid.size(), INFO, sizeof(INFO), keys.data(), keys.size()) != 0) {
    this->status_set_warning(LOG_STR("Key derivation failed"));
    return;
  }
  std::array<uint8_t, 208> data{};
  // Only read blocks used by the parser. Never read trailers or write the tag.
  static constexpr uint8_t BLOCKS[] = {1, 2, 4, 5, 6, 12};
  uint8_t sector = 255;
  for (auto block : BLOCKS) {
    if (block / 4 != sector) {
      sector = block / 4;
      mifare_classic::MifareClassicKey key;
      std::copy_n(keys.begin() + sector * 6, 6, key.begin());
      if (!reader.authenticate_mifare_classic(uid, block, key)) {
        this->status_set_warning(LOG_STR("Tag authentication failed"));
        return;
      }
    }
    mifare_classic::MifareClassicBlock buffer;
    if (!reader.read_mifare_classic(block, buffer)) {
      this->status_set_warning(LOG_STR("Tag read failed"));
      return;
    }
    std::copy(buffer.begin(), buffer.end(), data.begin() + block * 16);
  }
  auto &spool = this->spool_;
  if (!parse_spool(data.data(), data.size(), spool)) {
    this->status_set_warning(LOG_STR("Unrecognized spool data"));
    return;
  }
  char uid_buffer[format_hex_size(4)];
  spool.uid = format_hex_to(uid_buffer, uid.data(), uid.size());
  this->status_clear_warning();
  ESP_LOGD(TAG, "%s: %s (%s), RGBA %s", spool.uid.c_str(), spool.detailed_filament_type.c_str(),
           spool.material_id.c_str(), spool.color.c_str());
  this->callbacks_.call(spool);
#ifdef USE_BAMBU_LAB_TAGS_TAG_PUBLISHER
  if (this->tag_publisher_ != nullptr)
    this->tag_publisher_(spool);
#endif
}

}  // namespace esphome::bambu_lab_tags
