from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_TYPE
from .. import tea5767_ns, CONF_TEA5767_ID, TEA5767

DEPENDENCIES = ["tea5767"]

TEA5767Switch = tea5767_ns.class_("TEA5767Switch", switch.Switch, cg.Component)
SwitchType = tea5767_ns.enum("SwitchType")

SWITCH_TYPE = {
    "MUTE": SwitchType.MUTE,
    "MONO": SwitchType.MONO,
    "SOFT_MUTE": SwitchType.SOFT_MUTE,
    "STEREO_NOISE_CANCELING": SwitchType.STEREO_NOISE_CANCELING,
    "HIGH_CUT_CONTROL": SwitchType.HIGH_CUT_CONTROL,
}


CONFIG_SCHEMA = cv.All(
    switch.switch_schema(TEA5767Switch)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(TEA5767Switch),
            cv.GenerateID(CONF_TEA5767_ID): cv.use_id(TEA5767),
            cv.Required(CONF_TYPE): cv.enum(SWITCH_TYPE, upper=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)
    parent = await cg.get_variable(config[CONF_TEA5767_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(config[CONF_TYPE]))
