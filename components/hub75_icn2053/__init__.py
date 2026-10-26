from esphome import automation
import esphome.codegen as cg
from esphome.components import display
import esphome.config_validation as cv
from esphome.const import CONF_BRIGHTNESS, CONF_ID, CONF_MAX, CONF_MIN

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["esp32"]

hub75_icn2053_ns = cg.esphome_ns.namespace("hub75_icn2053")
HUB75Display = hub75_icn2053_ns.class_("HUB75Display", display.DisplayBuffer)
SetBrightnessAction = hub75_icn2053_ns.class_("SetBrightnessAction", automation.Action)
IsReadyCondition = hub75_icn2053_ns.class_("IsReadyCondition", automation.Condition)
BrightnessInRangeCondition = hub75_icn2053_ns.class_(
    "BrightnessInRangeCondition", automation.Condition
)

BASE_SCHEMA = cv.Schema({cv.GenerateID(): cv.use_id(HUB75Display)})


@automation.register_action(
    "hub75_icn2053.set_brightness",
    SetBrightnessAction,
    cv.maybe_simple_value(
        BASE_SCHEMA.extend(
            {cv.Required(CONF_BRIGHTNESS): cv.templatable(cv.int_range(min=0, max=255))}
        ),
        key=CONF_BRIGHTNESS,
    ),
    synchronous=True,
)
async def set_brightness_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_BRIGHTNESS], args, cg.int_)
    cg.add(var.set_brightness(value))
    return var


@automation.register_condition(
    "hub75_icn2053.is_ready",
    IsReadyCondition,
    automation.maybe_simple_id(BASE_SCHEMA),
)
async def is_ready_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


def validate_brightness_range(config):
    if config[CONF_MIN] > config[CONF_MAX]:
        raise cv.Invalid("min must be less than or equal to max")
    return config


@automation.register_condition(
    "hub75_icn2053.brightness_in_range",
    BrightnessInRangeCondition,
    cv.All(
        BASE_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN, default=0): cv.int_range(min=0, max=255),
                cv.Optional(CONF_MAX, default=255): cv.int_range(min=0, max=255),
            }
        ),
        validate_brightness_range,
    ),
)
async def brightness_in_range_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(
        condition_id, template_arg, parent, config[CONF_MIN], config[CONF_MAX]
    )
