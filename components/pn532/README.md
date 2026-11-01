# PN532 NFC Reader Override

Reads NFC tags using a PN532 over SPI or I2C. Supports tag detection, removal
automations, and MIFARE Classic decoding through components such as
`bambu_lab_tags`.

This folder overrides ESPHome's built-in `pn532` implementation shared by
`pn532_spi` and `pn532_i2c`. Configure one of those reader platforms rather
than a standalone `pn532:` entry.

## Override changes

- Adds the shared `mifare_classic` reader interface for Key A authentication
  and 16-byte block reads while a tag is selected.
- Passes Classic tags to an attached decoder before attempting normal NDEF
  reading. This lets `bambu_lab_tags` authenticate with its own sector keys.
- Loads `mifare_classic` automatically and exposes the reader interface to
  configuration code.

The override retains the normal reader configuration and tag automations.
Classic tags use normal NDEF reading when no decoder is attached. The
`pn532_spi`, `pn532_i2c`, and `nfc` components remain the built-in versions.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [mifare_classic, pn532]

spi:
  clk_pin: GPIO18
  miso_pin: GPIO19
  mosi_pin: GPIO23

pn532_spi:
  id: nfc_reader
  cs_pin: GPIO5
  update_interval: 1s
  on_tag:
    - logger.log:
        format: "Tag detected: %s"
        args: [x.c_str()]
  on_tag_removed:
    - logger.log:
        format: "Tag removed: %s"
        args: [x.c_str()]
```

Choose pins for your board and set the reader's hardware interface to match.
For I2C, replace the SPI configuration with:

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22

pn532_i2c:
  id: nfc_reader
  address: 0x24
  update_interval: 1s
```

## Reader configuration

| Option | Purpose |
| --- | --- |
| `id` | Reader ID used by tag decoders and binary sensors. |
| `cs_pin` | Required chip-select pin for SPI. |
| `address` | I2C address; defaults to `0x24`. |
| `update_interval` | Tag scan interval; defaults to `1s`. |
| `on_tag` | Runs when a new tag is detected. Provides UID string `x` and NFC object `tag`. |
| `on_tag_removed` | Runs when the detected tag is removed. Provides `x` and `tag`. |
| `on_finished_write` | Runs when the reader finishes a write task. |

## Bambu spool tags

Add `bambu_lab_tags` to the external component list and configure:

```yaml
bambu_lab_tags:
  reader_id: nfc_reader
```

Use the decoder's `on_tag` automation and optional sensors for spool metadata.
See [Bambu Lab Tags](../bambu_lab_tags/) for the available fields.

With a decoder attached, Classic tags use that decoder instead of normal NDEF
reading. Reader tag callbacks still provide the UID and tag type. Readers
without a decoder keep normal NFC behavior.
