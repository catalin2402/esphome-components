# TinyUSB Audio

Connect the native USB OTG port to a Windows PC to use the ESP32 as a USB Audio
Class 2 device. Windows playback goes to a `speaker_source` media player, and
an optional ESPHome microphone appears as a Windows recording input. Requires
ESP-IDF and ESPHome 2026.11.0b4 or newer.

Supported variants are ESP32-S2, ESP32-S3, ESP32-P4, and ESP32-S31.
On ESP32-P4, connect to the high-speed USB OTG port. ESP32-S31 requires
ESP-IDF 6.1 or newer.

## Example

This ESP32-S3 example uses two I²S buses so playback and capture can use
different sample rates. On ESP32-S2, use a single full-duplex I²S bus with
matching speaker and microphone clock settings, or configure playback only.

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [tinyusb_audio]

logger:
  hardware_uart: UART0

tinyusb:
  usb_product_str: ESPHome USB Audio

tinyusb_audio:
  playback:
    sample_rate: 44100
    bits_per_sample: 24
  capture:
    microphone: i2s_mic
    sample_rate: 48000
    bits_per_sample: 16

i2s_audio:
  - id: output_bus
    i2s_lrclk_pin: GPIO4
    i2s_bclk_pin: GPIO5
  - id: input_bus
    i2s_lrclk_pin: GPIO8
    i2s_bclk_pin: GPIO9

speaker:
  - platform: i2s_audio
    id: i2s_speaker
    i2s_audio_id: output_bus
    i2s_dout_pin: GPIO6
    dac_type: external
    channel: stereo
    sample_rate: 44100
    bits_per_sample: 24bit
    mclk_multiple: 384

microphone:
  - platform: i2s_audio
    id: i2s_mic
    i2s_audio_id: input_bus
    i2s_din_pin: GPIO10
    adc_type: external
    channel: left
    sample_rate: 48000
    bits_per_sample: 32bit

media_source:
  - platform: tinyusb_audio
    id: usb_source

media_player:
  - platform: speaker_source
    name: USB Audio
    media_pipeline:
      speaker: i2s_speaker
      sources: usb_source
      num_channels: 2
      format: NONE
```

USB audio starts automatically when audible Windows playback begins and
returns to idle after 750 ms of silence. Set `playback.keep_alive: true` to
select USB as soon as Windows opens the stream and keep the player in
`PLAYING` through silence and host pause. If you switch sources with this option, USB can
automatically take over again when Windows closes and reopens the stream.
To select USB explicitly while the stream is open, play `usb://current`.
Omit `capture:` to expose only the playback endpoint.

Changes to the Windows USB playback device's master volume or mute update the
ESPHome media player. Application-specific mixer levels are handled by Windows.

## Media keys

Add `tinyusb_keyboard:` to expose a keyboard alongside the USB audio interfaces.
The USB media source forwards play/pause, next, and previous commands to the
connected computer. For GPIO buttons or other automations, use the
`tinyusb_keyboard.media_press` and `tinyusb_keyboard.media_release` actions.

```yaml
tinyusb_keyboard:
  id: usb_keyboard

# Example action list for a volume-up button
# on_press:
#   - tinyusb_keyboard.media_press:
#       id: usb_keyboard
#       usage: 0xE9  # Volume up
#   - delay: 50ms
#   - tinyusb_keyboard.media_release: usb_keyboard
```

## Configuration

| Option | Description |
| --- | --- |
| `id` | Optional hub ID. |
| `tinyusb_id` | Optional ID of the TinyUSB hub. USB identity and connection automations are configured under `tinyusb:`. |
| `playback.sample_rate` | Stereo playback rate in integer Hz. Inferred from the media player pipeline or its speaker. Falls back to 48000. |
| `playback.bits_per_sample` | Playback PCM depth. Inferred from the pipeline's speaker. Supports 16, 24, and 32 bits. Falls back to 16. |
| `playback.keep_alive` | Select USB playback when Windows opens the stream and keep it selected through silence. Defaults to `false`. |
| `capture.microphone` | Required microphone ID when capture is enabled. |
| `capture.channels` | Set to `2` for stereo capture from a microphone configured with two channels. Defaults to `1`, which uses the microphone's first channel. |
| `capture.sample_rate` | Capture rate in integer Hz. Inferred from the microphone, with a fallback of 48000. An explicit rate must match the microphone. |
| `capture.bits_per_sample` | Capture PCM depth. Inferred from the microphone, with a fallback of 16. Supports 16, 24, and 32 bits. ESPHome converts samples to the selected depth. |
| `media_source.tinyusb_audio_id` | Optional hub ID on the media source platform. Configure one media source per hub. |

Both rates must be between 1000 and 192000 Hz and fit your chip's USB packet
and FIFO limits. The USB formats are fixed when the firmware is built. Set the
options explicitly to override inference. Higher rates may require a lower
depth. The speaker pipeline
must accept the playback format. Use a hardware UART for logging if the board
shares the native USB port with the default logger. USB host mode and
`usb_cdc_acm` conflict with this component.

