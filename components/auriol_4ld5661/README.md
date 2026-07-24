# Auriol 4LD5661

Decodes the 52-bit RF frames sent by an Auriol 4LD5661 weather sensor. The
component publishes temperature, battery status, and the sensor's 20-bit
cumulative rain counter. It can also derive individual rain events and
in-memory rolling 12-hour and 24-hour totals.

## Example

```yaml
external_components:
  - source: github://catalin2402/esphome-components
    components: [auriol_4ld5661]

remote_receiver:
  id: rf_receiver
  pin: GPIO4
  tolerance: 50%
  filter: 250us
  idle: 4ms

time:
  - platform: homeassistant
    id: homeassistant_time

sensor:
  - platform: auriol_4ld5661
    receiver_id: rf_receiver
    auriol_id: "3A"
    time_id: homeassistant_time
    mm_per_tip: 0.3
    rain_timeout: 1h

    temperature:
      name: "Outdoor temperature"
    rain:
      name: "Rain counter"
    rain_tips:
      name: "Rain counter in tips"
    total_last_rain:
      name: "Current or last rain event"
    total_rain_last_12h:
      name: "Rain in last 12 hours"
    total_rain_last_24h:
      name: "Rain in last 24 hours"
    battery:
      name: "Weather sensor battery low"
    start_of_rain:
      name: "Start of rain"
    end_of_rain:
      name: "End of rain"
```

`auriol_id` is optional, but it should be set when more than one compatible
sensor can be received. It accepts an integer, a `0x`-prefixed value, or a
one-to-two digit hexadecimal string.

`time_id` is only needed for meaningful `start_of_rain` and `end_of_rain`
timestamps. Without a valid time source, those entities publish
`no_time_source` or `time_not_valid`.

The rolling 12-hour and 24-hour totals are kept in RAM and start from zero
after a reboot. `rain` and `rain_tips` come directly from the weather sensor's
cumulative counter.
