"""Speaker wrapper that analyzes accepted PCM for an arbitrary light_id."""

from esphome import automation
import esphome.codegen as cg
from esphome.components import speaker
import esphome.config_validation as cv
from esphome.const import CONF_MODE

from ._publisher import Publisher

CONF_SOUND_REACTIVE_LIGHTS_ID = "sound_reactive_lights_id"
CONF_SENSITIVITY = "sensitivity"
CONF_MAX_BRIGHTNESS = "max_brightness"
CONF_ON_MODE = "on_mode"
CONF_ON_ENABLED = "on_enabled"

AUTO_LOAD = ["speaker", "light"]
CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["esp32"]
DOMAIN = "sound_reactive_lights"
sound_reactive_lights_ns = cg.esphome_ns.namespace(DOMAIN)
SoundReactiveLightsSpeaker = sound_reactive_lights_ns.class_(
    "SoundReactiveLightsSpeaker", speaker.Speaker, cg.Component
)

ReactiveMode = sound_reactive_lights_ns.enum("ReactiveMode")
MODES = {
    "off": ReactiveMode.MODE_OFF,
    "vu": ReactiveMode.MODE_VU,
    "spectrum": ReactiveMode.MODE_SPECTRUM,
    "rainbow_pulse": ReactiveMode.MODE_RAINBOW_PULSE,
}
STATE_PUBLISHER = Publisher(
    "state",
    cg.void,
    [
        (cg.uint8, "mode"),
        (cg.bool_, "enabled"),
        (cg.float_, "sensitivity"),
        (cg.float_, "max_brightness"),
    ],
    define="USE_SOUND_REACTIVE_LIGHTS_STATE_PUBLISHER",
)
MODE = STATE_PUBLISHER.param("mode")
ENABLED = STATE_PUBLISHER.param("enabled")
SENSITIVITY = STATE_PUBLISHER.param("sensitivity")
MAX_BRIGHTNESS = STATE_PUBLISHER.param("max_brightness")
RESTORE_PUBLISHER = Publisher("restore", cg.void, [])
DUMP_PUBLISHER = Publisher("dump", cg.void, [(cg.const_char_ptr, "TAG")])

CONFIG_SCHEMA = cv.Schema({})

REACTIVE_ACTION_SCHEMA = automation.maybe_simple_id(
    {cv.GenerateID(): cv.use_id(SoundReactiveLightsSpeaker)}
)

automation.register_apply_action(
    "sound_reactive_lights.next_mode",
    REACTIVE_ACTION_SCHEMA,
    automation.ApplyCall("next_mode()"),
)

automation.register_apply_action(
    "sound_reactive_lights.enable",
    REACTIVE_ACTION_SCHEMA,
    automation.ApplyCall("set_enabled(true)"),
)

automation.register_apply_action(
    "sound_reactive_lights.disable",
    REACTIVE_ACTION_SCHEMA,
    automation.ApplyCall("set_enabled(false)"),
)

automation.register_apply_action(
    "sound_reactive_lights.set_mode",
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(SoundReactiveLightsSpeaker),
            cv.Required(CONF_MODE): cv.templatable(cv.enum(MODES, lower=True)),
        }
    ),
    automation.ApplyField(CONF_MODE, "set_mode", cg.uint8),
)

automation.register_apply_action(
    "sound_reactive_lights.set_sensitivity",
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(SoundReactiveLightsSpeaker),
            cv.Required(CONF_SENSITIVITY): cv.templatable(
                cv.float_range(min=0.1, max=100)
            ),
        }
    ),
    automation.ApplyField(CONF_SENSITIVITY, "set_sensitivity", cg.float_),
)

automation.register_apply_action(
    "sound_reactive_lights.set_max_brightness",
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(SoundReactiveLightsSpeaker),
            cv.Required(CONF_MAX_BRIGHTNESS): cv.templatable(cv.percentage),
        }
    ),
    automation.ApplyField(CONF_MAX_BRIGHTNESS, "set_max_brightness", cg.float_),
)

automation.register_apply_condition(
    "sound_reactive_lights.is_enabled", REACTIVE_ACTION_SCHEMA, "is_enabled()"
)

automation.register_apply_condition(
    "sound_reactive_lights.selected_mode",
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(SoundReactiveLightsSpeaker),
            cv.Required(CONF_MODE): cv.templatable(cv.enum(MODES, lower=True)),
        }
    ),
    automation.ApplyCall("get_mode() == {}", ((CONF_MODE, cg.uint8),)),
)
