import esphome.codegen as cg
from esphome.components import remote_base, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

from .. import CONF_XIRON_ID, XironBase, register_xiron, xiron_ns

DEPENDENCIES = ["remote_receiver"]
XironSensor = xiron_ns.class_("XironSensor", XironBase, sensor.Sensor)
XironSensorType = xiron_ns.enum("XironSensorType", is_class=True)
SENSOR_TYPES = {
    "temperature": XironSensorType.TEMPERATURE,
    "humidity": XironSensorType.HUMIDITY,
}


def entity_schema(**kwargs):
    return (
        sensor.sensor_schema(XironSensor, state_class=STATE_CLASS_MEASUREMENT, **kwargs)
        .extend({cv.Optional(CONF_XIRON_ID): cv.uint16_t})
        .extend(remote_base.REMOTE_LISTENER_SCHEMA)
        .extend(cv.COMPONENT_SCHEMA)
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        "temperature": entity_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
        ),
        "humidity": entity_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_HUMIDITY,
        ),
    },
    lower=True,
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await register_xiron(var, config)
    cg.add(var.set_type(SENSOR_TYPES[config[CONF_TYPE]]))
