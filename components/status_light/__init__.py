import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_BLUE, CONF_GREEN, CONF_ID, CONF_LIGHT_ID, CONF_RED
from esphome.types import ConfigType

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["light"]
DOMAIN = "status_light"
MULTI_CONF = True
MULTI_CONF_NO_DEFAULT = True

status_light_ns = cg.esphome_ns.namespace(DOMAIN)
StatusLight = status_light_ns.class_("StatusLight", cg.Component)

CONF_COLORS = "colors"
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
            {
                cv.Optional("error"): RGB_SCHEMA,
                cv.Optional("warning"): RGB_SCHEMA,
                cv.Optional("network"): RGB_SCHEMA,
                cv.Optional("api"): RGB_SCHEMA,
            }
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


async def setup_status_light(var: cg.MockObj, config: ConfigType) -> None:
    await cg.add_variable(var.set_light, config[CONF_LIGHT_ID])
    colors = config[CONF_COLORS]
    if (color := colors.get("error")) is not None:
        cg.add(
            var.set_error_color(color[CONF_RED], color[CONF_GREEN], color[CONF_BLUE])
        )
    if (color := colors.get("warning")) is not None:
        cg.add(
            var.set_warning_color(color[CONF_RED], color[CONF_GREEN], color[CONF_BLUE])
        )
    if (color := colors.get("network")) is not None:
        cg.add(
            var.set_network_color(color[CONF_RED], color[CONF_GREEN], color[CONF_BLUE])
        )
    if (color := colors.get("api")) is not None:
        cg.add(var.set_api_color(color[CONF_RED], color[CONF_GREEN], color[CONF_BLUE]))


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await setup_status_light(var, config)
