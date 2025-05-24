from esphome.components import number
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_TYPE, CONF_MIN_VALUE, CONF_MAX_VALUE
from .. import pt2258_ns, CONF_PT2258_ID, PT2258

DEPENDENCIES = ["pt2258"]

PT2258Number = pt2258_ns.class_("PT2258Number", number.Number, cg.Component)
NumberType = pt2258_ns.enum("NumberType")

CONF_NUMBER_TYPE = "number_type"
CONF_CHANNEL_A = "channel_a"
CONF_CHANNEL_B = "channel_b"

NUMBER_TYPE = {
    "MASTER": NumberType.MASTER,
    "VOLUME": NumberType.VOLUME,
    "OFFSET": NumberType.OFFSET,
}


def vaildate(value):
    if (
        value[CONF_TYPE] != "MASTER"
        and CONF_CHANNEL_A not in value
        and CONF_CHANNEL_B not in value
    ):
        raise cv.Invalid("At least one channel is needed")
    if value[CONF_TYPE] == "MASTER" and (
        CONF_CHANNEL_A in value or CONF_CHANNEL_B in value
    ):
        raise cv.Invalid("Please remove channel_a and channel_b from config")
    return value


CONFIG_SCHEMA = cv.All(
    number.number_schema(PT2258Number)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(PT2258Number),
            cv.GenerateID(CONF_PT2258_ID): cv.use_id(PT2258),
            cv.Optional(CONF_TYPE, default="master"): cv.enum(NUMBER_TYPE, upper=True),
            cv.Optional(CONF_CHANNEL_A): cv.int_range(min=1, max=6),
            cv.Optional(CONF_CHANNEL_B): cv.int_range(min=1, max=6),
            cv.Optional(CONF_MIN_VALUE, default=0): cv.int_range(min=-79, max=79),
            cv.Optional(CONF_MAX_VALUE, default=79): cv.int_range(min=-79, max=79),
        }
    )
    .extend(cv.polling_component_schema("1s")),
    vaildate,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await number.register_number(
        var,
        config,
        min_value=config[CONF_MIN_VALUE],
        max_value=config[CONF_MAX_VALUE],
        step=1,
    )
    paren = await cg.get_variable(config[CONF_PT2258_ID])
    cg.add(var.set_parent(paren))
    cg.add(var.set_type(config[CONF_TYPE]))
    if CONF_CHANNEL_A in config:
        cg.add(var.set_channel_a(config[CONF_CHANNEL_A]))
    if CONF_CHANNEL_B in config:
        cg.add(var.set_channel_b(config[CONF_CHANNEL_B]))
