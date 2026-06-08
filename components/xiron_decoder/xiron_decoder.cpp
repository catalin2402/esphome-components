#include "xiron_decoder.h"

namespace esphome {
namespace xiron_decoder {

static const char *const TAG = "xiron";

bool XironDecoder::on_receive(remote_base::RemoteReceiveData data) {
  const auto &raw = data.get_raw_data();

  if (raw.size() < 72)
    return false;

  for (size_t start = 0; start + 71 < raw.size(); start++) {
    uint32_t bitstream1 = 0;
    uint16_t bitstream2 = 0;
    bool valid = true;

    for (uint8_t bit_count = 0; bit_count < 36; bit_count++) {
      int32_t mark = raw[start + bit_count * 2];
      int32_t space = raw[start + bit_count * 2 + 1];

      if (mark >= 0 || space <= 0) {
        valid = false;
        break;
      }

      uint32_t mark_us = static_cast<uint32_t>(-mark);
      uint32_t space_us = static_cast<uint32_t>(space);

      bool bit;

      if (mark_us >= 750 && mark_us <= 1250) {
        bit = false;
      } else if (mark_us >= 1750 && mark_us <= 2250) {
        bit = true;
      } else {
        valid = false;
        break;
      }

      if (space_us < 250 || space_us > 800) {
        valid = false;
        break;
      }

      if (bit_count < 24)
        bitstream1 = (bitstream1 << 1) | bit;
      else
        bitstream2 = (bitstream2 << 1) | bit;
    }

    if (!valid)
      continue;

    if (bitstream1 == 0)
      continue;

    if (((bitstream2 >> 8) & 0x0F) != 0x0F)
      continue;

    uint8_t humidity = bitstream2 & 0xFF;
    if (humidity == 0 || humidity > 100)
      continue;

    uint8_t sensor_id = (bitstream1 >> 16) & 0x7F;
    bool battery_low = !((bitstream1 >> 15) & 0x01);
    uint8_t bat0 = (bitstream1 >> 14) & 0x01;
    uint8_t channel = ((bitstream1 >> 12) & 0x03) + 1;

    if (bat0 != 0)
      continue;

    if (channel < 1 || channel > 3)
      continue;

    uint16_t device_id = (static_cast<uint16_t>(sensor_id) << 8) | channel;

    if (xiron_id_ != 0 && device_id != xiron_id_)
      continue;

    int temp_raw = bitstream1 & 0x0FFF;
    float temperature;

    if (temp_raw > 3000) {
      temp_raw = 4096 - temp_raw;
      temperature = -(temp_raw / 10.0f);
    } else {
      temperature = temp_raw / 10.0f;
    }

    if (temperature < -60.0f || temperature > 60.0f)
      continue;

    ESP_LOGD(TAG, "Xiron decoded: id=%04X temperature=%.1f humidity=%u battery_low=%s", device_id, temperature, humidity,
             battery_low ? "true" : "false");

    if (temp_sensor_ != nullptr)
      temp_sensor_->publish_state(temperature);

    if (hum_sensor_ != nullptr)
      hum_sensor_->publish_state(humidity);

    if (battery_sensor_ != nullptr)
      battery_sensor_->publish_state(battery_low);

    return true;
  }

  return false;
}

}  // namespace xiron_decoder
}  // namespace esphome