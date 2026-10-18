import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_TYPE

from .. import CONF_TEA5767_ID, TEA5767, tea5767_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["tea5767"]

TEA5767BinarySensor = tea5767_ns.class_(
    "TEA5767BinarySensor", binary_sensor.BinarySensor, cg.Component
)
ReceptionType = tea5767_ns.enum("ReceptionType", is_class=True)
RECEPTION_TYPES = {"mono": ReceptionType.MONO, "stereo": ReceptionType.STEREO}

ENTITY_SCHEMA = (
    binary_sensor.binary_sensor_schema(TEA5767BinarySensor, icon="mdi:surround-sound")
    .extend(
        {
            cv.GenerateID(CONF_TEA5767_ID): cv.use_id(TEA5767),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)
CONFIG_SCHEMA = cv.typed_schema(
    {"mono": ENTITY_SCHEMA, "stereo": ENTITY_SCHEMA}, lower=True
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_TEA5767_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(RECEPTION_TYPES[config[CONF_TYPE]]))
