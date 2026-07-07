#include "auriol_4ld5661.h"

namespace esphome {
namespace auriol_4ld5661 {

static const char *const TAG = "auriol_4ld5661";
static const uint32_t RAIN_COUNTER_MAX = 0xFFFFF;  // 20-bit monotonic rain counter
static const uint32_t RAIN_ROLLING_12H_MS = 12UL * 60UL * 60UL * 1000UL;
static const uint32_t RAIN_ROLLING_24H_MS = 24UL * 60UL * 60UL * 1000UL;
static const uint32_t RAIN_ROLLING_PUBLISH_INTERVAL_MS = 60UL * 1000UL;

static bool in_range_(uint32_t value, uint32_t min, uint32_t max) {
  return value >= min && value <= max;
}

bool Auriol4LD5661::on_receive(remote_base::RemoteReceiveData data) {
  const auto &raw = data.get_raw_data();

  if (raw.size() < 104)
    return false;

  for (size_t start = 0; start + (52 * 2) < raw.size(); start++) {
    if (this->try_decode_from_gap_(raw, start, false))
      return true;
    if (this->try_decode_from_gap_(raw, start, true))
      return true;
  }

  return false;
}

void Auriol4LD5661::loop() {
  const uint32_t now = millis();
  if (raining_ && now - last_tip_millis_ >= rain_timeout_ms_) {
    this->finish_rain_event_();
  }

  if (has_published_rolling_totals_ &&
      now - last_rolling_publish_millis_ >= RAIN_ROLLING_PUBLISH_INTERVAL_MS) {
    this->publish_rolling_rain_totals_(false);
  }
}

bool Auriol4LD5661::try_decode_from_gap_(const std::vector<int32_t> &raw, size_t first_gap_index, bool inverted) {
  std::array<uint8_t, 7> b{};
  b.fill(0);

  for (uint8_t bit_index = 0; bit_index < 52; bit_index++) {
    size_t raw_index = first_gap_index + bit_index * 2;
    if (raw_index >= raw.size())
      return false;

    uint32_t width = std::abs(raw[raw_index]);

    bool bit;
    if (in_range_(width, 700, 1350)) {
      bit = false;
    } else if (in_range_(width, 1600, 2400)) {
      bit = true;
    } else {
      return false;
    }

    if (inverted)
      bit = !bit;

    if (bit)
      b[bit_index / 8] |= 1 << (7 - (bit_index % 8));
  }

  return this->try_candidate_(b);
}

bool Auriol4LD5661::try_candidate_(const std::array<uint8_t, 7> &b) {
  uint8_t id = b[0];

  if (has_auriol_id_ && id != auriol_id_)
    return false;

  if (b[3] != 0xF0)
    return false;

  if ((b[1] & 0x70) != 0)
    return false;

  // The protocol bit is "battery OK"; Home Assistant battery binary_sensor
  // device_class expects ON=true to mean "battery low", so invert it.
  bool battery_ok = (b[1] >> 7) & 0x01;
  bool battery_low = !battery_ok;

  int16_t temp_raw = static_cast<int16_t>(((b[1] & 0x0F) << 12) | (b[2] << 4));
  float temperature = (temp_raw >> 4) * 0.1f;

  if (temperature < -60.0f || temperature > 80.0f)
    return false;

  uint32_t rain_raw = (static_cast<uint32_t>(b[4]) << 12) |
                      (static_cast<uint32_t>(b[5]) << 4) |
                      (b[6] >> 4);

  float rain_mm = rain_raw * mm_per_tip_;

  ESP_LOGD(TAG,
           "Auriol decoded: id=%02X battery_low=%s temp=%.1f rain=%.1fmm rain_tips=%u bytes=%02X %02X %02X %02X %02X %02X %02X",
           id,
           battery_low ? "true" : "false",
           temperature,
           rain_mm,
           rain_raw,
           b[0], b[1], b[2], b[3], b[4], b[5], b[6]);

  if (temperature_sensor_ != nullptr)
    temperature_sensor_->publish_state(temperature);

  if (rain_sensor_ != nullptr)
    rain_sensor_->publish_state(rain_mm);

  if (rain_tips_sensor_ != nullptr)
    rain_tips_sensor_->publish_state(rain_raw);

  if (battery_sensor_ != nullptr)
    battery_sensor_->publish_state(battery_low);

  this->handle_rain_counter_(rain_raw);

  return true;
}

void Auriol4LD5661::handle_rain_counter_(uint32_t rain_raw) {
  if (!has_last_rain_raw_) {
    has_last_rain_raw_ = true;
    last_rain_raw_ = rain_raw;
    this->publish_rolling_rain_totals_(true);
    return;
  }

  uint32_t delta = this->rain_counter_delta_(last_rain_raw_, rain_raw);

  if (delta == 0)
    return;

  if (delta > 1000) {
    ESP_LOGW(TAG, "Ignoring implausible rain counter jump: previous=%u current=%u delta=%u",
             last_rain_raw_, rain_raw, delta);
    last_rain_raw_ = rain_raw;
    return;
  }

  this->add_rain_history_(delta);

  if (!raining_) {
    this->start_rain_event_(last_rain_raw_, rain_raw);
  } else {
    current_event_tips_ = this->rain_counter_delta_(rain_start_base_tips_, rain_raw);
    last_tip_millis_ = millis();
    this->publish_current_event_total_();

    ESP_LOGD(TAG, "Rain event updated: tips=%u total=%.1fmm",
             current_event_tips_, current_event_tips_ * mm_per_tip_);
  }

  last_rain_raw_ = rain_raw;
}

uint32_t Auriol4LD5661::rain_counter_delta_(uint32_t from, uint32_t to) const {
  if (to >= from)
    return to - from;

  return (RAIN_COUNTER_MAX - from) + to + 1;
}

void Auriol4LD5661::start_rain_event_(uint32_t previous_rain_raw, uint32_t current_rain_raw) {
  raining_ = true;
  rain_start_base_tips_ = previous_rain_raw;
  current_event_tips_ = this->rain_counter_delta_(previous_rain_raw, current_rain_raw);
  last_tip_millis_ = millis();

  const std::string ts = this->now_string_();

  ESP_LOGI(TAG, "Rain started: start=%s tips=%u total=%.1fmm",
           ts.c_str(), current_event_tips_, current_event_tips_ * mm_per_tip_);

  if (start_of_rain_sensor_ != nullptr)
    start_of_rain_sensor_->publish_state(ts);

  this->publish_current_event_total_();
}

void Auriol4LD5661::finish_rain_event_() {
  raining_ = false;

  const std::string ts = this->now_string_();
  const float total_mm = current_event_tips_ * mm_per_tip_;

  ESP_LOGI(TAG, "Rain ended: end=%s tips=%u total=%.1fmm",
           ts.c_str(), current_event_tips_, total_mm);

  if (end_of_rain_sensor_ != nullptr)
    end_of_rain_sensor_->publish_state(ts);

  if (total_last_rain_sensor_ != nullptr)
    total_last_rain_sensor_->publish_state(total_mm);

  current_event_tips_ = 0;
}

void Auriol4LD5661::publish_current_event_total_() {
  if (total_last_rain_sensor_ != nullptr)
    total_last_rain_sensor_->publish_state(current_event_tips_ * mm_per_tip_);
}

void Auriol4LD5661::add_rain_history_(uint32_t tips) {
  if (total_rain_last_12h_sensor_ == nullptr && total_rain_last_24h_sensor_ == nullptr)
    return;

  rain_history_.push_back({millis(), tips});
  this->publish_rolling_rain_totals_(true);
}

void Auriol4LD5661::prune_rain_history_(uint32_t now) {
  auto first_valid = rain_history_.begin();
  while (first_valid != rain_history_.end() &&
         now - first_valid->timestamp_ms > RAIN_ROLLING_24H_MS) {
    ++first_valid;
  }

  if (first_valid != rain_history_.begin())
    rain_history_.erase(rain_history_.begin(), first_valid);
}

uint32_t Auriol4LD5661::rain_history_total_(uint32_t now, uint32_t window_ms) const {
  uint32_t total = 0;
  for (const auto &entry : rain_history_) {
    if (now - entry.timestamp_ms <= window_ms)
      total += entry.tips;
  }
  return total;
}

void Auriol4LD5661::publish_rolling_rain_totals_(bool force) {
  if (total_rain_last_12h_sensor_ == nullptr && total_rain_last_24h_sensor_ == nullptr)
    return;

  const uint32_t now = millis();
  if (!force && has_published_rolling_totals_ &&
      now - last_rolling_publish_millis_ < RAIN_ROLLING_PUBLISH_INTERVAL_MS) {
    return;
  }

  this->prune_rain_history_(now);

  if (total_rain_last_12h_sensor_ != nullptr) {
    total_rain_last_12h_sensor_->publish_state(
        this->rain_history_total_(now, RAIN_ROLLING_12H_MS) * mm_per_tip_);
  }

  if (total_rain_last_24h_sensor_ != nullptr) {
    total_rain_last_24h_sensor_->publish_state(
        this->rain_history_total_(now, RAIN_ROLLING_24H_MS) * mm_per_tip_);
  }

  last_rolling_publish_millis_ = now;
  has_published_rolling_totals_ = true;
}

std::string Auriol4LD5661::now_string_() const {
  if (time_ == nullptr)
    return "no_time_source";

  auto now = time_->now();
  if (!now.is_valid())
    return "time_not_valid";

  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year, now.month, now.day_of_month, now.hour, now.minute, now.second);
  return std::string(buffer);
}

}  // namespace auriol_4ld5661
}  // namespace esphome
