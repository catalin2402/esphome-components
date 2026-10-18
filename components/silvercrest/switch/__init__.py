import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_CHANNEL

from .. import CHANNELS, CONF_SILVERCREST_ID, Silvercrest, silvercrest_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["silvercrest"]

SilvercrestSwitch = silvercrest_ns.class_(
    "SilvercrestSwitch", switch.Switch, cg.Component
)

CONFIG_SCHEMA = (
    switch.switch_schema(SilvercrestSwitch)
    .extend(
        {
            cv.GenerateID(CONF_SILVERCREST_ID): cv.use_id(Silvercrest),
            cv.Required(CONF_CHANNEL): cv.enum(CHANNELS, upper=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_SILVERCREST_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_CHANNEL]))
