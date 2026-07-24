# ESPHome Components

External ESPHome components maintained by
[@catalin2402](https://github.com/catalin2402).

## Components

| Component | Purpose | Ownership |
| --- | --- | --- |
| [arduino_expander](components/arduino_expander/) | AVR board used as an I²C GPIO/ADC expander | Custom |
| [auriol_4ld5661](components/auriol_4ld5661/) | Auriol weather and rain sensor decoder | Custom |
| [cd405x](components/cd405x/) | CD405x analog multiplexer control | Custom |
| [climate_ir_hyundai](components/climate_ir_hyundai/) | Hyundai air-conditioner IR climate | Custom |
| [esp32_hosted_gpio](components/esp32_hosted_gpio/) | GPIO expander over ESP32 Hosted | Custom |
| [hub75_icn2053](components/hub75_icn2053/) | ICN2053 HUB75 RGB matrix display | Custom |
| [lcd_base](components/lcd_base/) | Local override of base ESPHome LCD support | ESPHome base |
| [pt2258](components/pt2258/) | Six-channel volume controller | Custom |
| [pt2323](components/pt2323/) | Audio selector and processor | Custom |
| [remote_receiver](components/remote_receiver/) | Local override of ESPHome's remote receiver | ESPHome base |
| [rf_remote](components/rf_remote/) | 24-bit RC Switch remote decoder | Custom |
| [silvercrest](components/silvercrest/) | Silvercrest remote socket transmitter | Custom |
| [tea5767](components/tea5767/) | FM radio tuner | Custom |
| [xiron_decoder](components/xiron_decoder/) | Xiron weather sensor decoder | Custom |

Each component directory contains installation notes and a working YAML
example.

## Installation

Load only the components needed by a device:

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components:
      - auriol_4ld5661
      - rf_remote
```

The `lcd_base` and `remote_receiver` directories override components included
with ESPHome. They are documented here but intentionally retain upstream
ESPHome ownership.
