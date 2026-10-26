# HUB75 ICN2053

Controls HUB75 LED matrix panels with ICN2053 drivers. Supports display lambdas,
pages, rotation, chained panel layouts and LVGL. A number entity lets you adjust
brightness from Home Assistant.

Requires an original ESP32 using the ESP-IDF framework. One display output is
supported; multiple panels can share its data chain.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [hub75_icn2053]

esp32:
  board: esp32dev
  framework:
    type: esp-idf

display:
  - platform: hub75_icn2053
    id: matrix
    panel_width: 64
    panel_height: 32
    brightness: 128
    r1_pin: GPIO25
    g1_pin: GPIO26
    b1_pin: GPIO27
    r2_pin: GPIO14
    g2_pin: GPIO12
    b2_pin: GPIO13
    a_pin: GPIO23
    b_pin: GPIO19
    c_pin: GPIO5
    d_pin: GPIO17
    lat_pin: GPIO4
    oe_pin: GPIO15
    clk_pin: GPIO16
    lambda: |-
      it.rectangle(0, 0, it.get_width(), it.get_height(), Color(255, 0, 0));

number:
  - platform: hub75_icn2053
    name: "Matrix Brightness"
    display_id: matrix
```

Choose pins to match your controller wiring.

## Display configuration

| Option | Purpose |
| --- | --- |
| `id` | Identifies the display for drawing, automations and the brightness number. |
| `panel_width` | Required width of each panel in pixels; must be a multiple of 16. |
| `panel_height` | Required height of each panel in pixels; must be even and between 2 and 64. |
| `r1_pin`, `g1_pin`, `b1_pin` | Required RGB data pins for the upper half of the panel. |
| `r2_pin`, `g2_pin`, `b2_pin` | Required RGB data pins for the lower half of the panel. |
| `a_pin`, `b_pin`, `c_pin` | Required row address pins. |
| `d_pin` | Row address pin required for panels taller than 16 pixels. |
| `e_pin` | Row address pin required for panels taller than 32 pixels. |
| `lat_pin` | Required latch pin. |
| `oe_pin` | Required output enable pin. |
| `clk_pin` | Required clock pin. |
| `brightness` | Startup brightness from 0 (off) to 255 (maximum). Default: 128. |
| `gamma_correct` | Brightness curve: `LINEAR` (default), `CIE1931` or `GAMMA_2_2`. |
| `clock_speed` | Panel clock: `1MHZ` (default), `5MHZ`, `8MHZ`, `10MHZ` or `20MHZ`. |
| `clock_phase` | Inverts the panel clock phase when true. Default: false. |
| `layout_rows` | Number of panel rows. Default: 1. |
| `layout_cols` | Number of panel columns. Default: 1. |
| `layout` | Arrangement and wiring direction of the panel chain. Default: `HORIZONTAL`. |
| `rotation` | Display rotation: 0, 90, 180 or 270 degrees. Default: 0. |
| `lambda`, `pages` | Standard ESPHome display drawing configuration. |
| `update_interval` | How often the display redraws. Default: 1s. Use `never` with LVGL. |
| `auto_clear_enabled` | Standard display clearing behavior. Set false with LVGL. |

Pins must be unique, non-inverted, push-pull GPIO outputs. The display reserves
I2S1 and uses a single RGB565 framebuffer. `shift_driver` accepts `ICN2053`;
`scan_wiring` accepts `STANDARD_TWO_SCAN`. `double_buffer` can only be false.
`bit_depth`, `latch_blanking` and `min_refresh_rate` are not configurable.

### Panel layouts

`HORIZONTAL` connects panels in a single row. For grids, use `TOP_LEFT_DOWN`,
`TOP_RIGHT_DOWN`, `BOTTOM_LEFT_UP` or `BOTTOM_RIGHT_UP`, depending on where the
chain starts. These layouts rotate alternate panel rows by 180 degrees.

Add `_ZIGZAG` to any grid layout to keep every panel upright. Grid layouts
require more than one row; `_ZIGZAG` layouts also require more than one column.
All panels share a single data chain and have the same dimensions.

### LVGL

When using this display with LVGL, replace its drawing lambda or pages with:

```yaml
update_interval: never
auto_clear_enabled: false
```

Configure drawing and rotation through LVGL.

## Brightness number

The `hub75_icn2053` number platform controls display brightness from 0 to 255
in steps of 1. It follows changes made through actions or lambdas without
polling. At startup, it reports the display's configured `brightness` value.

`display_id` selects the display and can be omitted when it resolves
unambiguously. Standard number options such as `name`, `id`, `icon` and
`entity_category` are supported.

## Actions

`hub75_icn2053.set_brightness` changes brightness. `brightness` accepts a value
from 0 to 255 or a lambda. Runtime values are limited to that range.

```yaml
- hub75_icn2053.set_brightness:
    id: matrix
    brightness: 200
```

The `id` is optional when the display resolves unambiguously. A single value
can also be used:

```yaml
- hub75_icn2053.set_brightness: 128
```

In a lambda, use `id(matrix).set_brightness(128)` and
`id(matrix).get_brightness()`.

## Conditions

| Condition | Purpose |
| --- | --- |
| `hub75_icn2053.is_ready` | True after successful display initialization and while the display has not failed or shut down. |
| `hub75_icn2053.brightness_in_range` | Checks the current brightness against inclusive `min` and `max` values. Defaults: 0 and 255. |

Both conditions accept an optional `id`.

```yaml
- if:
    condition:
      hub75_icn2053.is_ready:
    then:
      - hub75_icn2053.set_brightness: 200

- if:
    condition:
      hub75_icn2053.brightness_in_range:
        min: 1
        max: 127
    then:
      - logger.log: "Display is dimmed"
```

## Triggers

Configure these on the display entry:

| Trigger | Purpose |
| --- | --- |
| `on_ready` | Runs after initialization and the first successful display transfer. |
| `on_error` | Runs when initialization or a display transfer fails. The display is marked failed. |
| `on_brightness` | Runs when brightness changes after setup. Provides the integer variable `brightness` (0-255). Startup configuration and unchanged values do not trigger it. |

```yaml
on_ready:
  - logger.log: "Matrix ready"
on_error:
  - logger.log: "Matrix failed"
on_brightness:
  - logger.log:
      format: "Matrix brightness: %d"
      args: [brightness]
```
