# ESP32 Hosted GPIO

Exposes unused GPIOs on an ESP32 Hosted Wi-Fi co-processor as normal ESPHome
GPIO pins. Communication uses the GPIO-expander RPC support in ESP-Hosted.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [esp32_hosted_gpio]

esp32_hosted:
  type: sdio
  variant: ESP32C6
  active_high: true
  reset_pin: GPIO4
  clk_pin: GPIO12
  cmd_pin: GPIO13
  d0_pin: GPIO14
  bus_width: 1

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

esp32_hosted_gpio:
  id: hosted_gpio

switch:
  - platform: gpio
    name: Co-processor output
    pin:
      esp32_hosted_gpio: hosted_gpio
      number: 5
      mode:
        output: true

binary_sensor:
  - platform: gpio
    name: Co-processor input
    pin:
      esp32_hosted_gpio: hosted_gpio
      number: 6
      mode:
        input: true
```

The component automatically rejects SDIO pins reserved by the stock
co-processor firmware. For SPI transports, set `reserved_pins` to every
co-processor GPIO consumed by SPI, handshake and data-ready signals.

Remote inputs are polled because they cannot use interrupts on the host.
Open-drain, pull-up, pull-down and inverted pin modes are supported when the
co-processor firmware supports them.
