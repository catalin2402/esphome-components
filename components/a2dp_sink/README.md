# Native ESP-IDF A2DP sink

Small Classic Bluetooth SBC receiver for the original ESP32 and ESPHome's
`media_source` / `speaker_source` audio pipeline. Uses ESP-IDF directly: no
ESP32-A2DP, Arduino, AudioTools, or separately downloaded codec library.

Requires ESPHome with `media_source` and `speaker_source` support and ESP-IDF 5.x
with the built-in SBC decoder. ESP-IDF 5.5 calls the decoded PCM interface
"legacy"; a future IDF release removing that interface will require changes.
ESP32-S3/C3/C6 and other chips without Classic Bluetooth cannot receive A2DP.

## Configuration

Remove the PxPert external component entry and add `a2dp_sink` to this repository's
existing component list:

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [pt2323, pt2258, a2dp_sink]

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    advanced:
      compiler_optimization: SIZE

a2dp_sink:
  id: bluetooth_sink
  name: bedroom2-sound-system
  minimal_bluetooth: true
  buffer_size: 32768

media_source:
  - platform: a2dp_sink
    id: bluetooth_media_source
    a2dp_sink_id: bluetooth_sink

switch:
  - platform: a2dp_sink
    type: bluetooth
    a2dp_sink_id: bluetooth_sink
    name: Bluetooth A2DP
    restore_mode: RESTORE_DEFAULT_OFF
```

Keep your existing `i2s_audio`, ES8388 DAC, speaker, Sendspin source and
`speaker_source` player. Include `bluetooth_media_source` in the player's
`media_pipeline.sources` alongside the Sendspin source, exactly as before.
The external source URL works after these changes are pushed; for local testing
use `source: {type: local, path: /path/to/esphome-components/components}`.

## Behavior and limits

* One sink, one A2DP media source, one speaker pipeline.
* Bluetooth starts on unless a Bluetooth switch is configured; the switch's
  restore mode then determines the startup state.
* Switch off hides the device, rejects new connections and disconnects the phone.
  The initialized stack remains in memory so switching back on is reliable.
  Switching off does not reduce firmware size or release the stack's RAM.
* Pair from the phone and start playback to select the Bluetooth source.
* Play/pause/next/previous use native AVRCP commands. Switching to another source
  pauses the phone and blocks automatic takeover until Bluetooth is selected
  explicitly or the phone reconnects.
* SBC audio is decoded by ESP-IDF to signed 16-bit PCM. The negotiated sample
  rate and channel count are passed to the speaker pipeline. The downstream
  pipeline must support that format or provide resampling (phones often use
  44.1 kHz; a fixed 48 kHz output may require a resampler).
* `buffer_size` is 8192–131072 bytes of RAM, default 32768. This does not reduce
  flash usage. Overflow drops audio and logs a warning instead of blocking the
  Bluetooth task. A few initial packets can be lost while the player selects
  the source.
* Classic-only mode (`minimal_bluetooth: true`) cannot coexist with BLE.
  Optional shared mode is described below.
* No metadata, RSSI, cover art, phone absolute-volume synchronization, extra
  entities, automation triggers, or automatic reconnect. Pairing keys remain in
  the IDF bond database, so the phone can reconnect without pairing again.
  Set listening volume through the existing speaker player/hardware controls.
* Other upstream options/platforms are not drop-in compatible.

## Optional BLE coexistence

`minimal_bluetooth: false` uses the controller and Bluedroid owned by
`esp32_ble`. It requires `classic_bt: true`, introduced by
[ESPHome PR #21408](https://github.com/esphome/esphome/pull/21408).
While that PR is unmerged, load its `esp32_ble` external component on an
ESPHome version compatible with the PR branch:

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [a2dp_sink]
  - source: github://pr#21408
    components: [esp32_ble]

esp32_ble:
  classic_bt: true

a2dp_sink:
  name: bedroom2-sound-system
  minimal_bluetooth: false
```

Keep the media source, speaker pipeline and switch configuration from above.
BLE trackers and Bluetooth proxy are permitted in this mode. A2DP waits for
ESP32BLE to start the shared stack and registers only its Classic GAP, A2DP and
AVRCP callbacks. It does not release BLE memory or change BLE's GATT settings.
Disabling ESP32BLE also interrupts A2DP; after BLE is re-enabled, the profiles
are registered again and the phone must reconnect. The A2DP Bluetooth switch
only affects Classic discoverability/connections and leaves BLE running.

Shared mode retains BLE code and RAM; use the default Classic-only mode when
minimizing flash usage. Coexistence and BLE disable/re-enable need hardware
testing; the PR itself does not validate A2DP playback.

## Flash size

Removing the wrapper avoids its application code, but Classic Bluetooth,
Bluedroid, AVRCP and SBC remain necessary. No specific saving or fit with LVGL is
guaranteed without comparing actual firmware builds.

Your pasted configuration uses `compiler_optimization: PERF`; try `SIZE` first.
Compare the same configuration and ESPHome/IDF versions with the upstream and
native components, using clean builds, and record the linked application bytes
and maximum application partition size. Then re-enable LVGL and rebuild. RAM
availability should also be checked during simultaneous Ethernet/audio/display
activity. PSRAM does not increase flash capacity.

Do not enlarge flash size beyond the physical chip capacity. Changing partitions
can increase the application slot but may sacrifice OTA or storage; the right
choice depends on the actual flash chip and required OTA behavior.

Hardware validation is required for pairing, audio continuity, source switching,
repeated Bluetooth toggles and OTA under load before deployment.
