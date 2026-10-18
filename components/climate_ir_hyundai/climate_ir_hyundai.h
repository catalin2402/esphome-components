#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome::climate_ir_hyundai {

class HyundaiIrClimate : public climate_ir::ClimateIR {
 public:
  HyundaiIrClimate();
  void dump_config() override;
  void set_fahrenheit(bool value) { this->fahrenheit_ = value; }

 protected:
  climate::ClimateTraits traits() override;
  void transmit_state() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;
  static uint8_t checksum_(uint64_t value);
  void transmit_(uint64_t value);

  bool fahrenheit_{false};
  uint64_t mode_before_{0x10000000000ULL};
};

}  // namespace esphome::climate_ir_hyundai
