from esphome.components import number
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
from .. import pt2258_ns, CONF_PT2258_ID, PT2258

DEPENDENCIES = ["pt2258"]

PT2258Number = pt2258_ns.class_("PT2258Number", number.Number, cg.Component)

CONF_NUMBER_TYPE = "number_type"
CONF_ON_MUTE = "on_mute"
NUMBER_TYPE = {
    "MASTER": 0,
    "FRONT": 1,
    "REAR": 2,
    "CENTER": 3,
    "SUBWOOFER": 4,
}

CONFIG_SCHEMA = cv.All(
    number.NUMBER_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(PT2258Number),
            cv.GenerateID(CONF_PT2258_ID): cv.use_id(PT2258),
            cv.Required(CONF_NUMBER_TYPE): cv.enum(NUMBER_TYPE),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    min = 0
    max = 79

    if config[CONF_NUMBER_TYPE] != "MASTER":
        min = -22
        max = 10

    await cg.register_component(var, config)
    await number.register_number(
        var,
        config,
        min_value=min,
        max_value=max,
        step=1,
    )

    paren = await cg.get_variable(config[CONF_PT2258_ID])
    cg.add(var.set_parent(paren))
    cg.add(var.set_number_type(config[CONF_NUMBER_TYPE]))
