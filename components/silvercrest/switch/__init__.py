from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
from .. import silvercrest_ns, CONF_SILVERCREST_ID, Silvercrest

DEPENDENCIES = ["silvercrest"]

CONF_CHANNEL = "channel"
CHANNEL_ID = {
    "A": 0,
    "B": 1,
    "C": 2,
    "D": 3,
    "MASTER": 4,
}

SilvercrestButton = silvercrest_ns.class_("SilvercrestSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SilvercrestButton),
        cv.GenerateID(CONF_SILVERCREST_ID): cv.use_id(Silvercrest),
        cv.Required(CONF_CHANNEL): cv.enum(CHANNEL_ID),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    parent = await cg.get_variable(config[CONF_SILVERCREST_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_CHANNEL]))
