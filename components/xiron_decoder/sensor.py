import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, remote_base, sensor
from esphome.const import (
    CONF_HUMIDITY,
    CONF_ID,
    CONF_TEMPERATURE,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

AUTO_LOAD = ["binary_sensor"]
DEPENDENCIES = ["remote_receiver"]
CODEOWNERS = ["@catalin2402"]

CONF_XIRON_ID = "xiron_id"
CONF_BATTERY = "battery"

xiron_decoder_ns = cg.esphome_ns.namespace("xiron_decoder")
XironDecoder = xiron_decoder_ns.class_(
    "XironDecoder", remote_base.RemoteReceiverListener, cg.Component
)


def validate_xiron_id(value):
    if isinstance(value, int) and not isinstance(value, bool):
        if 0 <= value <= 0xFFFF:
            return value
        raise cv.Invalid("xiron_id must be between 0x0000 and 0xFFFF")

    value = cv.string_strict(value).strip()
    if value.lower().startswith("0x"):
        value = value[2:]
    if not 1 <= len(value) <= 4:
        raise cv.Invalid(
            "xiron_id must be a 1-4 character hex string, for example '1A2B'"
        )
    try:
        return int(value, 16)
    except ValueError:
        raise cv.Invalid("xiron_id must be hexadecimal, for example '1A2B'")


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XironDecoder),
            cv.Required(CONF_XIRON_ID): validate_xiron_id,
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_BATTERY): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_BATTERY
            ),
        }
    )
    .extend(remote_base.REMOTE_LISTENER_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await remote_base.register_listener(var, config)

    cg.add(var.set_xiron_id(config[CONF_XIRON_ID]))

    if CONF_TEMPERATURE in config:
        sens_t = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens_t))

    if CONF_HUMIDITY in config:
        sens_h = await sensor.new_sensor(config[CONF_HUMIDITY])
        cg.add(var.set_humidity_sensor(sens_h))

    if CONF_BATTERY in config:
        batt = await binary_sensor.new_binary_sensor(config[CONF_BATTERY])
        cg.add(var.set_battery_sensor(batt))
