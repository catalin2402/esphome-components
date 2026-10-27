# Sound Reactive Lights

Controls an existing light using audio passing through a speaker. The speaker
wrapper forwards audio to its output speaker and analyzes the accepted samples
to produce VU, spectrum, or rainbow pulse lighting. It accepts 16-bit mono or
stereo audio from 16000 Hz to 48000 Hz.

## Example configuration

Configure an output speaker named `speaker_id` and a light named `reactive_light`
first. Use `reactive_speaker` as the speaker in the audio playback pipeline.

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [sound_reactive_lights]

speaker:
  - platform: sound_reactive_lights
    id: reactive_speaker
    output_speaker: speaker_id
    light_id: reactive_light
    mode: spectrum

select:
  - platform: sound_reactive_lights
    name: Sound Reactive Mode
    sound_reactive_lights_id: reactive_speaker
```

## Speaker configuration

| Option | Description |
| --- | --- |
| `id` | Optional speaker ID used by the playback pipeline, select, and actions. |
| `output_speaker` | Required ID of the speaker that receives the audio. |
| `light_id` | Required ID of the light to control. Each light can have one reactive speaker. |
| `mode` | Initial mode: `off`, `vu`, `spectrum`, or `rainbow_pulse`. Default: `spectrum`. |
| `enabled` | Whether reactive lighting starts enabled. Default: `true`. |
| `sensitivity` | Audio response multiplier, from 0.1 to 100. Default: `4.0`. |
| `max_brightness` | Maximum reactive brightness. Default: `100%`. |
| `update_interval` | Minimum time between light updates. Default: `20ms`. |
| `silence_timeout` | Time after the last analyzed audio block before clearing the lighting levels. Default: `250ms`. |

`vu` maps audio level to brightness and green-to-red color. `spectrum` maps low,
mid, and high audio bands to red, green, and blue. `rainbow_pulse` cycles colors
with audio-driven brightness. `off` renders the light off while reactive
lighting remains enabled. Monochrome lights use the brightness response.

Disabling reactive lighting restores the previously saved light state and
effect. Manually turning the light off or selecting another effect disables
reactive control. Audio forwarding continues when reactive lighting is disabled.

## Mode select

The optional select exposes `Off`, `VU`, `Spectrum`, and `Rainbow Pulse`.
`sound_reactive_lights_id` references the reactive speaker and can be omitted
when a single reactive speaker is configured.
Only one mode select can control each reactive speaker. Standard select entity
options apply. Selecting a mode also enables reactive lighting.

## Actions

| Action | Description |
| --- | --- |
| `sound_reactive_lights.enable` | Enable reactive lighting and save the current light state for restoration. |
| `sound_reactive_lights.disable` | Disable reactive lighting and restore the saved light state. |
| `sound_reactive_lights.next_mode` | Cycle to the next mode and enable reactive lighting. |
| `sound_reactive_lights.set_mode` | Set `mode` to `off`, `vu`, `spectrum`, or `rainbow_pulse`. Preserves the enabled state. |
| `sound_reactive_lights.set_sensitivity` | Set `sensitivity` from 0.1 to 100. |
| `sound_reactive_lights.set_max_brightness` | Set `max_brightness` from `0%` to `100%`. |

Action IDs are optional when a single reactive speaker is configured.

```yaml
on_press:
  - sound_reactive_lights.next_mode:
```

The three `set_` actions accept templated values. For example:

```yaml
on_press:
  - sound_reactive_lights.set_mode:
      mode: spectrum
  - sound_reactive_lights.set_sensitivity:
      sensitivity: 6.0
  - sound_reactive_lights.set_max_brightness:
      max_brightness: 75%
```

## Enable switch

The optional switch enables reactive lighting when ON and restores the saved
light state when OFF. It also follows enable/disable actions and manual light
overrides.

```yaml
switch:
  - platform: sound_reactive_lights
    name: Sound Reactive Lights
    sound_reactive_lights_id: reactive_speaker
```

`sound_reactive_lights_id` is optional when one reactive speaker is configured.
Only one enable switch can control each speaker. Standard switch options apply.
The default `restore_mode` is `DISABLED`, which preserves the speaker's initial
`enabled` setting. Choosing the `Off` mode keeps the switch ON; disabling
reactive lighting releases control of the light.

## Number controls

Expose sensitivity and maximum brightness as optional number entities:

```yaml
number:
  - platform: sound_reactive_lights
    type: sensitivity
    name: Sound Reactive Sensitivity
  - platform: sound_reactive_lights
    type: max_brightness
    name: Sound Reactive Maximum Brightness
```

| Option | Description |
| --- | --- |
| `sound_reactive_lights_id` | Optional ID of the reactive speaker. Required when the parent cannot be resolved automatically. |
| `type` | Required. `sensitivity` or `max_brightness`. |

Sensitivity ranges from 0.1 to 100 in steps of 0.1. Maximum brightness ranges
from 0 to 100 percent in steps of 1. Initial values come from the speaker
configuration. Runtime changes stay synchronized with actions. Only one number
of each type can control a speaker. Standard number entity options apply.

## Conditions

| Condition | Description |
| --- | --- |
| `sound_reactive_lights.is_enabled` | Reactive lighting is enabled, including when its selected mode is `off`. |
| `sound_reactive_lights.selected_mode` | The selected mode matches the required `mode` value. |

Condition IDs are optional when one reactive speaker is configured.

```yaml
condition:
  sound_reactive_lights.selected_mode:
    mode: spectrum
```

## Triggers

Configure triggers under the reactive speaker platform:

| Trigger | Arguments | Description |
| --- | --- | --- |
| `on_mode` | `uint8_t mode` | Mode changes: 0 = Off, 1 = VU, 2 = Spectrum, 3 = Rainbow Pulse. |
| `on_enabled` | `bool enabled` | Reactive control is enabled or disabled, including by a manual light override. |

```yaml
speaker:
  - platform: sound_reactive_lights
    id: reactive_speaker
    output_speaker: speaker_id
    light_id: reactive_light
    on_mode:
      - logger.log:
          format: "Reactive mode: %u"
          args: [mode]
    on_enabled:
      - logger.log:
          format: "Reactive lighting: %s"
          args: ['enabled ? "enabled" : "disabled"']
```
