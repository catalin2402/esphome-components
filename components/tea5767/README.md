# TEA5767 FM Radio

Controls a TEA5767 FM tuner over I2C. Supports frequency tuning, mute, forced
mono, soft mute, stereo noise canceling, and high-cut control. Optional sensors
report the tuner frequency, signal level, and received mono/stereo indication.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [tea5767]

i2c:
  sda: GPIO21
  scl: GPIO22

tea5767:
  id: radio
  frequency: 99.5MHz
  update_interval: 5s
  on_frequency:
    - lambda: |-
        ESP_LOGD("radio", "Requested frequency: %.1f MHz", frequency / 1e6);
  on_stereo:
    - lambda: |-
        ESP_LOGD("radio", "Stereo reception: %s", stereo ? "yes" : "no");

sensor:
  - platform: tea5767
    tea5767_id: radio
    type: frequency
    name: "Radio Frequency"

  - platform: tea5767
    tea5767_id: radio
    type: signal_level
    name: "Radio Signal Level"

binary_sensor:
  - platform: tea5767
    tea5767_id: radio
    type: mono
    name: "Radio Mono Reception"

switch:
  - platform: tea5767
    tea5767_id: radio
    type: mute
    name: "Radio Mute"

button:
  - platform: template
    name: "Tune Radio"
    on_press:
      - tea5767.set_frequency:
          frequency: 101.2MHz
  - platform: template
    name: "Toggle Radio Mute"
    on_press:
      - tea5767.toggle_mute:
```

## Hub configuration

| Option | Description |
| --- | --- |
| `id` | Optional hub ID. |
| `address` | I2C address. Defaults to `0x60`. |
| `i2c_id` | I2C bus ID, needed when the bus cannot be selected automatically. |
| `frequency` | Startup tuning frequency. Defaults to `87.5MHz`. |
| `in_japan` | Enable the Japan band setting and tuning below 87.5 MHz. Defaults to `false`. |
| `mute` | Start with audio muted. Defaults to `false`. |
| `mono` | Force mono audio. Defaults to `false`. |
| `soft_mute` | Enable signal-dependent soft mute. Defaults to `false`. |
| `stereo_noise_canceling` | Enable signal-dependent mono/stereo blending. Defaults to `false`. |
| `high_cut_control` | Enable signal-dependent high-frequency reduction. Defaults to `false`. |
| `update_interval` | Hardware status polling interval. Defaults to `5s`. |

Frequencies accept units such as `99.5MHz`, or a number in Hz. The normal
configuration accepts 87.5-108 MHz. With `in_japan: true`, preset tuning accepts
76-108 MHz, including frequencies above the traditional 76-91 MHz Japan band.
Out-of-range runtime frequencies are ignored with a warning. More than one hub
can be configured as a list.

## Sensor configuration

Create measurements under `sensor:` with `platform: tea5767`:

| Option | Description |
| --- | --- |
| `tea5767_id` | Hub ID. Can be omitted when there is only one hub. |
| `type` | Required. `frequency` or `signal_level`. |
| `name` | Entity name. Standard ESPHome sensor options are supported. |

`frequency` reports the frequency calculated from the tuner's measured PLL
value, in Hz. It may differ slightly from the requested frequency.
`signal_level` reports the raw signal-strength level from 0 to 15; this is not
a dBm measurement.

## Binary sensor configuration

Create reception indicators under `binary_sensor:` with `platform: tea5767`:

| Option | Description |
| --- | --- |
| `tea5767_id` | Hub ID. Can be omitted when there is only one hub. |
| `type` | Required. `mono` or `stereo`. |
| `name` | Entity name. Standard ESPHome binary sensor options are supported. |

`mono` is ON when the tuner reports mono reception. `stereo` is ON when it
reports stereo reception. These describe the received indication; the `mono`
switch controls the separate forced-mono setting. Neither publishes an initial
reception state before a successful hardware read.

Measurement entities use the hub's status reads after startup and tuning, and
at its `update_interval`; they do not poll the tuner independently.

## Switch configuration

| Option | Description |
| --- | --- |
| `tea5767_id` | Hub ID. Can be omitted when there is only one hub. |
| `type` | Required. `mute`, `mono`, `soft_mute`, `stereo_noise_canceling`, or `high_cut_control`. |
| `name` | Entity name. Standard ESPHome switch options are supported. |

An enabled switch enables its selected setting. Switches update immediately
when actions change the same setting; they do not use `update_interval`.
Startup values come from the hub configuration. Switch states describe the
requested settings rather than confirmation of a successful I2C write.

## Actions

| Action | Additional fields | Purpose |
| --- | --- | --- |
| `tea5767.set_frequency` | `frequency` | Tune to a frequency. |
| `tea5767.set_mute` | `mute`: boolean | Set audio mute. |
| `tea5767.mute` | None | Mute audio. |
| `tea5767.unmute` | None | Unmute audio. |
| `tea5767.toggle_mute` | None | Toggle mute. |
| `tea5767.set_mono` | `mono`: boolean | Enable or disable forced mono. |
| `tea5767.set_soft_mute` | `soft_mute`: boolean | Enable or disable soft mute. |
| `tea5767.set_stereo_noise_canceling` | `stereo_noise_canceling`: boolean | Enable or disable stereo noise canceling. |
| `tea5767.set_high_cut_control` | `high_cut_control`: boolean | Enable or disable high-cut control. |
| `tea5767.resend_data` | None | Reapply all requested settings and schedule a status read. |

All additional fields accept lambdas. Frequency lambdas return Hz. IDs are
optional when exactly one hub is configured; with multiple hubs, include `id`
in the mapping or use the hub ID directly for actions without additional fields:

```yaml
on_press:
  - tea5767.set_frequency:
      id: radio
      frequency: 99.5MHz
  - tea5767.mute: radio
```

Tuning preserves mute and noise-control settings. With one hub, actions without
additional fields use an empty block, as shown for `toggle_mute` above.

## Conditions

| Condition | Additional fields | Meaning |
| --- | --- | --- |
| `tea5767.is_muted` | None | Audio mute is requested. |
| `tea5767.is_mono` | None | Forced mono is requested. |
| `tea5767.is_soft_muted` | None | Soft mute is enabled. |
| `tea5767.stereo_noise_canceling_enabled` | None | Stereo noise canceling is enabled. |
| `tea5767.high_cut_control_enabled` | None | High-cut control is enabled. |
| `tea5767.frequency_in_range` | Optional `min`, `max` | Requested frequency is within the inclusive range. Defaults to 76-108 MHz. |
| `tea5767.is_stereo` | None | The last valid hardware status reports stereo reception. |
| `tea5767.signal_level_in_range` | Optional `min`, `max` | The last valid signal level is within the inclusive range. Defaults to 0-15. |

Conditions use the same optional ID rules as actions. Range limits accept
lambdas; frequency limits use Hz or frequency units. Hardware-status conditions
are false until a successful read, after a failed read, or while waiting for
status after tuning. Other conditions inspect cached requested settings. Use
ESPHome's `not:` condition for the inverse.

```yaml
on_press:
  - if:
      condition:
        tea5767.is_muted:
      then:
        - tea5767.unmute:
  - if:
      condition:
        tea5767.signal_level_in_range:
          min: 8
      then:
        - logger.log: "Strong radio signal"
```

## Triggers

Configure triggers under the `tea5767:` hub:

| Trigger | Lambda argument | When it fires |
| --- | --- | --- |
| `on_frequency` | `uint64_t frequency` | Requested frequency changes, in Hz. |
| `on_mute` | `bool enabled` | Requested audio mute changes. |
| `on_mono` | `bool enabled` | Forced mono changes. |
| `on_soft_mute` | `bool enabled` | Soft mute changes. |
| `on_stereo_noise_canceling` | `bool enabled` | Stereo noise canceling changes. |
| `on_high_cut_control` | `bool enabled` | High-cut control changes. |
| `on_stereo` | `bool stereo` | A valid status read first reports reception, or the mono/stereo indication changes. |
| `on_level` | `uint8_t level` | A valid status read first reports signal level, or that level changes. |

Requested-setting triggers do not fire for initial YAML settings, unchanged
assignments, or resends. They report the requested change, even if an I2C write
fails. Reception triggers use measured status and report the first valid sample
again after tuning or recovery from a read failure. A resend alone does not
repeat unchanged measured values.
