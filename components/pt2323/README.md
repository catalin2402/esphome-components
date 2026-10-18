# PT2323 Audio Selector

Controls a PT2323 audio selector over I2C. Provides input selection, sound
enhancement, boost, and muting of individual channels or all channels.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [pt2323]

i2c:
  sda: GPIO21
  scl: GPIO22

pt2323:
  id: audio_selector
  input: 0
  enhance: false
  boost: false
  mute: false

select:
  - platform: pt2323
    pt2323_id: audio_selector
    name: "Audio Input"
    options:
      - Auxiliary
      - Bluetooth
      - Television
      - Media Player
      - Surround

switch:
  - platform: pt2323
    pt2323_id: audio_selector
    type: mute_all
    name: "Mute All"

  - platform: pt2323
    pt2323_id: audio_selector
    type: mute
    channel_a: 1
    channel_b: 2
    name: "Front Mute"

  - platform: pt2323
    pt2323_id: audio_selector
    type: enhance
    name: "Sound Enhancement"

  - platform: pt2323
    pt2323_id: audio_selector
    type: boost
    name: "Boost"
```

## Hub configuration

| Option | Description |
| --- | --- |
| `id` | Optional hub ID. |
| `address` | I2C address. Defaults to `0x4A`. |
| `i2c_id` | I2C bus ID, needed when the bus cannot be selected automatically. |
| `input` | Startup input index, from 0 to 4. Defaults to 0. |
| `enhance` | Enable sound enhancement at startup. Defaults to `false`. |
| `boost` | Enable boost at startup. Defaults to `false`. |
| `mute` | Mute all channels at startup. Defaults to `false`. |

More than one hub can be configured as a list.

## Select configuration

| Option | Description |
| --- | --- |
| `pt2323_id` | Hub ID. Can be omitted when there is only one hub. |
| `options` | Required list of 1-5 unique input names, in hardware input order. The first name represents input 0. |
| `name` | Entity name. Standard ESPHome select options are supported. |

Name the inputs according to your wiring. The list must include the hub's
startup input. If automations select an input outside the list, the select
cannot display a matching option. The select updates immediately after an
input action.

## Switch configuration

| Option | Description |
| --- | --- |
| `pt2323_id` | Hub ID. Can be omitted when there is only one hub. |
| `type` | Required. `enhance`, `boost`, `mute`, or `mute_all`. |
| `channel_a`, `channel_b` | Channels 1-6. At least one is required for `mute`; these options are not used by other types. |
| `name` | Entity name. Standard ESPHome switch options are supported. |

An enabled `enhance` or `boost` switch enables that feature. An enabled `mute`
switch mutes its selected channels; `mute_all` mutes every channel. If both
channels are configured, the switch changes both and reports channel A's mute
setting. Global mute takes precedence over individual channel settings;
unmuting globally restores the individual mute settings.

Entities update immediately when actions change the hub state; no
`update_interval` is needed. They report cached requested settings, since the
controller does not provide state readback. Hub startup settings are used on
boot.

## Actions

| Action | Additional fields | Purpose |
| --- | --- | --- |
| `pt2323.select_input` | `input`: 0-4 | Select an input. |
| `pt2323.set_enhance` | `enhance`: boolean | Enable or disable enhancement. |
| `pt2323.set_boost` | `boost`: boolean | Enable or disable boost. |
| `pt2323.mute` | None | Mute all channels. |
| `pt2323.unmute` | None | Unmute globally and restore individual mute settings. |
| `pt2323.mute_channel` | `channel`: 1-6 | Mute one channel. |
| `pt2323.unmute_channel` | `channel`: 1-6 | Unmute one channel. |
| `pt2323.resend_data` | None | Reapply the input, features, and mute settings. |
| `pt2323.set_mute` | `mute`: boolean | Set global mute. |
| `pt2323.toggle_mute` | None | Toggle global mute. |
| `pt2323.set_channel_mute` | `channel`: 1-6, `mute`: boolean | Set individual channel mute. |

Actions with additional fields use a mapping and accept lambdas for those
fields. Actions without additional fields accept the hub ID directly or as an
`id:` mapping. For all actions and conditions, `id` can be omitted when exactly
one PT2323 hub is configured. With multiple hubs, specify `id` explicitly.

```yaml
button:
  - platform: template
    name: "Select Bluetooth"
    on_press:
      - pt2323.select_input:
          input: 1
```

## Conditions

| Condition | Additional fields | Meaning |
| --- | --- | --- |
| `pt2323.is_muted` | None | Global mute is enabled. |
| `pt2323.selected_input` | `input`: 0-4 | The selected input matches. |
| `pt2323.is_enhanced` | None | Enhancement is enabled. |
| `pt2323.is_boosted` | None | Boost is enabled. |
| `pt2323.is_channel_muted` | `channel`: 1-6 | The channel is muted individually or by global mute. |

Conditions without additional fields accept a hub ID directly or an `id:`
mapping. Other conditions use a mapping and accept lambdas for the additional fields. Conditions inspect cached requested settings. Use
ESPHome's `not:` condition for the inverse.

```yaml
on_press:
  - if:
      condition:
        pt2323.selected_input:
          input: 1
      then:
        - pt2323.toggle_mute: audio_selector
```

## Triggers

Configure triggers under the `pt2323:` hub:

| Trigger | Lambda arguments | When it fires |
| --- | --- | --- |
| `on_input` | `int input` | Selected input changes. |
| `on_enhance` | `bool enabled` | Enhancement changes. |
| `on_boost` | `bool enabled` | Boost changes. |
| `on_mute` | `bool muted` | Global mute changes. |
| `on_channel_mute` | `int channel`, `bool muted` | An individual channel mute setting changes. |

`on_channel_mute` reports the individual setting, independently of global mute;
global mute changes emit `on_mute`. Channel numbers are 1-6.

```yaml
pt2323:
  id: audio_selector
  on_input:
    - lambda: |-
        ESP_LOGD("audio", "Input: %d", input);
  on_mute:
    - lambda: |-
        ESP_LOGD("audio", "Muted: %s", muted ? "yes" : "no");
  on_channel_mute:
    - lambda: |-
        ESP_LOGD("audio", "Channel %d muted: %s", channel, muted ? "yes" : "no");
```

Triggers describe requested settings, not hardware confirmation. They do not
fire for initial YAML settings, unchanged assignments, or `resend_data`.

With one hub, actions and conditions without other fields can use an empty
block:

```yaml
on_press:
  - pt2323.toggle_mute:
  - if:
      condition:
        pt2323.is_muted:
      then:
        - pt2323.unmute:
```
