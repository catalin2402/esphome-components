from esphome.components import button
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
from .. import gates_ns, CONF_GATES, GatesComponent

DEPENDENCIES = ["gates"]


GatesButton = gates_ns.class_("GatesButton", button.Button, cg.Component)

CONFIG_SCHEMA = (
    button.button_schema
    .extend(
        {
            cv.GenerateID(): cv.declare_id(GatesButton),
            cv.Required(CONF_GATES): cv.use_id(GatesComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await button.register_button(var, config)
    parent = await cg.get_variable(config[CONF_GATES])
    cg.add(var.set_parent(parent))
