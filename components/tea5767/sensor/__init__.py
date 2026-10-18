import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_TYPE, STATE_CLASS_MEASUREMENT, UNIT_HERTZ

from .. import CONF_TEA5767_ID, TEA5767, tea5767_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["tea5767"]

TEA5767Sensor = tea5767_ns.class_("TEA5767Sensor", sensor.Sensor, cg.Component)
SensorType = tea5767_ns.enum("SensorType", is_class=True)
SENSOR_TYPES = {
    "frequency": SensorType.FREQUENCY,
    "signal_level": SensorType.SIGNAL_LEVEL,
}


def entity_schema(**kwargs):
    return (
        sensor.sensor_schema(
            TEA5767Sensor,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            **kwargs,
        )
        .extend(
            {
                cv.GenerateID(CONF_TEA5767_ID): cv.use_id(TEA5767),
            }
        )
        .extend(cv.COMPONENT_SCHEMA)
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        "frequency": entity_schema(
            unit_of_measurement=UNIT_HERTZ, icon="mdi:radio-tower"
        ),
        "signal_level": entity_schema(icon="mdi:signal"),
    },
    lower=True,
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_TEA5767_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(SENSOR_TYPES[config[CONF_TYPE]]))
