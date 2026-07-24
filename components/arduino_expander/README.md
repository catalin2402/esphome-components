# Arduino Expander

Uses an Arduino-compatible AVR board as a 14-pin digital and 6-channel analog
I²C GPIO expander for ESPHome. Flash
[`arduino_expander.ino`](../../arduino_sketches/arduino_expander/arduino_expander.ino)
to the secondary board before using the component.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [arduino_expander]

i2c:
  sda: GPIO21
  scl: GPIO22

arduino_expander:
  id: expander
  address: 0x10
  adc_source: internal

binary_sensor:
  - platform: gpio
    name: Expander input
    pin:
      arduino_expander: expander
      number: 2
      mode:
        input: true
        pullup: true

output:
  - platform: gpio
    id: expander_output
    pin:
      arduino_expander: expander
      number: 8
      mode:
        output: true

sensor:
  - platform: arduino_expander
    arduino_expander: expander
    pin: 0
    name: Expander analog input
```

Digital pin numbers range from 0 to 13. Analog sensors support channels 0, 1,
2, 3, 6 and 7 as mapped by the companion sketch. `adc_source` accepts
`default`, `internal` or `external`.

The default I²C address is `0x10`; change `I2C_ADDRESS` in the Arduino sketch
as well when selecting a different address.
