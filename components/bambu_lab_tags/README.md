# Bambu Lab Tags

Reads filament information from Bambu Lab spool tags and publishes optional
text and numeric sensors. Supports PN532 readers over SPI or I2C. Requires an
ESP32 using the ESP-IDF framework.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [mifare_classic, pn532, bambu_lab_tags]

spi:
  clk_pin: GPIO36
  miso_pin: GPIO37
  mosi_pin: GPIO35

pn532_spi:
  id: spool_reader
  cs_pin: GPIO34
  update_interval: 200ms

bambu_lab_tags:
  id: spool_decoder
  reader_id: spool_reader
  on_tag:
    - logger.log:
        format: "%s: %s, RGBA %s"
        args:
          - spool.material_id.c_str()
          - spool.detailed_filament_type.c_str()
          - spool.color.c_str()

text_sensor:
  - platform: bambu_lab_tags
    bambu_lab_tags_id: spool_decoder
    type: MATERIAL_ID
    name: Spool Material ID

  - platform: bambu_lab_tags
    bambu_lab_tags_id: spool_decoder
    type: DETAILED_FILAMENT_TYPE
    name: Spool Filament

  - platform: bambu_lab_tags
    bambu_lab_tags_id: spool_decoder
    type: COLOR
    name: Spool Color Name

sensor:
  - platform: bambu_lab_tags
    bambu_lab_tags_id: spool_decoder
    type: WEIGHT
    name: Spool Nominal Weight

  - platform: bambu_lab_tags
    bambu_lab_tags_id: spool_decoder
    type: MIN_NOZZLE_TEMPERATURE
    name: Spool Minimum Nozzle Temperature

  - platform: bambu_lab_tags
    bambu_lab_tags_id: spool_decoder
    type: MAX_NOZZLE_TEMPERATURE
    name: Spool Maximum Nozzle Temperature
```

For I2C, replace the SPI bus and `pn532_spi` entry with an `i2c` bus and
`pn532_i2c`. Set `reader_id` to that reader's ID. Choose pins for your board.

For a local installation, replace the GitHub source with:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [mifare_classic, pn532, bambu_lab_tags]
```

## Hub configuration

| Option | Purpose |
| --- | --- |
| `id` | Optional ID for referencing this decoder from sensors. |
| `reader_id` | Optional with one compatible reader; required with multiple readers. Each reader supports one decoder. |
| `on_tag` | Runs after a complete successful reading. Provides `const Spool &spool`. |

With one compatible reader, the hub can be configured as:

```yaml
bambu_lab_tags:
```

Multiple decoders can use separate readers. Sensor platforms accept an optional
`bambu_lab_tags_id`; specify it when more than one decoder is configured.
All sensor entries are optional. Values remain at the last successful reading
when a tag is removed or a subsequent read fails.

## Text sensors

Each entry requires `type` and accepts the standard text sensor options.
Type values are case-insensitive.

| Type | Value |
| --- | --- |
| `UID` | Tag UID as hexadecimal text. |
| `MATERIAL_ID` | Material identifier stored on the tag. |
| `VARIANT_ID` | Material variant identifier. |
| `FILAMENT_TYPE` | Filament family, such as PLA. |
| `DETAILED_FILAMENT_TYPE` | Detailed filament name stored on the tag. |
| `COLOR` | English color name, such as Jade White; eight hexadecimal RGBA digits for unlisted variants. |
| `PRODUCTION_DATE` | Production date text stored on the tag, without timezone conversion. |

## Numeric sensors

Each entry requires `type` and accepts the standard sensor options.
Type values are case-insensitive.

| Type | Value |
| --- | --- |
| `WEIGHT` | Nominal spool filament weight in grams. |
| `DRYING_TEMPERATURE` | Recommended drying temperature in degrees Celsius. |
| `DRYING_TIME` | Recommended drying duration in hours. |
| `BED_TEMPERATURE` | Recommended bed temperature in degrees Celsius. |
| `MIN_NOZZLE_TEMPERATURE` | Minimum nozzle temperature in degrees Celsius. |
| `MAX_NOZZLE_TEMPERATURE` | Maximum nozzle temperature in degrees Celsius. |

## Tag automations

`on_tag` runs once when a spool is detected and read successfully. The `spool`
argument uses lowercase field names, such as `spool.uid` and `spool.color`.
Copy any values needed after the
automation completes. Unknown material IDs remain usable.

Use the reader's `on_tag_removed` for removal automations. Remove and present
a tag again to retry a failed read.

A reader assigned to this component reads Bambu spool information from Classic
tags. Its normal Classic NDEF reading is replaced; ordinary Classic tags can
produce authentication warnings. Other readers retain their normal behavior.

The reported weight is the nominal filament weight, not the remaining amount.
The color sensor reports the primary color. Tag authenticity is not verified.

Color names are matched by RFID variant ID and material ID using an embedded
[catalogue](https://github.com/piitaya/bambu-filaments). No network lookup is
required. The catalogue is a snapshot; newer or unlisted variants report
the RGBA value instead. `spool.color_name` is also available in tag automations.
