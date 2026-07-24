# PT2323 Audio Processor

Controls a PT2323 six-channel audio selector and processor over I²C. It
supports input selection, 3D enhancement, bass boost and channel muting.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [pt2323]

i2c:
  sda: GPIO21
  scl: GPIO22

pt2323:
  id: audio_selector
  address: 0x4A
  input: 1
  enhance: false
  boost: false
  mute: false

select:
  - platform: pt2323
    pt2323_id: audio_selector
    name: Audio input
    options:
      - Auxiliary
      - Bluetooth

switch:
  - platform: pt2323
    pt2323_id: audio_selector
    type: mute
    channel_a: 1
    channel_b: 2
    name: Front channel mute
```

Inputs range from 0 to 4 and channels from 1 to 6. Switch types are
`enhance`, `boost`, `mute` and `mute_all`.

Available actions are `pt2323.set_input`, `pt2323.mute`, `pt2323.unmute`,
`pt2323.mute_channel`, `pt2323.unmute_channel`, `pt2323.set_enhance`,
`pt2323.set_boost` and `pt2323.resend_data`. The default I²C address is
`0x4A`.
