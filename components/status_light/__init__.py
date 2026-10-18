import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_BLUE, CONF_GREEN, CONF_ID, CONF_LIGHT_ID, CONF_RED

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["light"]
MULTI_CONF = True
MULTI_CONF_NO_DEFAULT = True

status_light_ns = cg.esphome_ns.namespace("status_light")
StatusLight = status_light_ns.class_("StatusLight", cg.Component)

CONF_COLORS = "colors"
STATUS_COLORS = ("error", "warning", "network", "api")
RGB_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_RED, default=0): cv.percentage,
        cv.Optional(CONF_GREEN, default=0): cv.percentage,
        cv.Optional(CONF_BLUE, default=0): cv.percentage,
    }
)
STATUS_LIGHT_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_LIGHT_ID): cv.use_id(light.LightState),
        cv.Optional(CONF_COLORS, default={}): cv.Schema(
            {cv.Optional(status): RGB_SCHEMA for status in STATUS_COLORS}
        ),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(StatusLight),
        }
    )
    .extend(STATUS_LIGHT_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def setup_status_light(var, config):
    light_var = await cg.get_variable(config[CONF_LIGHT_ID])
    cg.add(var.set_light(light_var))
    for status, color in config[CONF_COLORS].items():
        setter = getattr(var, f"set_{status}_color")
        cg.add(setter(color[CONF_RED], color[CONF_GREEN], color[CONF_BLUE]))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await setup_status_light(var, config)
