# Optimised PCF8574 LCD

Standalone display platform for HD44780 character LCDs with a PCF8574 I2C
backpack. Writes only changed characters to reduce I2C traffic and visible
tearing. Supports text, formatted output, pages, custom characters, and large
clock digits.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components@testing
    components: [lcd_pcf8574_optimised]

i2c:
  sda: GPIO4
  scl: GPIO5

display:
  - platform: lcd_pcf8574_optimised
    id: main_display
    dimensions: 20x4
    address: 0x27
    update_interval: 1s
    lambda: |-
      it.print(0, 0, "Optimised LCD");
      it.printf(0, 1, "Uptime: %lu s", millis() / 1000UL);
```

## Configuration

| Option | Description |
| --- | --- |
| `id` | Optional display ID for lambdas and automation actions. |
| `dimensions` | Required character dimensions, such as `20x4` or `16x2`. Up to 64 columns and 4 rows. |
| `address` | PCF8574 I2C address. Defaults to `0x3F`. |
| `i2c_id` | I2C bus ID, needed when the bus cannot be selected automatically. |
| `update_interval` | Rendering interval. Defaults to `1s`. Only changed characters are transmitted. |
| `lambda` | Rendering lambda with the display available as `it`. Cannot be combined with `pages`. |
| `pages` | List of pages, each with an `id` and rendering `lambda`. The first page is selected at startup. |
| `auto_cycle_interval` | Interval for selecting the next page. Defaults to `0s` (disabled). Requires `pages`. |
| `user_characters` | Up to eight custom glyphs, each with a unique `position` from 0 to 7 and eight `data` values from 0 to 31. |

Rendering supports `print`, `printf`, `strftime`, `print_digit`, and
`print_big_time`. Coordinates are zero-based columns and rows. The large digit
font uses all eight custom character slots; conflicting `user_characters`
disable large digit and clock rendering.

The first refresh writes the full display. Later refreshes compare the rendered
frame with the last successfully written frame. Unchanged frames send no LCD
writes. The extra frame buffer uses 80 bytes for a 20x4 display.

## Actions

| Action | Purpose |
| --- | --- |
| `lcd_pcf8574_optimised.page.show` | Select a page by its page ID. Also accepts a lambda returning a page pointer. |
| `lcd_pcf8574_optimised.page.show_next` | Select the next page, wrapping to the first. Accepts a display ID. |
| `lcd_pcf8574_optimised.page.show_previous` | Select the previous page, wrapping to the last. Accepts a display ID. |
| `component.update` | Render and immediately flush changes to the specified display. |

Page selection takes effect at the next refresh. Follow it with
`component.update` when an immediate refresh is needed.
For example, with a page named `status_page`:

```yaml
on_press:
  - lcd_pcf8574_optimised.page.show: status_page
  - component.update: main_display
```

Page actions accept the ID directly or an `id:` mapping. Next/previous actions
can omit the display ID when there is exactly one compatible display.

In rendering lambdas, `clear()` clears the RAM buffer and `display()` flushes
its changes. `update()` clears the buffer, renders the active page or lambda,
and flushes it. `invalidate()` forces a full redraw on the next flush.
`backlight()` and `no_backlight()` control the backpack's backlight output.

## Conditions

| Condition | Fields | Meaning |
| --- | --- | --- |
| `lcd_pcf8574_optimised.is_displaying_page` | Optional display `id`, required `page_id` | The specified page is currently selected. |

With one display, the page ID can be supplied directly:

```yaml
condition:
  lcd_pcf8574_optimised.is_displaying_page: status_page
```

With multiple displays, use an explicit mapping:

```yaml
condition:
  lcd_pcf8574_optimised.is_displaying_page:
    id: main_display
    page_id: status_page
```

