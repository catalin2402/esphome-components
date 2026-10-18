import esphome.codegen as cg
from esphome.components import remote_base, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_PRECIPITATION,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_CELSIUS,
    UNIT_MILLIMETER,
)

from . import (
    CONF_AURIOL_ID,
    CONF_MM_PER_TIP,
    Auriol4LD5661Base,
    auriol_4ld5661_ns,
    register_auriol,
)

DEPENDENCIES = ["remote_receiver"]
AuriolSensor = auriol_4ld5661_ns.class_(
    "AuriolSensor", Auriol4LD5661Base, sensor.Sensor
)
AuriolSensorType = auriol_4ld5661_ns.enum("AuriolSensorType", is_class=True)
SENSOR_TYPES = {
    "temperature": AuriolSensorType.TEMPERATURE,
    "rain": AuriolSensorType.RAIN,
    "rain_tips": AuriolSensorType.RAIN_TIPS,
}


def entity_schema(**kwargs):
    return (
        sensor.sensor_schema(AuriolSensor, **kwargs)
        .extend({cv.Optional(CONF_AURIOL_ID): cv.uint8_t})
        .extend(remote_base.REMOTE_LISTENER_SCHEMA)
        .extend(cv.COMPONENT_SCHEMA)
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        "temperature": entity_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        "rain": entity_schema(
            unit_of_measurement=UNIT_MILLIMETER,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_PRECIPITATION,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ).extend({cv.Optional(CONF_MM_PER_TIP, default=0.3): cv.positive_float}),
        "rain_tips": entity_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
    },
    lower=True,
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await register_auriol(var, config)
    cg.add(var.set_type(SENSOR_TYPES[config[CONF_TYPE]]))
    if CONF_MM_PER_TIP in config:
        cg.add(var.set_mm_per_tip(config[CONF_MM_PER_TIP]))
