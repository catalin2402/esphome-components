from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_CHANNEL
from .. import silvercrest_ns, CONF_SILVERCREST_ID, Silvercrest

DEPENDENCIES = ["silvercrest"]

Channel = silvercrest_ns.enum("Channel")

CHANNELS = {
    "A": Channel.A,
    "B": Channel.B,
    "C": Channel.C,
    "D": Channel.D,
    "MASTER": Channel.MASTER,
}

SilvercrestButton = silvercrest_ns.class_(
    "SilvercrestSwitch", switch.Switch, cg.Component
)

CONFIG_SCHEMA = (
    switch.switch_schema(SilvercrestButton)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(SilvercrestButton),
            cv.GenerateID(CONF_SILVERCREST_ID): cv.use_id(Silvercrest),
            cv.Required(CONF_CHANNEL): cv.enum(CHANNELS, upper=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    parent = await cg.get_variable(config[CONF_SILVERCREST_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_CHANNEL]))
