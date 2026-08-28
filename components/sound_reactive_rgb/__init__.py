import logging

from esphome import automation
import esphome.codegen as cg
from esphome.components import light, usb_audio_bridge
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["usb_audio_bridge"]
AUTO_LOAD = ["light"]
CODEOWNERS = []
MULTI_CONF = False

_LOGGER = logging.getLogger(__name__)

CONF_AUDIO_SOURCE_ID = "audio_source_id"
CONF_LEFT_LIGHT_ID = "left_light_id"
CONF_RIGHT_LIGHT_ID = "right_light_id"
CONF_MODE = "mode"
CONF_BRIGHTNESS = "brightness"
CONF_SENSITIVITY = "sensitivity"
CONF_NOISE_FLOOR = "noise_floor"
CONF_SMOOTHING = "smoothing"
CONF_UPDATE_INTERVAL = "update_interval"
CONF_BUFFER_FRAMES = "buffer_frames"
CONF_USE_PSRAM = "use_psram"
CONF_FOLLOW_VOLUME = "follow_volume"

sound_reactive_rgb_ns = cg.esphome_ns.namespace("sound_reactive_rgb")
SoundReactiveRGB = sound_reactive_rgb_ns.class_("SoundReactiveRGB", cg.Component)
SoundReactiveMode = sound_reactive_rgb_ns.enum("SoundReactiveMode", is_class=True)

MODES = {
    "off": SoundReactiveMode.OFF,
    "stereo_vu": SoundReactiveMode.STEREO_VU,
    "spectrum": SoundReactiveMode.SPECTRUM,
    "rainbow_pulse": SoundReactiveMode.RAINBOW_PULSE,
}

SetModeAction = sound_reactive_rgb_ns.class_("SetModeAction", automation.Action)

SET_MODE_ACTION_SCHEMA = cv.maybe_simple_value(
    {
        cv.GenerateID(): cv.use_id(SoundReactiveRGB),
        cv.Required(CONF_MODE): cv.templatable(cv.one_of(*MODES.keys(), lower=True)),
    },
    key=CONF_MODE,
)


@automation.register_action(
    "sound_reactive_rgb.set_mode",
    SetModeAction,
    SET_MODE_ACTION_SCHEMA,
    synchronous=True,
)
async def sound_reactive_rgb_set_mode_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    mode = await cg.templatable(config[CONF_MODE], args, cg.std_string)
    cg.add(var.set_mode(mode))
    return var

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SoundReactiveRGB),
        cv.Required(CONF_AUDIO_SOURCE_ID): cv.use_id(usb_audio_bridge.USBAudioBridge),
        cv.Optional(CONF_LEFT_LIGHT_ID): cv.use_id(light.LightState),
        cv.Optional(CONF_RIGHT_LIGHT_ID): cv.use_id(light.LightState),
        cv.Optional(CONF_MODE, default="stereo_vu"): cv.enum(MODES, lower=True),
        cv.Optional(CONF_BRIGHTNESS, default="70%"): cv.percentage,
        cv.Optional(CONF_SENSITIVITY, default=4.0): cv.float_range(min=0.1, max=50.0),
        cv.Optional(CONF_NOISE_FLOOR, default="0.2%"): cv.percentage,
        cv.Optional(CONF_SMOOTHING, default=0.80): cv.float_range(min=0.0, max=0.99),
        cv.Optional(CONF_UPDATE_INTERVAL, default="20ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_BUFFER_FRAMES, default=8192): cv.int_range(min=512, max=32768),
        cv.Optional(CONF_USE_PSRAM, default=True): cv.boolean,
        cv.Optional(CONF_FOLLOW_VOLUME, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    source = await cg.get_variable(config[CONF_AUDIO_SOURCE_ID])
    cg.add(var.set_audio_source(source))

    if CONF_LEFT_LIGHT_ID in config:
        left_light = await cg.get_variable(config[CONF_LEFT_LIGHT_ID])
        cg.add(var.set_left_light(left_light))

    if CONF_RIGHT_LIGHT_ID in config:
        right_light = await cg.get_variable(config[CONF_RIGHT_LIGHT_ID])
        cg.add(var.set_right_light(right_light))

    if CONF_LEFT_LIGHT_ID not in config and CONF_RIGHT_LIGHT_ID not in config:
        _LOGGER.warning(
            "No light is configured for sound_reactive_rgb; the component will remain idle"
        )

    cg.add(var.set_mode(config[CONF_MODE]))
    cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))
    cg.add(var.set_sensitivity(config[CONF_SENSITIVITY]))
    cg.add(var.set_noise_floor(config[CONF_NOISE_FLOOR]))
    cg.add(var.set_smoothing(config[CONF_SMOOTHING]))
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL].total_milliseconds))
    cg.add(var.set_buffer_frames(config[CONF_BUFFER_FRAMES]))
    cg.add(var.set_use_psram(config[CONF_USE_PSRAM]))
    cg.add(var.set_follow_volume(config[CONF_FOLLOW_VOLUME]))
