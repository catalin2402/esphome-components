#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"


namespace esphome {
namespace qr_reader {

class QrReader : public text_sensor::TextSensor,
                 public Component,
                 public uart::UARTDevice {
public:
  void loop() override;
  void dump_config() override;

protected:
  void check_buffer_();
  void sanitize_buffer_();

  std::string buffer_;
};

} // namespace qr_reader
} // namespace esphome
