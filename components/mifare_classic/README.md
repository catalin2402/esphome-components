# MIFARE Classic

Provides a shared interface for authenticating and reading MIFARE Classic tags.
Used by compatible readers and tag decoders such as `bambu_lab_tags`.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [mifare_classic, pn532, bambu_lab_tags]

spi:
  clk_pin: GPIO18
  miso_pin: GPIO19
  mosi_pin: GPIO23

pn532_spi:
  id: spool_reader
  cs_pin: GPIO5

bambu_lab_tags:
  reader_id: spool_reader
```

## Configuration

Loaded automatically by components that use it. No separate `mifare_classic:`
entry is required. Configure the reader and decoder instead.

Currently supported readers are PN532 over SPI and I2C. See
[PN532](../pn532/) for reader options and [Bambu Lab Tags](../bambu_lab_tags/)
for spool sensors and automations.
