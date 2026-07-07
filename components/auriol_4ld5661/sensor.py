import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, remote_base, text_sensor, time
from esphome.const import (
    CONF_TEMPERATURE,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_BATTERY,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_MILLIMETER,
)

CONF_RECEIVER_ID = "receiver_id"
CONF_TIME_ID = "time_id"
CONF_AURIOL_ID = "auriol_id"
CONF_RAIN = "rain"
CONF_RAIN_TIPS = "rain_tips"
CONF_BATTERY = "battery"
CONF_START_OF_RAIN = "start_of_rain"
CONF_END_OF_RAIN = "end_of_rain"
CONF_TOTAL_LAST_RAIN = "total_last_rain"
CONF_TOTAL_RAIN_LAST_12H = "total_rain_last_12h"
CONF_TOTAL_RAIN_LAST_24H = "total_rain_last_24h"
CONF_RAIN_TIMEOUT = "rain_timeout"
CONF_MM_PER_TIP = "mm_per_tip"

from . import Auriol4LD5661

def validate_auriol_id(value):
    value = cv.string(value).upper().replace("0X", "")
    if len(value) == 0 or len(value) > 2:
        raise cv.Invalid("auriol_id must be a 1-2 character hex string, for example '3A'")
    try:
        int(value, 16)
    except ValueError:
        raise cv.Invalid("auriol_id must be hexadecimal, for example '3A'")
    return value

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Auriol4LD5661),
    cv.GenerateID(CONF_RECEIVER_ID): cv.use_id(remote_base.RemoteReceiverBase),
    cv.Optional(CONF_TIME_ID): cv.use_id(time.RealTimeClock),

    cv.Optional(CONF_AURIOL_ID): validate_auriol_id,
    cv.Optional(CONF_RAIN_TIMEOUT, default="1h"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_MM_PER_TIP, default=0.3): cv.positive_float,

    cv.Required(CONF_TEMPERATURE): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),

    cv.Optional(CONF_RAIN): sensor.sensor_schema(
        unit_of_measurement=UNIT_MILLIMETER,
        accuracy_decimals=1,
        state_class=STATE_CLASS_MEASUREMENT,
    ),

    cv.Optional(CONF_RAIN_TIPS): sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),

    cv.Optional(CONF_TOTAL_LAST_RAIN): sensor.sensor_schema(
        unit_of_measurement=UNIT_MILLIMETER,
        accuracy_decimals=1,
        state_class=STATE_CLASS_MEASUREMENT,
    ),

    cv.Optional(CONF_TOTAL_RAIN_LAST_12H): sensor.sensor_schema(
        unit_of_measurement=UNIT_MILLIMETER,
        accuracy_decimals=1,
        state_class=STATE_CLASS_MEASUREMENT,
    ),

    cv.Optional(CONF_TOTAL_RAIN_LAST_24H): sensor.sensor_schema(
        unit_of_measurement=UNIT_MILLIMETER,
        accuracy_decimals=1,
        state_class=STATE_CLASS_MEASUREMENT,
    ),

    cv.Optional(CONF_START_OF_RAIN): text_sensor.text_sensor_schema(),
    cv.Optional(CONF_END_OF_RAIN): text_sensor.text_sensor_schema(),

    cv.Optional(CONF_BATTERY): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_BATTERY,
    ),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[cv.GenerateID()])
    await cg.register_component(var, config)

    if CONF_AURIOL_ID in config:
        cg.add(var.set_auriol_id(int(config[CONF_AURIOL_ID], 16)))

    cg.add(var.set_rain_timeout(config[CONF_RAIN_TIMEOUT]))
    cg.add(var.set_mm_per_tip(config[CONF_MM_PER_TIP]))

    if CONF_TIME_ID in config:
        rtc = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time(rtc))

    temp = await sensor.new_sensor(config[CONF_TEMPERATURE])
    cg.add(var.set_temperature_sensor(temp))

    if CONF_RAIN in config:
        rain = await sensor.new_sensor(config[CONF_RAIN])
        cg.add(var.set_rain_sensor(rain))

    if CONF_RAIN_TIPS in config:
        rain_tips = await sensor.new_sensor(config[CONF_RAIN_TIPS])
        cg.add(var.set_rain_tips_sensor(rain_tips))

    if CONF_TOTAL_LAST_RAIN in config:
        total_last = await sensor.new_sensor(config[CONF_TOTAL_LAST_RAIN])
        cg.add(var.set_total_last_rain_sensor(total_last))

    if CONF_TOTAL_RAIN_LAST_12H in config:
        total_12h = await sensor.new_sensor(config[CONF_TOTAL_RAIN_LAST_12H])
        cg.add(var.set_total_rain_last_12h_sensor(total_12h))

    if CONF_TOTAL_RAIN_LAST_24H in config:
        total_24h = await sensor.new_sensor(config[CONF_TOTAL_RAIN_LAST_24H])
        cg.add(var.set_total_rain_last_24h_sensor(total_24h))

    if CONF_START_OF_RAIN in config:
        start = await text_sensor.new_text_sensor(config[CONF_START_OF_RAIN])
        cg.add(var.set_start_of_rain_sensor(start))

    if CONF_END_OF_RAIN in config:
        end = await text_sensor.new_text_sensor(config[CONF_END_OF_RAIN])
        cg.add(var.set_end_of_rain_sensor(end))

    if CONF_BATTERY in config:
        batt = await binary_sensor.new_binary_sensor(config[CONF_BATTERY])
        cg.add(var.set_battery_sensor(batt))

    receiver = await cg.get_variable(config[CONF_RECEIVER_ID])
    cg.add(receiver.register_listener(var))
