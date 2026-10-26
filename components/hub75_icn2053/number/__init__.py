import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_DISPLAY_ID, CONF_ID

from .. import HUB75Display, hub75_icn2053_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["hub75_icn2053"]

HUB75BrightnessNumber = hub75_icn2053_ns.class_(
    "HUB75BrightnessNumber", number.Number, cg.Component
)

CONFIG_SCHEMA = (
    number.number_schema(HUB75BrightnessNumber, icon="mdi:brightness-6")
    .extend({cv.GenerateID(CONF_DISPLAY_ID): cv.use_id(HUB75Display)})
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await number.register_number(var, config, min_value=0, max_value=255, step=1)
    parent = await cg.get_variable(config[CONF_DISPLAY_ID])
    cg.add(var.set_parent(parent))
