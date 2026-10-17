# HUB75 ICN2053

ESPHome display component for HUB75 LED panels with ICN2053 drivers.
Requires an original ESP32 with the ESP-IDF framework.

## Example

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
```

Set the pins to match your controller wiring. All pins above are required
except `d_pin` for panels up to 16 pixels high. Panels taller than 32 pixels
also require `e_pin`. Panel width must be a multiple of 16; height must be
even and between 2 and 64.

For multiple panels, set `layout_rows`, `layout_cols` and `layout`. The
default `HORIZONTAL` layout supports one row. Grid layouts are
`TOP_LEFT_DOWN`, `TOP_RIGHT_DOWN`, `BOTTOM_LEFT_UP` and `BOTTOM_RIGHT_UP`;
add `_ZIGZAG` to keep every panel upright instead of rotating alternate
rows 180 degrees. All panels share a single data chain.

`brightness` ranges from 0 to 255 (default: 128). Change it with the
templatable `hub75_icn2053.set_brightness` action using `id` and `brightness`,
or call `id(matrix).set_brightness(128)` in a lambda.

`clock_speed` accepts `1MHZ` (default), `5MHZ`, `8MHZ`, `10MHZ` or `20MHZ`.
`clock_phase` defaults to false. `gamma_correct` accepts `LINEAR` (default),
`CIE1931` or `GAMMA_2_2`.

Standard display lambdas, pages and rotation are supported. For LVGL, set
`update_interval: never` and `auto_clear_enabled: false`.

Only one display output is supported. The driver reserves I2S1 and uses a
single RGB565 framebuffer. `double_buffer: true`, `bit_depth`,
`min_refresh_rate`, `latch_blanking` and scan wiring other than
`STANDARD_TWO_SCAN` are unsupported.
