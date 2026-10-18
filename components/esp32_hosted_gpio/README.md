# ESP32 Hosted GPIO

Uses unused GPIOs on an ESP-Hosted co-processor as ESPHome GPIO pins. These pins
can drive standard GPIO switches and outputs, or provide inputs to standard
GPIO binary sensors. Pin configuration and output state are restored after a
communication failure is detected.

Requires an ESP32 host using ESP-IDF and a configured `esp32_hosted` transport.

## Example configuration

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
  on_ready:
    - logger.log: "Co-processor GPIOs are ready"
  on_not_ready:
    - logger.log: "Co-processor GPIOs need recovery"

switch:
  - platform: gpio
    name: "Co-processor Output"
    pin:
      esp32_hosted_gpio: hosted_gpio
      number: 5
      mode:
        output: true

binary_sensor:
  - platform: gpio
    name: "Co-processor Input"
    use_interrupt: false
    pin:
      esp32_hosted_gpio: hosted_gpio
      number: 6
      mode:
        input: true
        pullup: true
```

The `esp32_hosted` pins are pins on the host; the GPIO pin `number` values
refer to the co-processor. Adapt the transport wiring and co-processor pins to
your board. The co-processor firmware must support GPIO-expander commands.

## Hub configuration

| Option | Description |
| --- | --- |
| `id` | Optional hub ID. |
| `reserved_pins` | Co-processor GPIO numbers that must not be used by this component. Defaults to an empty list. |
| `on_ready` | Automation fired when the link has responded and every registered pin configuration and queued output write has completed. |
| `on_not_ready` | Automation fired when a previously ready GPIO hub requires recovery or reconfiguration. |

Known stock-firmware SDIO pins are reserved automatically. With SPI, or an SDIO
variant whose slave pin mapping is unknown, `reserved_pins` must list all
co-processor transport pins. Add flash, PSRAM, board peripherals, and any other
unavailable GPIOs to this list as appropriate for the co-processor firmware.
For custom firmware, reserve its actual transport pins as well. The supported
pin numbers and modes ultimately depend on the co-processor hardware and
firmware.

## GPIO pin configuration

Use the following fields inside a standard ESPHome GPIO pin block:

| Option | Description |
| --- | --- |
| `esp32_hosted_gpio` | Required selector for this GPIO provider. Use the hub ID, or leave the value empty to select the single hub automatically. |
| `number` | Required co-processor GPIO number, from 0 to 63. It must exist on the selected co-processor and be available for GPIO use. |
| `inverted` | Invert logical reads and writes. Defaults to `false`. |
| `mode` | Input, output, or combined input/output. Supports `open_drain`, `pullup`, and `pulldown` when supported by the co-processor. |

Open drain requires output mode. Pull-up and pull-down cannot be enabled
together. Pins reserved for the transport or listed in `reserved_pins` are
rejected during validation. For example, with one hub:

```yaml
pin:
  esp32_hosted_gpio:
  number: 5
  mode:
    output: true
```

## Switches, outputs, and binary sensors

Use `switch: - platform: gpio` or `output: - platform: gpio` for output pins,
and `binary_sensor: - platform: gpio` for inputs. Their standard ESPHome
configuration options apply; this component does not need separate entity
platforms.

Remote inputs are polled and do not support host GPIO interrupts. Reads of registered input pins in the same bank are cached between hub loop
calls, unless invalidated by a write or reconfiguration. Each physical input read is a remote request, so
these pins are suited to ordinary controls and status signals rather than
precise pulse timing.

During recovery, input reads retain the last sampled level. Before the first
successful sample, the raw level defaults to low. The GPIO interface cannot
represent an unavailable input. Writes retain the latest requested output
state and apply it when communication resumes. Recovery restores outputs as
soon as their pin is configured; co-processor resets can still cause temporary
output changes.

## Action

`esp32_hosted_gpio.reconfigure` queues a new link check, reapplies registered
pin modes, and restores requested output states. It does not restart or flash
the co-processor, and it returns before recovery completes.

```yaml
button:
  - platform: template
    name: "Reconfigure Hosted GPIO"
    on_press:
      - esp32_hosted_gpio.reconfigure:
```

The ID can be omitted for the single hub, as shown, or supplied directly:

```yaml
on_press:
  - esp32_hosted_gpio.reconfigure: hosted_gpio
```

## Conditions

| Condition | Meaning |
| --- | --- |
| `esp32_hosted_gpio.is_ready` | The last link check succeeded and all registered GPIO configuration and queued writes are applied. |
| `esp32_hosted_gpio.has_pending` | At least one pin still needs configuration or an output write. |

Conditions accept an optional hub ID, with the same syntax as the action. They
inspect cached state and do not make remote requests. `has_pending` can be false
while the link is not ready if no pins are registered.

```yaml
on_press:
  - if:
      condition:
        esp32_hosted_gpio.is_ready:
      then:
        - logger.log: "Hosted GPIO is ready"
```

## Triggers

`on_ready` and `on_not_ready` have no lambda arguments and fire only on readiness
transitions. Startup begins not ready without firing `on_not_ready`; the first
successful initialization fires `on_ready`. Reconfiguration or a detected
remote error causes `on_not_ready` if the hub was previously ready, followed by
`on_ready` after recovery.

Failures are discovered by GPIO requests or link checks needed for recovery;
there is no continuous background link health probe when the hub is idle.
