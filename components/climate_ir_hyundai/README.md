# Hyundai IR Climate

Controls a Hyundai air conditioner using an infrared transmitter. Supports
cooling, heating, dry, fan-only, three fan speeds, vertical swing, and sleep
mode. An optional infrared receiver synchronizes the climate entity with
commands from the handheld remote.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [climate_ir_hyundai]

remote_transmitter:
  id: ir_tx
  pin: GPIO17
  carrier_duty_percent: 50%

remote_receiver:
  id: ir_rx
  pin:
    number: GPIO4
    inverted: true
    mode: INPUT_PULLUP

climate:
  - platform: climate_ir_hyundai
    name: Hyundai air conditioner
    transmitter_id: ir_tx
    receiver_id: ir_rx
    use_fahrenheit: false
```

Use the GPIOs connected to your infrared transmitter and receiver. The receiver
example assumes an active-low infrared receiver module. Omit `remote_receiver`
and `receiver_id` when only transmission is needed.

## Climate configuration

| Option | Description |
| --- | --- |
| `name` | Name of the climate entity. |
| `id` | Optional ID for use in automations. |
| `transmitter_id` | Infrared transmitter to use. Can be omitted when there is only one transmitter. |
| `receiver_id` | Optional infrared receiver used to track handheld remote commands. |
| `use_fahrenheit` | Send temperatures in Fahrenheit. Defaults to `false`. ESPHome target temperatures remain in Celsius. Received remote commands also update the transmitted unit selection. |
| `supports_cool` | Advertise cooling mode. Defaults to `true`. |
| `supports_heat` | Advertise heating mode. Defaults to `true`. |
| `sensor` | Optional temperature sensor ID used for the current temperature. |
| `humidity_sensor` | Optional humidity sensor ID used for the current humidity. |

Target temperatures range from 16 to 31 °C in 1 °C steps. Fan modes are low,
medium, and high. Swing can be off or vertical, and the sleep preset can be
enabled or disabled. Standard ESPHome climate options and automations are also
available.

The air conditioner does not report its state directly. Commands sent by this
component update the climate entity immediately. With a receiver configured,
recognized handheld remote commands also update it. The optional temperature
and humidity sensors provide local readings; they do not change the air
conditioner's own thermostat control.
