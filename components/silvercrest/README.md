# Silvercrest RF Sockets

Controls a Silvercrest four-channel RF socket set. Exposes channels A–D and the
master channel as switches or individual command buttons. An optional RF
receiver can log recognized remote commands or expose them as binary sensors.

## Example configuration

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [silvercrest]

remote_transmitter:
  id: rf_tx
  pin: GPIO2
  carrier_duty_percent: 100%

silvercrest:
  id: silvercrest_remote
  transmitter_id: rf_tx

switch:
  - platform: silvercrest
    silvercrest: silvercrest_remote
    channel: a
    name: "Socket A"

button:
  - platform: silvercrest
    silvercrest: silvercrest_remote
    button: m_off
    name: "All Sockets Off"
```

Connect the configured transmitter GPIO to the data input of a 433 MHz RF
transmitter. When there is only one transmitter, `transmitter_id` can be omitted
and the hub can be written as `silvercrest:`.

## Hub configuration

| Option | Description |
| --- | --- |
| `id` | Optional ID used to reference the hub. |
| `transmitter_id` | Transmitter used to send commands. Can be omitted when there is only one transmitter. |
| `repeat_times` | Number of packets per command, from 1 to 255. Defaults to 5. |

## Switch configuration

| Option | Description |
| --- | --- |
| `silvercrest` | Hub ID. Can be omitted when there is only one hub. |
| `channel` | Required. One of `a`, `b`, `c`, `d`, or `master`. |
| `name` | Name of the switch. |

Switches send on and off commands and report the requested state. They cannot
confirm whether a socket received a command. Received handheld remote commands
do not update these switches. Standard ESPHome switch options are supported.

## Button configuration

| Option | Description |
| --- | --- |
| `silvercrest` | Hub ID. Can be omitted when there is only one hub. |
| `button` | Required. One of `a_on`, `b_on`, `c_on`, `d_on`, `m_on`, `a_off`, `b_off`, `c_off`, `d_off`, or `m_off`. |
| `name` | Name of the button. |

Each press sends the selected command. `m_on` and `m_off` control all sockets.
Standard ESPHome button options are supported.

## Receiving remote commands

Add a receiver to log recognized commands or detect a particular command:

```yaml
remote_receiver:
  id: rf_rx
  pin: GPIO4
  tolerance: 25%
  filter: 100us
  idle: 4ms
  dump: [silvercrest]

binary_sensor:
  - platform: remote_receiver
    name: "Socket A On Command"
    receiver_id: rf_rx
    silvercrest:
      channel: a
      state: true
    filters:
      - delayed_off: 250ms
```

The `silvercrest` hub must be configured to load the protocol. The nested binary
sensor options are `channel` (`a`, `b`, `c`, `d`, or `master`) and `state` (`true`
for an on command, `false` for an off command). This binary sensor detects a
remote command, rather than the actual socket state. `delayed_off` holds the
pulse long enough to be visible in Home Assistant.

## Transmit action

Commands can also be sent from automations:

```yaml
on_press:
  - remote_transmitter.transmit_silvercrest:
      transmitter_id: rf_tx
      channel: master
      state: false
      repeat_times: 5
```

`channel` and `state` select the command. `repeat_times` defaults to 5 and is
independent of the hub setting. These three fields also accept lambdas.
