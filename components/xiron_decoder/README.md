# Xiron Decoder

Decodes compatible Xiron 433 MHz weather-sensor frames received by ESPHome's
remote receiver. It can publish temperature, humidity and the sensor's
battery-low flag.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [xiron_decoder]

remote_receiver:
  id: rf_receiver
  pin: GPIO4
  tolerance: 50%
  filter: 250us
  idle: 4ms

sensor:
  - platform: xiron_decoder
    receiver_id: rf_receiver
    xiron_id: 0x1234
    temperature:
      name: Xiron temperature
    humidity:
      name: Xiron humidity
    battery:
      name: Xiron battery low
```

`xiron_id` accepts an integer, a `0x`-prefixed value or a one-to-four digit
hexadecimal string in the range `0x0000`–`0xFFFF`. All three output entities
are optional.
