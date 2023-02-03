from esphome.components import select
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_OPTIONS
from .. import pt2323_ns, CONF_PT2323_ID, PT2323

DEPENDENCIES = ["pt2323"]


PT2323Select = pt2323_ns.class_("PT2323Select", select.Select, cg.Component)

CONFIG_SCHEMA = select.SELECT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(PT2323Select),
        cv.GenerateID(CONF_PT2323_ID): cv.use_id(PT2323),
        cv.Required(CONF_OPTIONS): cv.All(
            cv.ensure_list(cv.string_strict), cv.Length(min=1)
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await select.new_select(config, options=config[CONF_OPTIONS])
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_PT2323_ID])
    cg.add(var.set_parent(parent))
