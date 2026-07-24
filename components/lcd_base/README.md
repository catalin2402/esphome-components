# LCD Base Override

This folder overrides ESPHome's built-in `lcd_base` implementation used by
character LCD platforms such as `lcd_gpio`. It is not configured as a
standalone platform.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [lcd_base]

display:
  - platform: lcd_gpio
    id: character_lcd
    dimensions: 16x2
    data_pins:
      - GPIO26
      - GPIO27
      - GPIO32
      - GPIO33
    enable_pin: GPIO14
    rs_pin: GPIO13
    lambda: |-
      it.print("Hello");
```

The override retains the normal ESPHome LCD configuration, including pages,
custom characters and the `display.page.show`, `display.page.show_next` and
`display.page.show_previous` actions.

Because `lcd_base` is part of base ESPHome, ownership remains with the
upstream ESPHome project.
