# PT2258 Volume Controller

Controls a PT2258 six-channel electronic volume controller over I²C. It
supports master attenuation, per-channel volume and per-channel offsets.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [pt2258]

i2c:
  sda: GPIO21
  scl: GPIO22

pt2258:
  id: volume_controller
  address: 0x44
  default_volume: 20

number:
  - platform: pt2258
    pt2258_id: volume_controller
    type: master
    name: Master volume

  - platform: pt2258
    pt2258_id: volume_controller
    type: volume
    channel_a: 1
    channel_b: 2
    name: Front channels
```

Number `type` may be `master`, `volume` or `offset`. Channel numbers range
from 1 to 6. Non-master numbers require at least `channel_a` or `channel_b`.

Available actions are `pt2258.resend_data`, `pt2258.mute` and
`pt2258.unmute`. The default I²C address is `0x44`.
