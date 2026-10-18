# Auriol 4LD5661

Receives Auriol weather sensor messages and exposes temperature, cumulative
rainfall, rain tip count and battery status as ESPHome entities.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [auriol_4ld5661]

auriol_4ld5661:

remote_receiver:
  id: rf_receiver
  pin: GPIO4
  tolerance: 25%
  filter: 250us
  idle: 4ms
  dump:
    - auriol_4ld5661

sensor:
  - platform: auriol_4ld5661
    type: temperature
    name: "Outdoor Temperature"
    receiver_id: rf_receiver
    auriol_id: 0x3A

  - platform: auriol_4ld5661
    type: rain
    name: "Rain"
    receiver_id: rf_receiver
    auriol_id: 0x3A
    mm_per_tip: 0.3

  - platform: auriol_4ld5661
    type: rain_tips
    name: "Rain Tips"
    receiver_id: rf_receiver
    auriol_id: 0x3A

binary_sensor:
  - platform: auriol_4ld5661
    type: battery
    name: "Battery"
    receiver_id: rf_receiver
    auriol_id: 0x3A
```

Set `pin` to the GPIO connected to your RF receiver and `auriol_id` to your
weather sensor's ID. Configure only the entities you need.

## Configuration

### Component

`auriol_4ld5661:` enables support for Auriol messages. It has no additional
configuration options.

### Receiver and packet logging

Configure the RF receiver under `remote_receiver`. Add `auriol_4ld5661` to
its `dump` list to log received device IDs, channels, battery status,
temperature and rain tips at INFO level. Use the logged ID to configure
`auriol_id`. Packet logging is optional and works without any entities.

### Common entity options

These options apply to both sensor and binary sensor entries:

| Option | Required | Purpose |
| --- | --- | --- |
| `type` | Yes | Selects the reading, as described below. |
| `auriol_id` | No | Filters messages to a device ID from 0 to 255. Use a number such as `255` or `0xFF`, without quotes. If omitted, accepts any matching device. |
| `receiver_id` | When multiple receivers are configured | Selects the RF receiver. |

Set `auriol_id` on each entity when multiple weather sensors are in range.
Standard ESPHome entity options, such as `name` and `filters`, are supported.

### Sensors

Use `platform: auriol_4ld5661` under `sensor`:

| `type` | Reading |
| --- | --- |
| `temperature` | Temperature in degrees Celsius. |
| `rain` | Cumulative rainfall in millimetres, calculated from the device's rain tip counter. |
| `rain_tips` | Raw cumulative number of rain bucket tips reported by the device. |

For `type: rain`, `mm_per_tip` sets the rainfall in millimetres per bucket
tip (default: `0.3`). It is available only for this sensor type. Rainfall
and tip counts follow the device's cumulative counter; they are not daily
totals.

### Binary sensors

Use `platform: auriol_4ld5661` and `type: battery` under `binary_sensor`.
The entity is ON when the device reports a low battery and OFF when the
battery is OK. It retains the reported state until another message updates it.
