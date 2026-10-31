"""Five-band graphic equalizer for speaker audio."""

from esphome import automation
import esphome.codegen as cg
from esphome.components import speaker
import esphome.config_validation as cv
from esphome.const import CONF_GAIN, CONF_PRESET
import esphome.final_validate as fv

AUTO_LOAD = ["audio", "speaker"]
CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["esp32"]

CONF_GRAPHIC_EQUALIZER_ID = "graphic_equalizer_id"
CONF_BAND = "band"
CONF_BANDS = "bands"
CONF_PREAMP = "preamp"
CONF_ENABLED = "enabled"
CONF_AUTO_HEADROOM = "auto_headroom"
CONF_SLOT = "slot"
CONF_ON_GAIN = "on_gain"
CONF_ON_PRESET = "on_preset"
CONF_ON_ENABLED = "on_enabled"

graphic_equalizer_ns = cg.esphome_ns.namespace("graphic_equalizer")
GraphicEqualizerSpeaker = graphic_equalizer_ns.class_(
    "GraphicEqualizerSpeaker", cg.Component, speaker.Speaker
)
Preset = graphic_equalizer_ns.enum("Preset")
PRESETS = {
    "flat": Preset.PRESET_FLAT,
    "bass_boost": Preset.PRESET_BASS_BOOST,
    "treble_boost": Preset.PRESET_TREBLE_BOOST,
    "vocal": Preset.PRESET_VOCAL,
    "rock": Preset.PRESET_ROCK,
    "user_1": Preset.PRESET_USER_1,
    "user_2": Preset.PRESET_USER_2,
    "user_3": Preset.PRESET_USER_3,
}
BANDS = {"63hz": 0, "250hz": 1, "1khz": 2, "4khz": 3, "16khz": 4}
BAND_GAIN = cv.All(cv.decibel, cv.float_range(min=-12, max=12))
PREAMP_GAIN = cv.All(cv.decibel, cv.float_range(min=-36, max=0))
GainTrigger = graphic_equalizer_ns.class_(
    "GainTrigger", automation.Trigger.template(cg.uint8, cg.float_)
)
PresetTrigger = graphic_equalizer_ns.class_(
    "PresetTrigger", automation.Trigger.template(cg.uint8)
)
EnabledTrigger = graphic_equalizer_ns.class_(
    "EnabledTrigger", automation.Trigger.template(cg.bool_)
)

CONFIG_SCHEMA = cv.Schema({})
ACTION_SCHEMA = automation.maybe_simple_id(
    {cv.GenerateID(): cv.use_id(GraphicEqualizerSpeaker)}
)

for name, call in (
    ("enable", "set_enabled(true)"),
    ("disable", "set_enabled(false)"),
    ("reset", "reset()"),
):
    automation.register_apply_action(
        f"graphic_equalizer.{name}", ACTION_SCHEMA, automation.ApplyCall(call)
    )

for name, key, validator, method, value_type in (
    (
        "select_preset",
        CONF_PRESET,
        cv.enum(PRESETS, lower=True),
        "select_preset",
        cg.uint8,
    ),
    ("set_preamp", CONF_PREAMP, PREAMP_GAIN, "set_preamp", cg.float_),
    ("save_preset", CONF_SLOT, cv.int_range(min=1, max=3), "save_preset", cg.uint8),
):
    automation.register_apply_action(
        f"graphic_equalizer.{name}",
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(GraphicEqualizerSpeaker),
                cv.Required(key): cv.templatable(validator),
            }
        ),
        automation.ApplyField(key, method, value_type),
    )

automation.register_apply_action(
    "graphic_equalizer.set_band_gain",
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(GraphicEqualizerSpeaker),
            cv.Required(CONF_BAND): cv.templatable(cv.enum(BANDS, lower=True)),
            cv.Required(CONF_GAIN): cv.templatable(BAND_GAIN),
        }
    ),
    automation.ApplyCall(
        "set_band_gain({}, {})", ((CONF_BAND, cg.uint8), (CONF_GAIN, cg.float_))
    ),
)
automation.register_apply_condition(
    "graphic_equalizer.is_enabled", ACTION_SCHEMA, "is_enabled()"
)
automation.register_apply_condition(
    "graphic_equalizer.selected_preset",
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(GraphicEqualizerSpeaker),
            cv.Required(CONF_PRESET): cv.templatable(cv.enum(PRESETS, lower=True)),
        }
    ),
    automation.ApplyCall("get_preset() == {}", ((CONF_PRESET, cg.uint8),)),
)


def unique_control(domain):
    def validate(config):
        matching = [
            item
            for item in fv.full_config.get().get(domain, [])
            if item.get("platform") == "graphic_equalizer"
            and item[CONF_GRAPHIC_EQUALIZER_ID] == config[CONF_GRAPHIC_EQUALIZER_ID]
            and item.get("type") == config.get("type")
        ]
        if len(matching) > 1:
            raise cv.Invalid(
                f"Only one graphic_equalizer {domain} of each type may control a speaker"
            )
        return config

    return validate
