# USB Media Player

Provides a USB sound card on ESP32-S3 with optional playback and I2S microphone
capture. Plays audio from the computer through ESPHome speakers and streams
microphone audio back to the computer. Supports simultaneous playback and capture
and USB media-key controls.

The `usb_media_player` hub owns the USB connection. Its `media_source` platform
feeds the standard `speaker_source` media player, and its `switch` platform
controls microphone capture. Use an I2S speaker for a DAC, or ESPHome mixer and
resampler speakers when sharing the output with other audio sources. Home Assistant controls the standard
media player entity.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [usb_media_player]

esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: esp-idf

# USB audio uses the native USB port. Send serial logs through UART instead.
logger:
  hardware_uart: UART0

substitutions:
  pin_ext_io1: GPIO6
  pin_ext_io2: GPIO7
  pin_ext_io3: GPIO5

i2s_audio:
  - id: i2s_output
    i2s_lrclk_pin: $pin_ext_io1
    i2s_bclk_pin: $pin_ext_io3
  - id: i2s_input
    i2s_lrclk_pin: GPIO8
    i2s_bclk_pin: GPIO9

microphone:
  - platform: i2s_audio
    id: i2s_mic
    i2s_audio_id: i2s_input
    adc_type: external
    i2s_din_pin: GPIO10
    sample_rate: 48000
    bits_per_sample: 32bit
    channel: left

usb_media_player:
  id: usb_audio
  microphone: i2s_mic

switch:
  - platform: usb_media_player
    id: usb_microphone
    name: "USB Microphone"
    usb_media_player_id: usb_audio
    restore_mode: ALWAYS_OFF

speaker:
  - platform: i2s_audio
    id: speaker_id
    i2s_audio_id: i2s_output
    i2s_dout_pin: $pin_ext_io2
    dac_type: external
    sample_rate: 48000
    channel: stereo
    bits_per_sample: 16bit
    buffer_duration: 1000ms
    timeout: never

media_source:
  - platform: usb_media_player
    id: usb_input
    usb_media_player_id: usb_audio

media_player:
  - platform: speaker_source
    id: external_media_player
    name: "Media Player"
    volume_initial: 100%
    media_pipeline:
      speaker: speaker_id
      num_channels: 2
      sample_rate: 48000
      sources: usb_input
      format: NONE
```

Choose I2S GPIOs to match your hardware. Connect the USB host to the ESP32-S3's
native USB connector (D- on GPIO19, D+ on GPIO20), then select the USB sound card
as the host's audio output. A separate USB-to-UART connector does not carry USB
audio. Keep GPIO19 and GPIO20 available for USB.

USB audio is already decoded PCM, so the example uses `format: NONE`. It does
not need a WAV decoder. A pipeline containing other media sources can use their
preferred format, such as WAV or FLAC; USB still supplies PCM directly.

Only one USB hub and one USB playback media source are supported. The playback
source must belong to exactly one `speaker_source` pipeline. The native USB
controller cannot also be used for USB logging or another USB device component. Other ESP32 variants are not supported.

## Hub configuration

Configure these under `usb_media_player:`:

| Option | Purpose |
| --- | --- |
| `id` | Identifies the USB hub for its media source, microphone switch and automations. |
| `microphone` | Optional I2S microphone source. Omit it for playback only. |
| `microphone_buffer_size` | Capture buffer in bytes of internal RAM. Default: 8192. Accepts even values from 1920 to 65536. Requires `microphone`. |
| `sample_rate` | USB playback and capture sample rate. Currently 48000 only. |
| `manufacturer` | USB manufacturer string. Default: `ESPHome`. |
| `product_name` | Name shown by the USB host. Default: `USB Media Player`. |
| `serial_number` | USB serial string. Default: `ESPHomeUSB1`. |
| `usb_vid` | USB vendor ID. Default: `0x303A`. |
| `usb_pid` | USB product ID. Default: `0x8000`. |
| `support_macos` | Enables the USB audio feedback compatibility setting for macOS. Default: false. |

USB strings accept 1-20 printable ASCII characters. Playback is stereo, signed
16-bit PCM. Microphone capture is mono, signed 16-bit PCM. Both use 48 kHz.

For playback only, omit the microphone and its switch. The hub can be empty:

```yaml
usb_media_player:
```

For capture only, omit the USB playback media source, media player and speaker.
At least playback or microphone capture must be configured.

## Microphone configuration

The microphone must use ESPHome's `i2s_audio` platform and a sample rate of 48000.
Analog input is not supported. Set the I2S input GPIOs and left/right channel to
match your microphone. Use separate I2S buses for simultaneous playback and
capture; ESPHome's I2S microphone and speaker drivers lock their bus.

A microphone ID is sufficient:

```yaml
usb_media_player:
  microphone: i2s_mic
```

For channel selection or additional gain, use the standard microphone source
configuration:

```yaml
usb_media_player:
  microphone:
    microphone: i2s_mic
    bits_per_sample: 16
    channels: [0]
    gain_factor: 2
```

| Option | Purpose |
| --- | --- |
| `microphone` | ID of the native I2S microphone. |
| `bits_per_sample` | USB capture sample size. Must be 16. Native 32-bit microphone samples are converted automatically. |
| `channels` | One zero-based source channel to send to the PC. Default: `[0]`. A mono native microphone always uses channel 0, even when its hardware channel is right. |
| `gain_factor` | Integer capture gain from 1 to 64. Default: 1. High gain can clip the audio. |

No sample-rate conversion is performed. Capture does not depend on the selected
media player source: the PC can record the microphone while USB or another
media source is playing.

## Microphone switch

The `usb_media_player` switch platform enables or disables capture. It requires
a microphone on the hub.

```yaml
switch:
  - platform: usb_media_player
    name: "USB Microphone"
    usb_media_player_id: usb_audio
    restore_mode: ALWAYS_OFF
```

`usb_media_player_id` selects the hub and can be omitted when it resolves
unambiguously. Standard switch options are supported. `restore_mode` defaults
to `ALWAYS_OFF`; use `ALWAYS_ON` or a standard restore mode when desired.

ON allows capture while the USB host is connected. OFF stops this component's
microphone request, clears its capture buffer and sends silence to the PC. If
another component also uses the microphone, that consumer can continue using
it. The USB microphone remains listed on the PC while the switch is off.
Disconnecting USB or starting OTA also suspends capture without changing the
switch's requested state. Capture resumes when the connection returns or an OTA
attempt is cancelled. Missing microphone samples are filled with silence.

## Media source configuration

Configure these under `media_source: - platform: usb_media_player`:

| Option | Purpose |
| --- | --- |
| `id` | Identifies the playback source for the media pipeline. |
| `usb_media_player_id` | ID of the USB hub. Optional when it resolves unambiguously. |
| `buffer_size` | Playback packet buffer in bytes of internal RAM. Default: 32768. Accepts multiples of 4 from 8192 to 262144. |
| `auto_start` | Selects USB when the host connects. Default: true. |
| `output_gain_db` | Additional attenuation applied to USB playback, from -60 to 0 dB. Default: 0 dB. |

The USB packet buffer absorbs short scheduling delays. The speaker's
`buffer_duration` configures its separate playback buffer. A smaller speaker
buffer reduces playback latency. If the output cannot keep up, incoming packets
are dropped and a warning reports the number of lost audio frames.

## Playback and volume

With `auto_start: true`, USB is selected when the host connects. The playback
route stays active while connected, including through silence and gaps in USB
packets. There is no inactivity timeout. Connecting the headset selects its
local audio route without sending a Play key to the computer. The I2S speaker
example also uses `timeout: never` to keep its output driver active between
packets.

The media player's playing state describes the USB audio route, rather than the
host application's playback state. Track title, duration and exact host playback
state are not available.

Play and pause commands send media keys to the host. Pause leaves the local USB
route active, so notifications and audio from other applications still reach the
speaker. Next and previous are also delegated to the host. The host application
decides whether it handles those keys.

An explicit stop or selecting another media source releases the USB route.
Continuing USB packets do not select it again; select USB explicitly or reconnect
the USB host to return to it. USB disconnection and OTA suspension also stop
forwarding audio. To select USB explicitly, use:

```yaml
- media_player.play_media:
    id: external_media_player
    media_url: "usb://"
```

Use `auto_start: false` when you want to select USB only through this URI.
Sources such as Sendspin can share `media_pipeline.sources` with `usb_input`.
The media player selects the active source. For concurrent announcements, use
separate mixer speaker inputs and the standard announcement pipeline.

Home Assistant volume and mute apply to the speaker output. USB host volume and
mute apply to the incoming USB audio. They are independent, so both can attenuate
the signal. USB host changes do not change Home Assistant's displayed volume.

## USB media-key actions

These actions send keys to the USB host. They do not change the speaker's local
volume or directly select the USB source.

| Action | Purpose |
| --- | --- |
| `usb_media_player.volume_up` | Sends one volume-up key press. |
| `usb_media_player.volume_down` | Sends one volume-down key press. |
| `usb_media_player.play` | Sends a play key. |
| `usb_media_player.pause` | Sends a pause key. |
| `usb_media_player.play_pause` | Sends a play/pause toggle key. |
| `usb_media_player.stop` | Sends a stop key. |
| `usb_media_player.next_track` | Sends a next-track key. |
| `usb_media_player.previous_track` | Sends a previous-track key. |
| `usb_media_player.volume_up_pressed` | Holds the volume-up key until released. |
| `usb_media_player.volume_up_released` | Releases a held volume-up key. |
| `usb_media_player.volume_down_pressed` | Holds the volume-down key until released. |
| `usb_media_player.volume_down_released` | Releases a held volume-down key. |

Each action accepts an optional hub `id`. It can be omitted when the hub
resolves unambiguously:

```yaml
- usb_media_player.next_track:
- usb_media_player.volume_up:
    id: usb_audio
```

For local playback and output volume, use the standard `media_player` actions on
`external_media_player`.

## Microphone actions

`usb_media_player.enable_microphone` and `usb_media_player.disable_microphone`
control capture without requiring a switch entity. They accept an optional hub
`id`. A configured microphone switch stays synchronized with these actions.

```yaml
- usb_media_player.enable_microphone:
- usb_media_player.disable_microphone:
```

## Conditions

| Condition | Purpose |
| --- | --- |
| `usb_media_player.microphone_enabled` | True when microphone capture is enabled, even if the USB host is disconnected. |
| `usb_media_player.is_connected` | True when the USB host has enumerated the device. |
| `usb_media_player.is_streaming` | True while the connected USB playback route is active, including during silence or gaps in packets. False when stopped, deselected, disconnected or suspended for OTA. |

All conditions accept an optional hub `id`.

```yaml
- if:
    condition:
      usb_media_player.is_connected:
    then:
      - usb_media_player.play_pause:
```

## Triggers

Configure these on the `usb_media_player` hub:

| Trigger | Purpose |
| --- | --- |
| `on_connected` | Runs when the USB host enumerates the device. |
| `on_disconnected` | Runs when the USB connection is lost. |
| `on_stream_started` | Runs when the connected USB playback route becomes active. |
| `on_stream_stopped` | Runs when the USB playback route is stopped, deselected, disconnected or suspended for OTA. |

These triggers have no arguments. Stream triggers describe the local USB playback
route, rather than the host application or the presence of individual packets.

```yaml
usb_media_player:
  on_connected:
    - logger.log: "USB host connected"
  on_stream_started:
    - logger.log: "USB playback started"
  on_stream_stopped:
    - logger.log: "USB playback stopped"
```
