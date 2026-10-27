# USB Audio Device

Exposes an ESP32-S3 as a USB audio device with stereo playback from a computer
and optional mono microphone capture back to the computer. Audio uses 48000 Hz and
16-bit samples. Playback can go directly to a speaker or through a media source.

## Example configuration

For headphones without a microphone, configure an I2S speaker named `speaker_id`
using 48000 Hz and 16-bit samples. The component requires ESP-IDF and the
ESP32-S3 native USB port.

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [usb_audio_device]

logger:
  hardware_uart: UART0

usb_audio_device:
  id: usb_audio
  product: USB Headphones
  playback:
    speaker: speaker_id
```

To add a microphone, configure a mono I2S microphone using 48000 Hz and 16-bit
samples, then add `capture:` to the hub:

```yaml
usb_audio_device:
  id: usb_audio
  playback:
    speaker: speaker_id
  capture:
    microphone: microphone_id
```

Omitting `capture:` removes the USB microphone interface and endpoint. It also
omits the microphone task and capture audio buffers.

The USB port is dedicated to this device. Do not configure USB host mode,
`tinyusb`, `usb_cdc_acm`, or a USB logger on the same port.

## Configuration

| Option | Description |
| --- | --- |
| `id` | Optional component ID. |
| `manufacturer` | USB manufacturer string. Default: `ESPHome`. |
| `product` | USB product name. Default: `USB Headset`. The displayed audio name combines manufacturer and product. |
| `serial` | USB serial string. Default: `ESPHOME-UAC-1`. |
| `vendor_id` | USB vendor ID. Default: `0x303A`. |
| `product_id` | USB product ID. Default: `0x8001`. |
| `playback` | Playback settings. Set `speaker` here or configure one media source below. |
| `capture` | Optional microphone capture settings. Omit for playback-only headphones. |
| `activity_threshold` | Sample amplitude threshold for detecting audible playback, from 1 to 32767. Default: `64`. |
| `activity_timeout` | Time without an above-threshold sample before playback activity becomes inactive. Default: `750ms`. This does not stop the audio stream. |

### Playback

| Option | Description |
| --- | --- |
| `speaker` | Direct playback destination. Omit when using the media source platform. |
| `sample_rate` | Sample rate. Must be `48000`. |
| `bits_per_sample` | Sample depth. Must be `16`. |
| `channels` | Channel count. Must be `2`. |

### Capture

These settings apply only when `capture:` is configured.

| Option | Description |
| --- | --- |
| `microphone` | Required ID of a mono microphone configured for 48000 Hz and 16-bit samples. |
| `sample_rate` | Sample rate. Must be `48000`. |
| `bits_per_sample` | Sample depth. Must be `16`. |
| `channels` | Channel count. Must be `1`. |

## Media source

To use USB audio with a `speaker_source` media player, omit `playback.speaker`
and add a USB media source. Reference the configured speaker in the media
player pipeline instead.

```yaml
usb_audio_device:

media_source:
  - platform: usb_audio_device
    id: usb_source

media_player:
  - platform: speaker_source
    name: USB Media Player
    media_pipeline:
      speaker: speaker_id
      num_channels: 2
      sources: usb_source
      format: NONE
```

`usb_audio_device_id` selects the USB audio hub and can be omitted when one hub
is configured. Standard media source options also apply. The source accepts `usb://current` and requests
playback when audible computer audio starts. Silence keeps the selected audio
route open. Adding `usb_media_control` enables host transport controls.

## Actions

| Action | Description |
| --- | --- |
| `usb_audio_device.microphone_mute` | Send silence to the computer instead of microphone samples. |
| `usb_audio_device.microphone_unmute` | Resume sending microphone samples. |

Action IDs are optional when a single USB audio device is configured.

```yaml
on_press:
  - usb_audio_device.microphone_mute:
```

Playback and microphone capture pause during OTA. If the update fails or is
aborted, audio is enabled again without requiring a USB reconnection.

## Microphone mute switch

The optional switch requires a hub with `capture:` configured. It sends silence
to the computer when ON and microphone audio when OFF. It stays synchronized with the microphone mute/unmute actions.

```yaml
switch:
  - platform: usb_audio_device
    name: Microphone Mute
    usb_audio_device_id: usb_audio
```

`usb_audio_device_id` is optional when one USB audio hub is configured. Only one
mute switch can control each hub. Standard switch options apply, including
`restore_mode`. Its default is `DISABLED`, which uses the hub's initial mute
state. Muting does not close the USB capture interface or stop playback.

## Conditions

| Condition | Description |
| --- | --- |
| `usb_audio_device.is_connected` | The USB device has been enumerated by a host. USB suspend keeps the connection state. |
| `usb_audio_device.is_playback_active` | The host playback interface is open and audio is not suspended for OTA. This does not require audible samples or selection of the USB media source. |
| `usb_audio_device.is_capture_active` | The host capture interface is open and audio is not suspended for OTA. Muting does not change this state. |
| `usb_audio_device.is_microphone_muted` | Software microphone mute is enabled. |

Condition IDs are optional when one USB audio hub is configured.

```yaml
condition:
  usb_audio_device.is_connected:
```

## Triggers

Configure triggers under `usb_audio_device:`. Connection and interface events
are dispatched on the main loop. Playback and capture stop/start events also
reflect OTA suspension and recovery. Silence alone does not stop either stream.

| Trigger | Arguments | Description |
| --- | --- | --- |
| `on_connected` | None | A host enumerates the USB device. |
| `on_disconnected` | None | The USB device is unmounted by the host. |
| `on_playback_started` | None | The host playback interface becomes active. |
| `on_playback_stopped` | None | The host playback interface becomes inactive. |
| `on_capture_started` | None | The host microphone capture interface becomes active. |
| `on_capture_stopped` | None | The host microphone capture interface becomes inactive. |
| `on_playback_activity` | `bool active` | Audible playback crosses the configured activity threshold or times out. |
| `on_microphone_mute` | `bool muted` | Software microphone mute changes. |

Add triggers to the existing hub configuration, for example:

```yaml
usb_audio_device:
  id: usb_audio
  playback:
    speaker: speaker_id
  capture:
    microphone: microphone_id
  on_connected:
    - logger.log: USB host connected
  on_playback_activity:
    - logger.log:
        format: "USB audio activity: %s"
        args: ['active ? "yes" : "no"']
```

Without `capture:`, capture/microphone conditions remain false, microphone mute
actions have no effect, and capture/microphone triggers do not fire.
