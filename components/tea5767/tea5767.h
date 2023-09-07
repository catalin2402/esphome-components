#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace tea5767 {

#define QUARTZ 32768
#define FILTER 225000

#define STAT_3 0x02
#define STAT_3_STEREO 0x80

#define STAT_4 0x03
#define STAT_4_ADC 0xF0

class TEA5767 : public PollingComponent, public i2c::I2CDevice {

public:
  TEA5767() : PollingComponent() {}
  void dump_config() override;
  void setup() override;
  void update() override;
  void set_mono_sensor(binary_sensor::BinarySensor *mono_sensor) {
    mono_sensor_ = mono_sensor;
  }
  void set_level_sensor(sensor::Sensor *level_sensor) {
    level_sensor_ = level_sensor;
  }
  void set_frequency_sensor(sensor::Sensor *frequency_sensor) {
    frequency_sensor_ = frequency_sensor;
  }

  void set_in_japan(bool in_japan) { this->in_japan_ = in_japan; };
  void set_frequency(uint64_t frequency);
  void set_mono(bool mono);
  void set_mute(bool mute);

  uint64_t get_frequency(void);
  uint8_t get_level(void);
  bool is_stereo(void);

protected:
  uint8_t registers_[5];
  uint8_t status_[5];
  bool in_japan_ = false;
  bool read_registers_();
  void save_registers_();
  binary_sensor::BinarySensor *mono_sensor_{nullptr};
  sensor::Sensor *level_sensor_{nullptr};
  sensor::Sensor *frequency_sensor_{nullptr};
};

} // namespace tea5767
} // namespace esphome