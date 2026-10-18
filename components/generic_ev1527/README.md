# Generic EV1527

Receives EV1527 remote control messages, logs device IDs and button values,
and exposes matching button presses as binary sensors.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [generic_ev1527]

generic_ev1527:

remote_receiver:
  id: rf_receiver
  pin: GPIO4
  tolerance: 50%
  filter: 250us
  idle: 4ms
  dump:
    - generic_ev1527

binary_sensor:
  - platform: remote_receiver
    name: "EV1527 Button"
    receiver_id: rf_receiver
    generic_ev1527:
      id: 0x98B91
      data: 5
    filters:
      - delayed_off: 250ms
```

Set `pin` to the GPIO connected to your RF receiver.

## Configuration

### Component

`generic_ev1527:` enables support for EV1527 messages. It has no additional
configuration options.

### Receiver and packet logging

Configure the RF receiver under `remote_receiver`. Add `generic_ev1527` to
its `dump` list to log received device IDs and data values at INFO level.
Use these values to configure button matching. Packet logging is optional
and works without any binary sensors.

### Binary sensor

Use `platform: remote_receiver` with a nested `generic_ev1527` block:

| Option | Required | Purpose |
| --- | --- | --- |
| `id` | Yes | Device ID, from 0 to 1048575 (`0xFFFFF`). Accepts decimal or hexadecimal notation. |
| `data` | Yes | Button data value to match, from 0 to 15. |
| `receiver_id` | When multiple receivers are configured | Selects the receiver; placed outside the nested block. |

The binary sensor emits an ON/OFF pulse for each matching message. The
example's `delayed_off` filter holds it ON until 250 ms after the last match,
so the state is visible in Home Assistant. Create one entry per button.
