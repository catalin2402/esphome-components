# Graphic Equalizer

Applies a five-band graphic equalizer to audio passing through an ESPHome
speaker. The bands are 63 Hz, 250 Hz, 1 kHz, 4 kHz, and 16 kHz, with independent
gain controls shared by both stereo channels. Supports ESP32, 16-bit PCM,
mono/stereo, and sample rates of 44100 or 48000 Hz.

Includes five built-in presets and three user presets. User presets store their
own band gains and preamp setting. The selected preset and user presets are
restored after a restart.

## Example configuration

Configure an output speaker named `speaker_id` first. Reference
`equalized_speaker` in the playback pipeline instead of the output speaker.
The select, switch, and number entries below are optional.

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [graphic_equalizer]

speaker:
  - platform: graphic_equalizer
    id: equalized_speaker
    output_speaker: speaker_id
    preset: flat
    auto_headroom: true

select:
  - platform: graphic_equalizer
    name: Equalizer Preset

switch:
  - platform: graphic_equalizer
    name: Equalizer Enabled

number:
  - platform: graphic_equalizer
    type: 63hz
    name: Equalizer 63 Hz
  - platform: graphic_equalizer
    type: 250hz
    name: Equalizer 250 Hz
  - platform: graphic_equalizer
    type: 1khz
    name: Equalizer 1 kHz
  - platform: graphic_equalizer
    type: 4khz
    name: Equalizer 4 kHz
  - platform: graphic_equalizer
    type: 16khz
    name: Equalizer 16 kHz
  - platform: graphic_equalizer
    type: preamp
    name: Equalizer Preamp
```

No top-level `graphic_equalizer:` block is needed. When several equalizer
speakers are configured, set `graphic_equalizer_id` on each control to select
its parent.

The output speaker can also be another speaker wrapper, such as
`sound_reactive_lights`. Put the equalizer before the reactive speaker to make
the lighting follow the equalized audio.

## Speaker configuration

| Option | Description |
| --- | --- |
| `id` | Speaker ID used by the playback pipeline and optional controls. Use an explicit, stable ID for saved presets. |
| `output_speaker` | Required ID of the speaker that receives the processed audio. |
| `preset` | Initial preset when no saved selection exists. Default: `flat`. |
| `bands` | Optional initial overrides for `63hz`, `250hz`, `1khz`, `4khz`, and `16khz`. Each accepts -12 to +12 dB. |
| `preamp` | Initial manual attenuation, from -36 to 0 dB. Default: 0 dB. |
| `enabled` | Whether the equalizer starts enabled. Default: `true`. |
| `auto_headroom` | Reduce gain to make room for boosted bands. Default: `true`. |
| `bits_per_sample` | Optional stream constraint. Must be `16`. |
| `num_channels` | Optional stream constraint. Must be `1` or `2`. |
| `sample_rate` | Optional stream constraint. Must be `44100` or `48000`. |

For example, initial gain overrides can be configured as:

```yaml
speaker:
  - platform: graphic_equalizer
    id: equalized_speaker
    output_speaker: speaker_id
    preset: user_1
    preamp: -3dB
    bands:
      63hz: 4dB
      250hz: 2dB
      1khz: 0dB
      4khz: -1dB
      16khz: 2dB
```

When saved presets exist, the saved selection and user curves take precedence
over the initial preset/gain configuration. Preset storage is tied to the
speaker ID. Changes use ESPHome's normal preferences write interval.

Automatic headroom subtracts the sum of positive band gains from the manual
preamp setting. For example, +4 dB and +2 dB boosts add 6 dB of attenuation.
The preamp number shows the manual setting; automatic attenuation is additional.
This conservative setting can lower perceived volume. Disable `auto_headroom`
to manage headroom manually. Output is saturated if samples exceed the 16-bit
range, which prevents wraparound but can still produce audible distortion.

Disabling the equalizer bypasses both filters and preamp while retaining the
settings. Gain and bypass changes fade over approximately 20ms. Volume, mute,
pause, and start/stop commands continue to control the output speaker. Configure
an audio DAC on the output speaker rather than on this wrapper.

## Presets and select

The optional select exposes the following presets. All values are in dB; built-in
presets use a manual preamp of 0 dB, with automatic headroom applied separately.

| Preset | YAML value | 63 Hz | 250 Hz | 1 kHz | 4 kHz | 16 kHz |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Flat | `flat` | 0 | 0 | 0 | 0 | 0 |
| Bass Boost | `bass_boost` | +6 | +3 | 0 | 0 | 0 |
| Treble Boost | `treble_boost` | 0 | 0 | 0 | +3 | +6 |
| Vocal | `vocal` | -3 | -2 | +3 | +2 | -2 |
| Rock | `rock` | +4 | +2 | -2 | +3 | +4 |
| User 1 | `user_1` | Saved | Saved | Saved | Saved | Saved |
| User 2 | `user_2` | Saved | Saved | Saved | Saved | Saved |
| User 3 | `user_3` | Saved | Saved | Saved | Saved | Saved |

User presets start flat. Adjusting a band or preamp while a user preset is
selected automatically updates that user preset. Adjustments to built-in
presets are temporary and keep the selected preset's name. Selecting that
built-in preset again reloads its original curve. Use `save_preset` to copy
those temporary settings to a user slot and select that slot.

Selecting a preset updates all gain controls together and preserves the enabled
state. Only one preset select may control an equalizer. Standard select options
apply; `graphic_equalizer_id` is optional when one equalizer is configured.

## Number controls

| Option | Description |
| --- | --- |
| `graphic_equalizer_id` | Optional ID of the equalizer speaker. |
| `type` | Required. `63hz`, `250hz`, `1khz`, `4khz`, `16khz`, or `preamp`. |

Band controls range from -12 to +12 dB. Preamp ranges from -36 to 0 dB. Controls
use steps of 0.5 dB, follow preset/action changes, and accept standard number
entity options. Only one number of each type may control an equalizer.

## Enable switch

The optional switch enables processing when ON and bypasses it when OFF.
`graphic_equalizer_id` is optional when one equalizer is configured. Standard
switch options apply. The default `restore_mode` is `DISABLED`, which preserves
the speaker's initial `enabled` setting. Preset selection does not turn the
switch on or off. Only one enable switch may control an equalizer.

## Actions

All action IDs are optional when one equalizer is configured. Use `id:` to
select between multiple equalizers. Gain, preset, band, and slot values can be
templated. In a band lambda, return an index from 0 to 4 in ascending frequency
order. Preset lambdas use indices from 0 to 7 in the table order above.

| Action | Configuration | Description |
| --- | --- | --- |
| `graphic_equalizer.enable` | Optional `id` | Enable processing. |
| `graphic_equalizer.disable` | Optional `id` | Bypass processing. |
| `graphic_equalizer.reset` | Optional `id` | Select Flat, including 0 dB manual preamp. Keeps user presets. |
| `graphic_equalizer.select_preset` | Required `preset` | Load a built-in or user preset. |
| `graphic_equalizer.set_band_gain` | Required `band` and `gain` | Set one band using its YAML frequency name and a gain from -12 to +12 dB. |
| `graphic_equalizer.set_preamp` | Required `preamp` | Set manual attenuation from -36 to 0 dB. |
| `graphic_equalizer.save_preset` | Required `slot` (1, 2, or 3) | Save current gains/preamp to a user slot and select it. |

For example, a template button can save the current curve to User 1:

```yaml
button:
  - platform: template
    name: Save Equalizer User 1
    on_press:
      - graphic_equalizer.save_preset:
          slot: 1
```

Or adjust settings in an existing automation:

```yaml
on_press:
  - graphic_equalizer.select_preset:
      preset: user_2
  - graphic_equalizer.set_band_gain:
      band: 63hz
      gain: 5dB
  - graphic_equalizer.set_preamp:
      preamp: -3dB
  - graphic_equalizer.enable:
```

## Conditions

| Condition | Description |
| --- | --- |
| `graphic_equalizer.is_enabled` | Processing is enabled, including when the curve is flat. |
| `graphic_equalizer.selected_preset` | The selected preset matches the required `preset` value. |

Condition IDs are optional when one equalizer is configured.

```yaml
condition:
  graphic_equalizer.selected_preset:
    preset: user_1
```

## Triggers

Configure these under the equalizer's `speaker:` entry:

| Trigger | Arguments | Description |
| --- | --- | --- |
| `on_gain` | `uint8_t band`, `float gain` | A gain changes. Band indices 0-4 follow ascending frequency; index 5 is preamp. Gain is in dB. |
| `on_preset` | `uint8_t preset` | The selected preset changes. Indices 0-7 follow the preset table. |
| `on_enabled` | `bool enabled` | Processing is enabled or bypassed. |

```yaml
speaker:
  - platform: graphic_equalizer
    id: equalized_speaker
    output_speaker: speaker_id
    on_preset:
      - logger.log:
          format: "Equalizer preset: %u"
          args: [preset]
```
