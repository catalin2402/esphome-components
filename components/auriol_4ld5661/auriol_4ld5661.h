#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace esphome {
namespace auriol_4ld5661 {

class Auriol4LD5661 : public remote_base::RemoteReceiverListener, public Component {
 public:
  void set_auriol_id(uint8_t id) {
    auriol_id_ = id;
    has_auriol_id_ = true;
  }

  void set_temperature_sensor(sensor::Sensor *s) { temperature_sensor_ = s; }
  void set_rain_sensor(sensor::Sensor *s) { rain_sensor_ = s; }
  void set_rain_tips_sensor(sensor::Sensor *s) { rain_tips_sensor_ = s; }
  void set_total_last_rain_sensor(sensor::Sensor *s) { total_last_rain_sensor_ = s; }
  void set_total_rain_last_12h_sensor(sensor::Sensor *s) { total_rain_last_12h_sensor_ = s; }
  void set_total_rain_last_24h_sensor(sensor::Sensor *s) { total_rain_last_24h_sensor_ = s; }
  void set_battery_sensor(binary_sensor::BinarySensor *s) { battery_sensor_ = s; }
  void set_start_of_rain_sensor(text_sensor::TextSensor *s) { start_of_rain_sensor_ = s; }
  void set_end_of_rain_sensor(text_sensor::TextSensor *s) { end_of_rain_sensor_ = s; }
  void set_time(time::RealTimeClock *time) { time_ = time; }
  void set_rain_timeout(uint32_t timeout_ms) { rain_timeout_ms_ = timeout_ms; }
  void set_mm_per_tip(float mm_per_tip) { mm_per_tip_ = mm_per_tip; }

  bool on_receive(remote_base::RemoteReceiveData data) override;
  void loop() override;

 protected:
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *rain_sensor_{nullptr};
  sensor::Sensor *rain_tips_sensor_{nullptr};
  sensor::Sensor *total_last_rain_sensor_{nullptr};
  sensor::Sensor *total_rain_last_12h_sensor_{nullptr};
  sensor::Sensor *total_rain_last_24h_sensor_{nullptr};
  binary_sensor::BinarySensor *battery_sensor_{nullptr};
  text_sensor::TextSensor *start_of_rain_sensor_{nullptr};
  text_sensor::TextSensor *end_of_rain_sensor_{nullptr};
  time::RealTimeClock *time_{nullptr};

  uint8_t auriol_id_{0};
  bool has_auriol_id_{false};

  uint32_t rain_timeout_ms_{3600000};
  float mm_per_tip_{0.3f};

  bool has_last_rain_raw_{false};
  uint32_t last_rain_raw_{0};

  bool raining_{false};
  uint32_t rain_start_base_tips_{0};
  uint32_t current_event_tips_{0};
  uint32_t last_tip_millis_{0};
  uint32_t last_rolling_publish_millis_{0};
  bool has_published_rolling_totals_{false};

  struct RainHistoryEntry {
    uint32_t timestamp_ms;
    uint32_t tips;
  };
  std::vector<RainHistoryEntry> rain_history_;

  bool try_decode_from_gap_(const std::vector<int32_t> &raw, size_t first_gap_index, bool inverted);
  bool try_candidate_(const std::array<uint8_t, 7> &b);

  void handle_rain_counter_(uint32_t rain_raw);
  uint32_t rain_counter_delta_(uint32_t from, uint32_t to) const;
  void start_rain_event_(uint32_t previous_rain_raw, uint32_t current_rain_raw);
  void finish_rain_event_();
  void publish_current_event_total_();
  void add_rain_history_(uint32_t tips);
  void prune_rain_history_(uint32_t now);
  uint32_t rain_history_total_(uint32_t now, uint32_t window_ms) const;
  void publish_rolling_rain_totals_(bool force);
  std::string now_string_() const;
};

}  // namespace auriol_4ld5661
}  // namespace esphome
