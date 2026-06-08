import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, remote_base
from esphome.const import (
    CONF_TEMPERATURE,
    CONF_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

CONF_RECEIVER_ID = "receiver_id"
CONF_XIRON_ID = "xiron_id"
CONF_BATTERY = "battery"

from . import XironDecoder

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(XironDecoder),
        cv.GenerateID(CONF_RECEIVER_ID): cv.use_id(remote_base.RemoteReceiverBase),
        cv.Required(CONF_XIRON_ID): cv.string,
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
            device_class="battery"
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[cv.GenerateID()])
    await cg.register_component(var, config)

    xiron_id = int(config[CONF_XIRON_ID], 16)
    cg.add(var.set_xiron_id(xiron_id))
    if CONF_TEMPERATURE in config:
        sens_t = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens_t))

    if CONF_HUMIDITY in config:
        sens_h = await sensor.new_sensor(config[CONF_HUMIDITY])
        cg.add(var.set_humidity_sensor(sens_h))

    if CONF_BATTERY in config:
        batt = await binary_sensor.new_binary_sensor(config[CONF_BATTERY])
        cg.add(var.set_battery_sensor(batt))

    receiver = await cg.get_variable(config[CONF_RECEIVER_ID])
    cg.add(receiver.register_listener(var))
