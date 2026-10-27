# USB Media Control

Adds USB HID media controls to a `usb_audio_device` device. Automations can send
a single media key press, hold a key, or release a held key on the connected
computer.

## Example configuration

Configure a `usb_audio_device` hub with the ID `usb_audio` first.

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [usb_audio_device, usb_media_control]

usb_media_control:
  id: usb_controls
  usb_audio_device_id: usb_audio
```

## Configuration

| Option | Description |
| --- | --- |
| `id` | Optional component ID for use in actions. |
| `usb_audio_device_id` | Optional ID of the USB audio hub that hosts the HID interface. Resolved automatically when one hub is configured. |

## Actions

| Action | Description |
| --- | --- |
| `usb_media_control.media_key` | Queue a key press followed by a release after 30ms. Requires `key`. |
| `usb_media_control.media_key_press` | Hold the specified `key` until released or replaced by another held key. |
| `usb_media_control.media_key_release` | Release the specified `key` if it is currently held. |
| `usb_media_control.media_key_release_all` | Release held keys and clear queued key presses. |

Supported keys: `PLAY`, `PAUSE`, `PLAY_PAUSE`, `STOP`, `NEXT`, `PREVIOUS`,
`FAST_FORWARD`, `REWIND`, `VOLUME_UP`, and `VOLUME_DOWN`.

Action IDs are optional when a single USB media controller is configured.

```yaml
on_press:
  - usb_media_control.media_key:
      key: PLAY_PAUSE
```

For a held volume control:

```yaml
on_press:
  - usb_media_control.media_key_press:
      key: VOLUME_UP
on_release:
  - usb_media_control.media_key_release:
      key: VOLUME_UP
```

The connected computer determines how each media key is handled.

## Media key buttons

Expose individual media keys as optional buttons:

```yaml
button:
  - platform: usb_media_control
    name: USB Play Pause
    key: PLAY_PAUSE
  - platform: usb_media_control
    name: USB Next Track
    key: NEXT
```

| Option | Description |
| --- | --- |
| `usb_media_control_id` | Optional ID of the USB media controller. Required to select between multiple matching controllers. |
| `key` | Required media key from the supported keys listed above. |

Standard button options apply. Each press queues the same 30ms key pulse as the
`usb_media_control.media_key` action. Buttons do not hold keys.

## Conditions

`usb_media_control.is_ready` is true when the HID endpoint is mounted, ready to
send a report, and the controller is running. The ID is optional when one
controller is configured.

```yaml
condition:
  usb_media_control.is_ready:
```

The endpoint may be briefly busy while sending a report. Media key actions and
buttons queue pulses while mounted, so checking readiness before each press is
optional.
