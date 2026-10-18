# Xiron

Receives Xiron weather sensor messages and exposes temperature, humidity
and battery status as ESPHome entities.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [xiron]

xiron:

remote_receiver:
  id: rf_receiver
  pin: GPIO4
  tolerance: 25%
  filter: 250us
  idle: 4ms
  dump:
    - xiron

sensor:
  - platform: xiron
    type: temperature
    name: "Outdoor Temperature"
    receiver_id: rf_receiver
    xiron_id: 0x1201

  - platform: xiron
    type: humidity
    name: "Outdoor Humidity"
    receiver_id: rf_receiver
    xiron_id: 0x1201

binary_sensor:
  - platform: xiron
    type: battery
    name: "Battery"
    receiver_id: rf_receiver
    xiron_id: 0x1201
```

Set `pin` to the GPIO connected to your RF receiver and `xiron_id` to your
weather sensor's device ID. Configure only the entities you need.

## Configuration

### Component

`xiron:` enables support for Xiron messages. It has no additional
configuration options.

### Receiver and packet logging

Configure the RF receiver under `remote_receiver`. Add `xiron` to its `dump`
list to log received device IDs, channels, battery status, temperature and
humidity at INFO level. Use the logged `device_id` to configure `xiron_id`.
Packet logging is optional and works without any entities.

### Common entity options

These options apply to both sensor and binary sensor entries:

| Option | Required | Purpose |
| --- | --- | --- |
| `type` | Yes | Selects the reading, as described below. |
| `xiron_id` | No | Filters messages to a device ID. Accepts a number from 0 to 65535, such as `4609` or `0x1201`, without quotes. If omitted, accepts any matching device. |
| `receiver_id` | When multiple receivers are configured | Selects the RF receiver. |

The device ID combines the sensor ID and channel. For sensor ID `0x12` on
channel 1, use `xiron_id: 0x1201`. A supplied value of zero matches device ID
zero; omit the option to receive any device. Set `xiron_id` on each entity
when multiple weather sensors are in range.

Standard ESPHome entity options, such as `name` and `filters`, are supported.

### Sensors

Use `platform: xiron` under `sensor`:

| `type` | Reading |
| --- | --- |
| `temperature` | Temperature in degrees Celsius. |
| `humidity` | Relative humidity in percent. |

### Binary sensors

Use `platform: xiron` and `type: battery` under `binary_sensor`. The entity is
ON when the device reports a low battery and OFF when the battery is OK.
It retains the reported state until another message updates it.
