# ESPHome Components

External ESPHome components maintained by
[@catalin2402](https://github.com/catalin2402).

## Components

| Component | Purpose | 
| --- | --- | 
| [auriol_4ld5661](components/auriol_4ld5661/) | Auriol weather and rain sensor decoder |
| [cd405x](components/cd405x/) | CD405x analog multiplexer control |
| [climate_ir_hyundai](components/climate_ir_hyundai/) | Hyundai air-conditioner IR climate |
| [esp32_hosted_gpio](components/esp32_hosted_gpio/) | GPIO expander over ESP32 Hosted |
| [graphic_equalizer](components/graphic_equalizer/) | Five-band speaker equalizer with saved user presets |
| [hub75_icn2053](components/hub75_icn2053/) | ICN2053 HUB75 RGB matrix display |
| [lcd_base](components/lcd_base/) | Local override of base ESPHome LCD support | ESPHome base |
| [pt2258](components/pt2258/) | Six-channel volume controller |
| [pt2323](components/pt2323/) | Audio selector and processor |
| [generic_ev1527](components/generic_ev1527/) | EV1527 packet logger and binary sensors |
| [silvercrest](components/silvercrest/) | Silvercrest remote socket transmitter |
| [sound_reactive_lights](components/sound_reactive_lights/) | Audio-reactive control of existing lights |
| [status_light](components/status_light/) | Device status indication using monochrome or RGB lights |
| [tea5767](components/tea5767/) | FM radio tuner |
| [usb_audio_device](components/usb_audio_device/) | USB audio playback and microphone capture |
| [usb_media_control](components/usb_media_control/) | USB HID media controls |
| [xiron](components/xiron/) | Xiron weather sensor decoder |

Each component directory contains installation notes and a working YAML
example.

## Installation

Load only the components needed by a device:

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components:
      - auriol_4ld5661
      - generic_ev1527
```

The `lcd_base` and `remote_receiver` directories override components included
with ESPHome. They are documented here but intentionally retain upstream
ESPHome ownership.
