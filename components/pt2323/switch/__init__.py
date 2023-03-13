from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_TYPE
from .. import pt2323_ns, CONF_PT2323_ID, PT2323
from esphome import automation

DEPENDENCIES = ["pt2323"]

CONF_CHANNEL_A = "channel_a"
CONF_CHANNEL_B = "channel_b"

SWITCH_TYPE = {
    "enhance": 0,
    "boost": 1,
    "mute": 2,
    "mute_all": 3,
}

PT2323Switch = pt2323_ns.class_("PT2323Switch", switch.Switch, cg.Component)


def vaildate(value):
    if (
        value[CONF_TYPE] == "mute"
        and CONF_CHANNEL_A not in value
        and CONF_CHANNEL_B not in value
    ):
        raise cv.Invalid("At least one channel is needed for mute switch")
    return value


CONFIG_SCHEMA = cv.All(
    switch.SWITCH_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(PT2323Switch),
            cv.GenerateID(CONF_PT2323_ID): cv.use_id(PT2323),
            cv.Required(CONF_TYPE): cv.enum(SWITCH_TYPE),
            cv.Optional(CONF_CHANNEL_A): cv.int_range(min=1, max=6),
            cv.Optional(CONF_CHANNEL_B): cv.int_range(min=1, max=6),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    vaildate,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)
    parent = await cg.get_variable(config[CONF_PT2323_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(config[CONF_TYPE]))
    if CONF_CHANNEL_A in config:
        cg.add(var.set_channel_a(config[CONF_CHANNEL_A]))
    if CONF_CHANNEL_B in config:
        cg.add(var.set_channel_b(config[CONF_CHANNEL_B]))
