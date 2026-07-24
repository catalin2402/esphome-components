# Hyundai IR Climate

Adds bidirectional support for the 56-bit Hyundai air-conditioner infrared
protocol. It integrates with ESPHome's Climate IR platform and supports
cooling, heating, dry, fan-only, vertical swing and sleep mode.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [climate_ir_hyundai]

remote_transmitter:
  id: ir_tx
  pin: GPIO17
  carrier_duty_percent: 50%
  non_blocking: true

remote_receiver:
  id: ir_rx
  pin: GPIO4

climate:
  - platform: climate_ir_hyundai
    name: Hyundai air conditioner
    transmitter_id: ir_tx
    receiver_id: ir_rx
    use_fahrenheit: false
```

`receiver_id` is optional when only transmission is required.
`use_fahrenheit` controls the temperature representation sent in the IR frame;
ESPHome still exposes temperatures using its normal climate conventions.
