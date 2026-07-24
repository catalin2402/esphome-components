# RF Remote

Decodes 24-bit RC Switch protocol 2 frames into an 18-bit remote identifier
and a 6-bit command. It can publish the command as a numeric sensor or expose
a particular command as a binary sensor.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [rf_remote]

remote_receiver:
  id: rf_receiver
  pin: GPIO4
  filter: 250us
  idle: 4ms

sensor:
  - platform: rf_remote
    receiver_id: rf_receiver
    remote_id: 0x18B91
    reset_time: 50ms
    name: RF command

binary_sensor:
  - platform: rf_remote
    receiver_id: rf_receiver
    remote_id: 0x18B91
    remote_button: 3
    reset_time: 50ms
    name: RF button 3
```

`remote_id` accepts an integer, a `0x`-prefixed value or a one-to-five digit
hexadecimal string in the range `0x00000`–`0x3FFFF`. `remote_button` ranges
from 0 to 63. After `reset_time`, the numeric sensor returns to zero and the
binary sensor returns to off.
