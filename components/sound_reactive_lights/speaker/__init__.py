from esphome import automation
import esphome.codegen as cg
from esphome.components import audio, light, speaker
import esphome.config_validation as cv
from esphome.const import (
    CONF_BITS_PER_SAMPLE,
    CONF_ID,
    CONF_LIGHT_ID,
    CONF_MODE,
    CONF_NUM_CHANNELS,
    CONF_PLATFORM,
    CONF_SAMPLE_RATE,
    CONF_TRIGGER_ID,
    CONF_UPDATE_INTERVAL,
)
import esphome.final_validate as fv

from .. import (
    CONF_MAX_BRIGHTNESS,
    CONF_ON_ENABLED,
    CONF_ON_MODE,
    CONF_SENSITIVITY,
    MODES,
    EnabledTrigger,
    ModeTrigger,
    SoundReactiveLightsSpeaker,
)

CONF_ENABLED = "enabled"
CONF_OUTPUT_SPEAKER = "output_speaker"
CONF_SILENCE_TIMEOUT = "silence_timeout"

DEPENDENCIES = ["esp32"]


def stream_limits(config):
    if CONF_BITS_PER_SAMPLE in config:
        cv.one_of(16, int=True)(config[CONF_BITS_PER_SAMPLE])
    if CONF_NUM_CHANNELS in config:
        cv.int_range(min=1, max=2)(config[CONF_NUM_CHANNELS])
    if CONF_SAMPLE_RATE in config:
        cv.int_range(min=16000, max=48000)(config[CONF_SAMPLE_RATE])
    audio.set_stream_limits(
        min_bits_per_sample=16,
        max_bits_per_sample=16,
        min_channels=config.get(CONF_NUM_CHANNELS, 1),
        max_channels=config.get(CONF_NUM_CHANNELS, 2),
        min_sample_rate=config.get(CONF_SAMPLE_RATE, 16000),
        max_sample_rate=config.get(CONF_SAMPLE_RATE, 48000),
    )(config)
    return config


CONFIG_SCHEMA = cv.All(
    speaker.SPEAKER_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(SoundReactiveLightsSpeaker),
            cv.Optional(CONF_ON_MODE): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ModeTrigger)}
            ),
            cv.Optional(CONF_ON_ENABLED): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EnabledTrigger)}
            ),
            cv.Required(CONF_OUTPUT_SPEAKER): cv.use_id(speaker.Speaker),
            cv.Required(CONF_LIGHT_ID): cv.use_id(light.LightState),
            cv.Optional(CONF_MODE, default="spectrum"): cv.enum(MODES, lower=True),
            cv.Optional(CONF_ENABLED, default=True): cv.boolean,
            cv.Optional(CONF_SENSITIVITY, default=4.0): cv.float_range(
                min=0.1, max=100
            ),
            cv.Optional(CONF_MAX_BRIGHTNESS, default="100%"): cv.percentage,
            cv.Optional(
                CONF_UPDATE_INTERVAL, default="20ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_SILENCE_TIMEOUT, default="250ms"
            ): cv.positive_time_period_milliseconds,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    stream_limits,
)


def final_validate(config):
    speakers = fv.full_config.get().get("speaker", [])
    mapping = {str(item[CONF_ID]): item for item in speakers}
    seen = {str(config[CONF_ID])}
    target = str(config[CONF_OUTPUT_SPEAKER])
    while target in mapping:
        if target in seen:
            raise cv.Invalid(
                "sound_reactive_lights output_speaker chain contains a cycle"
            )
        seen.add(target)
        item = mapping[target]
        if CONF_OUTPUT_SPEAKER not in item:
            break
        target = str(item[CONF_OUTPUT_SPEAKER])
    for item in speakers:
        if (
            item.get(CONF_PLATFORM) == "sound_reactive_lights"
            and item[CONF_ID] != config[CONF_ID]
            and item.get(CONF_LIGHT_ID) == config[CONF_LIGHT_ID]
        ):
            raise cv.Invalid(
                "Only one sound_reactive_lights instance can own a light_id"
            )

    audio.final_validate_audio_schema(
        "sound_reactive_lights",
        audio_device=CONF_OUTPUT_SPEAKER,
        bits_per_sample=16,
        channels=config.get(CONF_NUM_CHANNELS, cv.UNDEFINED),
        sample_rate=config.get(CONF_SAMPLE_RATE, cv.UNDEFINED),
    )(config)
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await speaker.register_speaker(var, config)
    cg.add(var.set_output_speaker(await cg.get_variable(config[CONF_OUTPUT_SPEAKER])))
    cg.add(var.set_light(await cg.get_variable(config[CONF_LIGHT_ID])))
    for key in (CONF_MODE, CONF_ENABLED, CONF_SENSITIVITY, CONF_MAX_BRIGHTNESS):
        cg.add(getattr(var, f"set_{key}")(config[key]))
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL].total_milliseconds))
    cg.add(var.set_silence_timeout(config[CONF_SILENCE_TIMEOUT].total_milliseconds))

    for conf in config.get(CONF_ON_MODE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.uint8, "mode")], conf)
    for conf in config.get(CONF_ON_ENABLED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "enabled")], conf)
