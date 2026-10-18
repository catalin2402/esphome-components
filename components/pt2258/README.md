# PT2258 Volume Controller

Controls a six-channel PT2258 volume controller over I2C. Provides master
volume, individual or paired channel volume, channel offsets, and global mute.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [pt2258]

i2c:
  sda: GPIO21
  scl: GPIO22

pt2258:
  id: volume_controller
  default_volume: 20

number:
  - platform: pt2258
    pt2258_id: volume_controller
    type: master
    name: "Master Volume"

  - platform: pt2258
    pt2258_id: volume_controller
    type: volume
    channel_a: 1
    channel_b: 2
    name: "Front Volume"

  - platform: pt2258
    pt2258_id: volume_controller
    type: offset
    channel_a: 3
    min_value: -20
    max_value: 20
    name: "Center Offset"

button:
  - platform: template
    name: "Mute"
    on_press:
      - pt2258.mute: volume_controller
  - platform: template
    name: "Unmute"
    on_press:
      - pt2258.unmute: volume_controller
```

## Hub configuration

| Option | Description |
| --- | --- |
| `id` | Optional hub ID. |
| `address` | I2C address. Defaults to `0x44`. |
| `i2c_id` | I2C bus ID, needed when the bus cannot be selected automatically. |
| `default_volume` | Startup volume for all channels, from 0 to 79. Defaults to 0. |

Volume 0 applies 79 dB attenuation; volume 79 applies no attenuation. Values
increase in 1 dB steps. The controller starts unmuted at the configured volume.
More than one hub can be configured as a list.

## Number configuration

| Option | Description |
| --- | --- |
| `pt2258_id` | Hub ID. Can be omitted when there is only one hub. |
| `type` | `master`, `volume`, or `offset`. Defaults to `master`. |
| `channel_a`, `channel_b` | Channels 1-6. At least one is required for `volume` and `offset`; neither is allowed for `master`. |
| `min_value` | Minimum selectable value. Defaults to 0; negative values down to -79 are allowed for `offset`. |
| `max_value` | Maximum selectable value, up to 79. Defaults to 79. Must exceed `min_value`. |
| `name` | Entity name. Standard ESPHome number options are supported. |

`master` adjusts all channels while preserving their offsets. `volume` sets the
selected channels to an absolute value. `offset` sets their difference from the
master volume. When both channels are configured, the same setting is applied
to both and the entity reports channel A.

The hardware output is limited to 0-79. Channel offsets are retained when this
limit is reached, so they apply again when the master returns within range.
Numbers update immediately when another number or an automation changes the
hub state; no `update_interval` is needed. Values describe the requested state,
since the controller does not provide volume readback.

## Actions

| Action | Purpose |
| --- | --- |
| `pt2258.mute` | Mute all channels without changing their volumes. |
| `pt2258.unmute` | Unmute all channels. |
| `pt2258.resend_data` | Reapply cached volumes and mute state. |
| `pt2258.set_mute` | Set global mute using a `mute` boolean. |
| `pt2258.toggle_mute` | Toggle global mute. |
| `pt2258.set_master_volume` | Set master `volume` (0-79). |
| `pt2258.set_channel_volume` | Set `volume` (0-79) for `channel` (1-6). |
| `pt2258.set_channel_offset` | Set `offset` (-79-79) from the master for `channel` (1-6). |

Actions without additional fields accept the hub ID directly, as in the
example, or as `id:` in a mapping. Actions with additional fields use a mapping; all additional fields accept
lambdas. For all actions and conditions, `id` can be omitted when exactly one
PT2258 hub is configured. With multiple hubs, specify `id` explicitly. Mute remains active when
volumes change.

```yaml
button:
  - platform: template
    name: "Set Listening Volume"
    on_press:
      - pt2258.set_master_volume:
          volume: 35
      - pt2258.set_channel_offset:
          channel: 3
          offset: -5
```

## Conditions

| Condition | Fields | Meaning |
| --- | --- | --- |
| `pt2258.is_muted` | Hub ID | Global mute is enabled. |
| `pt2258.volume_in_range` | `id`, optional `channel`, `min`, `max` | The cached volume is within the inclusive range. |

For `volume_in_range`, channel 0 means master and channels 1-6 mean individual
outputs. Defaults are `channel: 0`, `min: 0`, and `max: 79`. Channel and bounds
accept lambdas. Individual channel volumes are checked after limiting them to
the hardware range of 0-79. Use ESPHome's `not:` condition for the inverse.

```yaml
on_press:
  - if:
      condition:
        pt2258.is_muted: volume_controller
      then:
        - pt2258.unmute: volume_controller
  - if:
      condition:
        pt2258.volume_in_range:
          id: volume_controller
          min: 40
      then:
        - pt2258.set_master_volume:
            id: volume_controller
            volume: 30
```

## Triggers

Configure triggers under the `pt2258:` hub:

| Trigger | Lambda arguments | When it fires |
| --- | --- | --- |
| `on_volume` | `int channel`, `int volume` | A cached master or channel volume changes. Channel 0 is master. |
| `on_mute` | `bool muted` | Global mute changes. |

`on_volume` reports the stored value, which can extend outside 0-79 for channel
offsets; the physical output remains limited to 0-79. A master change can emit
one event for master and one for each affected channel. For example:

```yaml
pt2258:
  id: volume_controller
  on_volume:
    - lambda: |-
        ESP_LOGD("audio", "Channel %d volume %d", channel, volume);
  on_mute:
    - lambda: |-
        ESP_LOGD("audio", "Muted: %s", muted ? "yes" : "no");
```

Triggers describe requested settings, not hardware confirmation. They do not
fire for initial YAML settings, unchanged assignments, or `resend_data`.

With one hub, actions and conditions without other fields can use an empty
block:

```yaml
on_press:
  - pt2258.mute:
  - if:
      condition:
        pt2258.is_muted:
      then:
        - pt2258.unmute:
```
