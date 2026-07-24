# HUB75 ICN2053 Display

Drives HUB75 RGB LED matrices based on ICN2053 shift-register drivers using
the ESP32 I²S parallel peripheral. The component implements ESPHome's display
API and provides an optional brightness number.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [hub75_icn2053]

display:
  - platform: hub75_icn2053
    id: matrix
    width: 64
    height: 32
    chain_length: 1
    brightness: 128
    D_pin: GPIO17
    lambda: |-
      it.fill(Color::BLACK);

number:
  - platform: hub75_icn2053
    hub75_icn2053_id: matrix
    name: Matrix brightness
```

The default pin map is R1=25, G1=26, B1=27, R2=14, G2=12, B2=13, A=23,
B=19, C=5, LAT=4, OE=15 and CLK=16. Configure `D_pin` and `E_pin` when
required by the panel scan rate. Brightness ranges from 0 to 255.

This is a low-level ESP32 driver. Verify the complete pin map, voltage levels
and power supply requirements for the specific panel before connecting it.
