"""Speaker wrapper that analyzes accepted PCM for an arbitrary light_id."""

from esphome import automation
import esphome.codegen as cg
from esphome.components import speaker
import esphome.config_validation as cv
from esphome.const import CONF_MODE

CONF_SOUND_REACTIVE_LIGHTS_ID = "sound_reactive_lights_id"
CONF_SENSITIVITY = "sensitivity"
CONF_MAX_BRIGHTNESS = "max_brightness"
CONF_ON_MODE = "on_mode"
CONF_ON_ENABLED = "on_enabled"

AUTO_LOAD = ["speaker", "light"]
CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["esp32"]
sound_reactive_lights_ns = cg.esphome_ns.namespace("sound_reactive_lights")
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
ModeTrigger = sound_reactive_lights_ns.class_(
    "ModeTrigger", automation.Trigger.template(cg.uint8)
)
EnabledTrigger = sound_reactive_lights_ns.class_(
    "EnabledTrigger", automation.Trigger.template(cg.bool_)
)

CONFIG_SCHEMA = cv.Schema({})

for name, call in (
    ("next_mode", "next_mode()"),
    ("enable", "set_enabled(true)"),
    ("disable", "set_enabled(false)"),
):
    automation.register_apply_action(
        f"sound_reactive_lights.{name}",
        automation.maybe_simple_id(
            {cv.GenerateID(): cv.use_id(SoundReactiveLightsSpeaker)}
        ),
        automation.ApplyCall(call),
    )

REACTIVE_ACTION_SCHEMA = automation.maybe_simple_id(
    {cv.GenerateID(): cv.use_id(SoundReactiveLightsSpeaker)}
)
automation.register_apply_condition(
    "sound_reactive_lights.is_enabled", REACTIVE_ACTION_SCHEMA, "is_enabled()"
)

for name, key, validator, method, value_type in (
    ("set_mode", CONF_MODE, cv.enum(MODES, lower=True), "set_mode", cg.uint8),
    (
        "set_sensitivity",
        CONF_SENSITIVITY,
        cv.float_range(min=0.1, max=100),
        "set_sensitivity",
        cg.float_,
    ),
    (
        "set_max_brightness",
        CONF_MAX_BRIGHTNESS,
        cv.percentage,
        "set_max_brightness",
        cg.float_,
    ),
):
    automation.register_apply_action(
        f"sound_reactive_lights.{name}",
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(SoundReactiveLightsSpeaker),
                cv.Required(key): cv.templatable(validator),
            }
        ),
        automation.ApplyField(key, method, value_type),
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
