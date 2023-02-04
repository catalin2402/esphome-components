from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_ON_TURN_OFF, CONF_ON_TURN_ON, CONF_TRIGGER_ID
from .. import pt2258_ns, CONF_PT2258_ID, PT2258
from esphome import automation

DEPENDENCIES = ["pt2258"]


PT2258Switch = pt2258_ns.class_("PT2258Switch", switch.Switch, cg.Component)

SwitchTurnOnTrigger = pt2258_ns.class_(
    "SwitchTurnOnTrigger", automation.Trigger.template()
)
SwitchTurnOffTrigger = pt2258_ns.class_(
    "SwitchTurnOffTrigger", automation.Trigger.template()
)

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(PT2258Switch),
        cv.GenerateID(CONF_PT2258_ID): cv.use_id(PT2258),
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
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    parent = await cg.get_variable(config[CONF_PT2258_ID])
    cg.add(var.set_parent(parent))
