from esphome.components import number
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_MIN_VALUE, CONF_MAX_VALUE
from .. import hub75_icn2053_ns, HUB75_ICN2053, HUB75_ICN2053_ID

HUB75_ICN2053_NUMBER = hub75_icn2053_ns.class_(
    "HUB75_ICN2053Brightness", number.Number, cg.Component
)

CONFIG_SCHEMA = (
    number.number_schema(HUB75_ICN2053_NUMBER)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(HUB75_ICN2053_NUMBER),
            cv.GenerateID(HUB75_ICN2053_ID): cv.use_id(HUB75_ICN2053),
            cv.Optional(CONF_MIN_VALUE, default=0): cv.int_range(min=0, max=255),
            cv.Optional(CONF_MAX_VALUE, default=255): cv.int_range(min=0, max=255),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await number.register_number(
        var,
        config,
        min_value=config[CONF_MIN_VALUE],
        max_value=config[CONF_MAX_VALUE],
        step=1,
    )
    paren = await cg.get_variable(config[HUB75_ICN2053_ID])
    cg.add(var.set_parent(paren))
