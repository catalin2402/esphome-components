# Status Light

Displays device status using an existing light, including a WS2812 LED. RGB
lights use a different color for each status. Monochrome lights use the same
blink patterns without colors. Use a dedicated status light or expose a
controllable light whose normal state is restored after status indication.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [status_light]

light:
  - platform: esp32_rmt_led_strip
    id: device_status_light
    internal: true
    pin: GPIO1
    num_leds: 1
    chipset: WS2812
    channel_colors: GRB
    restore_mode: ALWAYS_OFF
    default_transition_length: 0s

status_light:
  light_id: device_status_light
```

Use the GPIO connected to your LED. For a monochrome LED, reference a binary or
monochromatic light instead.

## Configuration

| Option | Description |
| --- | --- |
| `light_id` | Required. ID of the light to control. |
| `id` | Optional. ID of the status light component. |
| `colors` | Optional. RGB colors for `error`, `warning`, `network`, and `api`. |

In this form, the referenced light is dedicated to status indication and turns
off when healthy. Use `internal: true` to keep it out of Home Assistant.

## Controllable light

To control the normal light state from Home Assistant or automations, replace
the top-level `status_light:` block with a second light entry:

```yaml
light:
  - platform: esp32_rmt_led_strip
    id: device_status_light
    internal: true
    pin: GPIO1
    num_leds: 1
    chipset: WS2812
    channel_colors: GRB
    restore_mode: ALWAYS_OFF
    default_transition_length: 0s

  - platform: status_light
    id: status_indicator
    name: "Status Indicator"
    light_id: device_status_light
    restore_mode: ALWAYS_OFF
    default_transition_length: 0s
```

The platform accepts the usual light options, including `name`, `id`,
`restore_mode`, transitions, effects, and automations. `light_id` references the
underlying light; its capabilities determine the controls available. Use either
configuration form for a given underlying light.

Active statuses temporarily override the selected state. When healthy, the
platform displays its current brightness and color, including any running
effect. Changes made while a status is active appear when it clears. Control
the exposed status light rather than the underlying light.

Status changes apply immediately, stop effects on the underlying light, and do
not publish each blink or save it to flash.

## Custom colors

Both configuration forms accept a `colors:` block:

```yaml
status_light:
  light_id: device_status_light
  colors:
    error:
      red: 100%
    warning:
      red: 100%
      green: 50%
    network:
      blue: 100%
    api:
      green: 100%
```

For `light: - platform: status_light`, place `colors:` in the platform entry,
alongside `light_id`. Each color accepts `red`, `green`, and `blue` as percentages
or values from `0.0` to `1.0`. Omitted channels within a configured color default
to zero. Omitted statuses keep their default colors: red for errors, yellow for
warnings, blue for network connection, and green for API connection. Colors do
not affect monochrome lights or blink timings.

## Status indications

| Status | RGB color | Blink pattern |
| --- | --- | --- |
| Error | Red | 150 ms on, 100 ms off |
| Connecting to the network | Blue | 250 ms on, 1250 ms off |
| Waiting for an API client | Green | 250 ms on, 1250 ms off |
| Warning | Yellow | 250 ms on, 1250 ms off |
| Healthy | Off, or the selected light state | No status blinking |

Statuses are listed in priority order. Errors and warnings follow ESPHome's
application status. Network indication applies when a configured network is
enabled but disconnected. API indication applies when `api:` is configured and
no native API client is connected. Connection indications return if a connection
is lost.
