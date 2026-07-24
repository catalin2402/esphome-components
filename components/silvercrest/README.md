# Silvercrest RF Sockets

Transmits the fixed 24-bit codes used by a Silvercrest four-channel remote
socket set. Channels A–D and the master channel can be exposed as switches or
individual push buttons.

This component currently requires the ESP32 Arduino framework.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [silvercrest]

silvercrest:
  id: silvercrest_remote
  transmitter_pin: GPIO2

switch:
  - platform: silvercrest
    silvercrest: silvercrest_remote
    channel: a
    name: Socket A

button:
  - platform: silvercrest
    silvercrest: silvercrest_remote
    button: m_off
    name: All sockets off
```

Switch channels are `a`, `b`, `c`, `d` and `master`. Button values are
`a_on`, `b_on`, `c_on`, `d_on`, `m_on`, `a_off`, `b_off`, `c_off`, `d_off`
and `m_off`.

Connect `transmitter_pin` to a suitable 433 MHz transmitter input. Observe
the transmitter module's voltage and antenna requirements.
