from esphome import automation
import esphome.codegen as cg
from esphome.components import display
import esphome.config_validation as cv
from esphome.const import CONF_BRIGHTNESS
from esphome.types import ConfigType

from ._publisher import Publisher

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["esp32"]
DOMAIN = "hub75_icn2053"

CONF_MIN = "min"
CONF_MAX = "max"

hub75_icn2053_ns = cg.esphome_ns.namespace(DOMAIN)
HUB75Display = hub75_icn2053_ns.class_("HUB75Display", display.DisplayBuffer)
Hub75Pins = hub75_icn2053_ns.struct("Hub75Pins")
STATE_PUBLISHER = Publisher(
    "state",
    cg.void,
    [(cg.int_, "brightness")],
    define="USE_HUB75_ICN2053_STATE_PUBLISHER",
)
BRIGHTNESS = STATE_PUBLISHER.param("brightness")
DUMP_PUBLISHER = Publisher("dump", cg.void, [(cg.const_char_ptr, "TAG")])

BASE_SCHEMA = cv.Schema({cv.GenerateID(): cv.use_id(HUB75Display)})


def validate_brightness_range(config: ConfigType) -> ConfigType:
    if config[CONF_MIN] > config[CONF_MAX]:
        raise cv.Invalid("min must be less than or equal to max")
    return config


automation.register_apply_action(
    "hub75_icn2053.set_brightness",
    cv.maybe_simple_value(
        BASE_SCHEMA.extend(
            {cv.Required(CONF_BRIGHTNESS): cv.templatable(cv.int_range(min=0, max=255))}
        ),
        key=CONF_BRIGHTNESS,
    ),
    automation.ApplyField(CONF_BRIGHTNESS, "set_brightness", cg.int_),
)

automation.register_apply_condition(
    "hub75_icn2053.is_ready",
    automation.maybe_simple_id(BASE_SCHEMA),
    "is_display_ready()",
)

automation.register_apply_condition(
    "hub75_icn2053.brightness_in_range",
    cv.All(
        BASE_SCHEMA.extend(
            {
                cv.Optional(CONF_MIN, default=0): cv.int_range(min=0, max=255),
                cv.Optional(CONF_MAX, default=255): cv.int_range(min=0, max=255),
            }
        ),
        validate_brightness_range,
    ),
    automation.ApplyCall(
        "get_brightness() >= {} && {parent}->get_brightness() <= {}",
        ((CONF_MIN, cg.uint8), (CONF_MAX, cg.uint8)),
    ),
)
