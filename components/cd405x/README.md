# CD405x Multiplexer

Selects an analog signal path using GPIO address pins and an optional inhibit
pin. Two address pins select four channels; adding the third selects eight.
The inhibit pin can disconnect every channel without changing the selected
address.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [cd405x]

cd405x:
  id: mux
  pin_a: GPIO18
  pin_b: GPIO19
  pin_c: GPIO23
  pin_inhibit: GPIO25
  channel: 0
  inhibit: false
  on_channel:
    - lambda: |-
        ESP_LOGD("mux", "Selected channel: %d", channel);
  on_inhibit:
    - lambda: |-
        ESP_LOGD("mux", "Inhibited: %s", inhibited ? "yes" : "no");

button:
  - platform: template
    name: "Select Channel 3"
    on_press:
      - cd405x.select_channel:
          channel: 3

  - platform: template
    name: "Toggle Signal Path"
    on_press:
      - cd405x.toggle_inhibit:
```

## Configuration

| Option | Description |
| --- | --- |
| `id` | Optional hub ID. |
| `pin_a` | Required least significant address bit. |
| `pin_b` | Required second address bit. |
| `pin_c` | Optional third address bit. Enables channels 4-7. |
| `pin_inhibit` | Optional inhibit output. Logical high disconnects the signal paths. |
| `channel` | Startup channel. Defaults to 0. Range is 0-3 without `pin_c`, or 0-7 with it. |
| `inhibit` | Disconnect the signal paths at startup. Defaults to `false`. Setting `true` requires `pin_inhibit`. |
| `on_channel` | Automation triggered when the selected channel changes. |
| `on_inhibit` | Automation triggered when inhibit changes. Requires `pin_inhibit`. |

Configure more than one hub using a list. Pin options accept ESPHome's output
pin configuration, including inversion and supported GPIO expanders.

When `pin_inhibit` is configured, the component briefly disconnects the signal
paths while writing the address pins, then restores the requested inhibit
state. Selecting a channel while inhibited keeps the signal paths disconnected.
Without an inhibit pin, changing several address bits can briefly select an
intermediate channel.

## Actions

| Action | Additional fields | Purpose |
| --- | --- | --- |
| `cd405x.select_channel` | `channel` | Select an address without changing inhibit. |
| `cd405x.inhibit` | `inhibit`: boolean | Set the inhibit state. |
| `cd405x.enable` | None | Clear inhibit and connect the selected path. |
| `cd405x.disable` | None | Set inhibit and disconnect all paths. |
| `cd405x.toggle_inhibit` | None | Toggle inhibit. |

`channel` and `inhibit` accept lambdas. Channel numbers must be 0-3 without
`pin_c`, or 0-7 with it. Invalid runtime channels are ignored with a warning.
Inhibit actions need `pin_inhibit` to control the hardware; without it, the
component remains enabled and attempts to inhibit log a warning.

IDs are optional when there is exactly one CD405x hub. With multiple hubs,
include `id` in action mappings or use the hub ID directly for actions without
additional fields:

```yaml
on_press:
  - cd405x.select_channel:
      id: mux
      channel: 2
  - cd405x.disable: mux
```

With one hub, actions without additional fields use an empty block:

```yaml
on_press:
  - cd405x.disable:
  - cd405x.enable:
```

## Conditions

| Condition | Additional fields | Meaning |
| --- | --- | --- |
| `cd405x.selected_channel` | `channel` | The selected address matches, including while inhibited. |
| `cd405x.is_inhibited` | None | All signal paths have been disabled through inhibit. |
| `cd405x.is_enabled` | None | Inhibit is cleared, or no inhibit pin is configured. |

Conditions use the same optional ID rules as actions. The `channel` field
accepts a lambda. Use ESPHome's `not:` condition for the inverse.

```yaml
on_press:
  - if:
      condition:
        cd405x.selected_channel:
          channel: 3
      then:
        - cd405x.disable:
  - if:
      condition:
        cd405x.is_inhibited:
      then:
        - cd405x.enable:
```

## Triggers

| Trigger | Lambda arguments | When it fires |
| --- | --- | --- |
| `on_channel` | `int channel` | The selected address changes. |
| `on_inhibit` | `bool inhibited` | The requested inhibit state changes. |

Configure triggers under the hub, as shown in the first example. They do not
fire for initial YAML settings or unchanged assignments. The brief inhibit
pulse used while changing address pins does not fire `on_inhibit`.

States describe the GPIO settings requested by the component, rather than
feedback from the multiplexer or the connected analog signal.
