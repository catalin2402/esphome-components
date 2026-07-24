# Remote Receiver Override

This folder overrides ESPHome's built-in `remote_receiver` component. It
retains the standard receiver configuration and remote listener APIs while
including compatibility fixes used by the RF decoders in this repository.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [remote_receiver]

remote_receiver:
  id: rf_receiver
  pin: GPIO4
  tolerance: 50%
  filter: 250us
  idle: 4ms
  dump: rc_switch
```

All normal ESPHome options remain available, including `dump`, `tolerance`,
`filter`, `idle`, RMT clock resolution, DMA selection and receiver triggers.
Refer to the ESPHome remote receiver documentation for platform-specific
limits.

Because `remote_receiver` is part of base ESPHome, ownership remains with the
upstream ESPHome project.
