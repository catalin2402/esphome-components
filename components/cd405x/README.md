# CD405x Multiplexer

Controls CD4051/CD4052/CD4053-style analog multiplexers through their address
pins and optional inhibit pin.

## Example

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

button:
  - platform: template
    name: Select multiplexer channel 3
    on_press:
      - cd405x.activate_channel:
          id: mux
          channel: 3

switch:
  - platform: template
    name: Multiplexer inhibit
    turn_on_action:
      - cd405x.inhibit:
          id: mux
          inhibit: true
    turn_off_action:
      - cd405x.inhibit:
          id: mux
          inhibit: false
```

`pin_a` and `pin_b` are required. `pin_c` enables channels 0–7 and
`pin_inhibit` enables the inhibit action. Both action values are templatable.
