from esphome.components import binary_sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
from .. import gates_ns, CONF_GATES, GatesComponent

DEPENDENCIES = ["gates"]

CONF_PIN = "pin"

GatesBinarySensor = gates_ns.class_(
    "GatesBinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(GatesBinarySensor)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(GatesBinarySensor),
            cv.Required(CONF_GATES): cv.use_id(GatesComponent),
            cv.Required(CONF_PIN): cv.int_range(min=0, max=13),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_GATES])
    cg.add(var.set_parent(parent))
    cg.add(var.set_pin(config[CONF_PIN]))
