"""Five-band graphic equalizer for speaker audio."""

from esphome import automation
import esphome.codegen as cg
from esphome.components import speaker
import esphome.config_validation as cv
from esphome.const import CONF_GAIN, CONF_PRESET, CONF_TYPE
import esphome.final_validate as fv

from ._publisher import Publisher

AUTO_LOAD = ["audio", "speaker"]
CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["esp32"]
DOMAIN = "graphic_equalizer"

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

graphic_equalizer_ns = cg.esphome_ns.namespace(DOMAIN)
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
Curve = graphic_equalizer_ns.struct("Curve")
STATE_PUBLISHER = Publisher(
    "state",
    cg.void,
    [(Curve.const_ref(), "curve"), (cg.uint8, "preset"), (cg.bool_, "enabled")],
    define="USE_GRAPHIC_EQUALIZER_STATE_PUBLISHER",
)
CURVE = STATE_PUBLISHER.param("curve")
PRESET = STATE_PUBLISHER.param("preset")
ENABLED = STATE_PUBLISHER.param("enabled")
RESTORE_PUBLISHER = Publisher("restore", cg.void, [])
DUMP_PUBLISHER = Publisher("dump", cg.void, [(cg.const_char_ptr, "TAG")])

CONFIG_SCHEMA = cv.Schema({})
ACTION_SCHEMA = automation.maybe_simple_id(
    {cv.GenerateID(): cv.use_id(GraphicEqualizerSpeaker)}
)

automation.register_apply_action(
    "graphic_equalizer.enable", ACTION_SCHEMA, automation.ApplyCall("set_enabled(true)")
)

automation.register_apply_action(
    "graphic_equalizer.disable",
    ACTION_SCHEMA,
    automation.ApplyCall("set_enabled(false)"),
)

automation.register_apply_action(
    "graphic_equalizer.reset", ACTION_SCHEMA, automation.ApplyCall("reset()")
)

automation.register_apply_action(
    "graphic_equalizer.select_preset",
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(GraphicEqualizerSpeaker),
            cv.Required(CONF_PRESET): cv.templatable(cv.enum(PRESETS, lower=True)),
        }
    ),
    automation.ApplyField(CONF_PRESET, "select_preset", cg.uint8),
)

automation.register_apply_action(
    "graphic_equalizer.set_preamp",
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(GraphicEqualizerSpeaker),
            cv.Required(CONF_PREAMP): cv.templatable(PREAMP_GAIN),
        }
    ),
    automation.ApplyField(CONF_PREAMP, "set_preamp", cg.float_),
)

automation.register_apply_action(
    "graphic_equalizer.save_preset",
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(GraphicEqualizerSpeaker),
            cv.Required(CONF_SLOT): cv.templatable(cv.int_range(min=1, max=3)),
        }
    ),
    automation.ApplyField(CONF_SLOT, "save_preset", cg.uint8),
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


def unique_control(domain: str):
    return fv.unique_platform_key(
        domain=domain,
        platform=DOMAIN,
        keys=(CONF_GRAPHIC_EQUALIZER_ID, CONF_TYPE)
        if domain == "number"
        else (CONF_GRAPHIC_EQUALIZER_ID,),
        message=f"Only one graphic_equalizer {domain} of each type may control a speaker",
    )
