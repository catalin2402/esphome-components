from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_TYPE
from .. import gates_ns, CONF_GATES, GatesComponent

DEPENDENCIES = ["gates"]
SWITCH_TYPE = {
    "passthrough": 0,
    "relay": 1
}


GatesSwitch = gates_ns.class_("GatesSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(GatesSwitch),
        cv.Required(CONF_GATES): cv.use_id(GatesComponent),
        cv.Required(CONF_TYPE): cv.enum(SWITCH_TYPE),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)
    parent = await cg.get_variable(config[CONF_GATES])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(config[CONF_TYPE]))
