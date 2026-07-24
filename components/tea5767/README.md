# TEA5767 FM Radio

Controls a TEA5767 FM stereo receiver over I²C. It supports tuning, signal
level and stereo status reporting, mute/mono controls and the chip's
noise-reduction features.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [tea5767]

i2c:
  sda: GPIO21
  scl: GPIO22

tea5767:
  id: radio
  address: 0x60
  frequency: 99.5MHz
  update_interval: 5s
  mono_sensor:
    name: Radio mono
  level_sensor:
    name: Radio signal level
  frequency_sensor:
    name: Radio frequency

switch:
  - platform: tea5767
    tea5767_id: radio
    type: mute
    name: Radio mute
```

The normal tuning range is 87.5–108 MHz. Set `in_japan: true` for the
Japanese band configuration. Switch types are `mute`, `mono`, `soft_mute`,
`stereo_noise_canceling` and `high_cut_control`.

Equivalent automation actions are available as `tea5767.set_frequency`,
`tea5767.set_mute`, `tea5767.set_mono`, `tea5767.set_soft_mute`,
`tea5767.set_stereo_noise_canceling` and `tea5767.set_high_cut_control`.
