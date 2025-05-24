from esphome.components import button
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
from .. import silvercrest_ns, CONF_SILVERCREST_ID, Silvercrest

DEPENDENCIES = ["silvercrest"]

BUTTON = "button"
BUTTON_TYPE = {
    "A_ON": 0,
    "B_ON": 1,
    "C_ON": 2,
    "D_ON": 3,
    "M_ON": 4,
    "A_OFF": 5,
    "B_OFF": 6,
    "C_OFF": 7,
    "D_OFF": 8,
    "M_OFF": 9,
    "DOORBELL": 10,
}

SilvercrestButton = silvercrest_ns.class_("SilvercrestButton", button.Button, cg.Component)

CONFIG_SCHEMA = cv.All(
    button.button.button_schema(SilvercrestButton)
    .extend(
        cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(SilvercrestButton),
                cv.GenerateID(CONF_SILVERCREST_ID): cv.use_id(Silvercrest),
                cv.Required(BUTTON): cv.enum(BUTTON_TYPE),
                
            }
        ),
    )
    .extend(cv.COMPONENT_SCHEMA),
    cv.only_with_arduino,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await button.register_button(var, config)

    parent = await cg.get_variable(config[CONF_SILVERCREST_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_button(config[BUTTON]))
