from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID,  CONF_ON_TURN_OFF, CONF_ON_TURN_ON, CONF_TRIGGER_ID
from .. import pt2323_ns, CONF_PT2323_ID, PT2323
from esphome import automation

DEPENDENCIES = ['pt2323']

CONF_SWITCH_TYPE = "switch_type"
SWITCH_TYPE = {
    "ENHANCE": 0,
    "BOOST": 1,
    "MUTE": 2,
    "MUTE_FRONT": 3,
    "MUTE_REAR": 4,
    "MUTE_SUBWOOFER": 5,
    "MUTE_CENTER": 6,
}

PT2323Switch = pt2323_ns.class_('PT2323Switch', switch.Switch, cg.Component)

SwitchTurnOnTrigger = pt2323_ns.class_(
    "SwitchTurnOnTrigger", automation.Trigger.template()
)
SwitchTurnOffTrigger = pt2323_ns.class_(
    "SwitchTurnOffTrigger", automation.Trigger.template()
)

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(PT2323Switch),
    cv.GenerateID(CONF_PT2323_ID): cv.use_id(PT2323),
    cv.Required(CONF_SWITCH_TYPE): cv.enum(SWITCH_TYPE),
    cv.Optional(CONF_ON_TURN_ON): automation.validate_automation(
        {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SwitchTurnOnTrigger),
        }
    ),
    cv.Optional(CONF_ON_TURN_OFF): automation.validate_automation(
        {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SwitchTurnOffTrigger),
        }
    ),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    parent = await cg.get_variable(config[CONF_PT2323_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_switch_type(config[CONF_SWITCH_TYPE]))
