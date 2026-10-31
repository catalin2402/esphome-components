"""Graphic equalizer speaker wrapper."""

from esphome import automation
import esphome.codegen as cg
from esphome.components import audio, speaker
import esphome.config_validation as cv
from esphome.const import (
    CONF_AUDIO_DAC,
    CONF_BITS_PER_SAMPLE,
    CONF_ID,
    CONF_NUM_CHANNELS,
    CONF_OUTPUT_SPEAKER,
    CONF_PRESET,
    CONF_SAMPLE_RATE,
    CONF_TRIGGER_ID,
)
import esphome.final_validate as fv
from esphome.helpers import fnv1_hash

from .. import (
    BAND_GAIN,
    BANDS,
    CONF_AUTO_HEADROOM,
    CONF_BANDS,
    CONF_ENABLED,
    CONF_ON_ENABLED,
    CONF_ON_GAIN,
    CONF_ON_PRESET,
    CONF_PREAMP,
    PREAMP_GAIN,
    PRESETS,
    EnabledTrigger,
    GainTrigger,
    GraphicEqualizerSpeaker,
    PresetTrigger,
)

DEPENDENCIES = ["esp32"]


def set_stream_limits(config):
    audio.set_stream_limits(
        min_bits_per_sample=16,
        max_bits_per_sample=16,
        min_channels=config.get(CONF_NUM_CHANNELS, 1),
        max_channels=config.get(CONF_NUM_CHANNELS, 2),
        min_sample_rate=config.get(CONF_SAMPLE_RATE, 44100),
        max_sample_rate=config.get(CONF_SAMPLE_RATE, 48000),
    )(config)
    return config


CONFIG_SCHEMA = cv.All(
    speaker.SPEAKER_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(GraphicEqualizerSpeaker),
            cv.Optional(CONF_AUDIO_DAC): cv.invalid(
                "Configure audio_dac on the output speaker instead"
            ),
            cv.Required(CONF_OUTPUT_SPEAKER): cv.use_id(speaker.Speaker),
            cv.Optional(CONF_BITS_PER_SAMPLE): cv.one_of(16, int=True),
            cv.Optional(CONF_NUM_CHANNELS): cv.int_range(min=1, max=2),
            cv.Optional(CONF_SAMPLE_RATE): cv.one_of(44100, 48000, int=True),
            cv.Optional(CONF_ENABLED, default=True): cv.boolean,
            cv.Optional(CONF_AUTO_HEADROOM, default=True): cv.boolean,
            cv.Optional(CONF_PRESET, default="flat"): cv.enum(PRESETS, lower=True),
            cv.Optional(CONF_PREAMP): PREAMP_GAIN,
            cv.Optional(CONF_BANDS): cv.Schema(
                {cv.Optional(band): BAND_GAIN for band in BANDS}
            ),
            cv.Optional(CONF_ON_GAIN): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(GainTrigger)}
            ),
            cv.Optional(CONF_ON_PRESET): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PresetTrigger)}
            ),
            cv.Optional(CONF_ON_ENABLED): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EnabledTrigger)}
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    set_stream_limits,
)


def final_validate(config):
    full = fv.full_config.get()
    seen = {str(config[CONF_ID])}
    target = config[CONF_OUTPUT_SPEAKER]
    while True:
        if str(target) in seen:
            raise cv.Invalid("graphic_equalizer output_speaker chain contains a cycle")
        seen.add(str(target))
        target_config = full.get_config_for_path(full.get_path_for_id(target)[:-1])
        if CONF_OUTPUT_SPEAKER not in target_config:
            break
        target = target_config[CONF_OUTPUT_SPEAKER]
    # Advertise only formats the downstream speaker can actually accept.
    rates = [config[CONF_SAMPLE_RATE]] if CONF_SAMPLE_RATE in config else [44100, 48000]
    channel_counts = (
        [config[CONF_NUM_CHANNELS]] if CONF_NUM_CHANNELS in config else [1, 2]
    )
    compatible = []
    for rate in rates:
        for channels in channel_counts:
            try:
                audio.final_validate_audio_schema(
                    "graphic_equalizer",
                    audio_device=CONF_OUTPUT_SPEAKER,
                    bits_per_sample=16,
                    channels=channels,
                    sample_rate=rate,
                )(config)
            except cv.Invalid:
                continue
            compatible.append((rate, channels))
    if not compatible:
        raise cv.Invalid(
            "output_speaker must accept the requested 16-bit mono/stereo format at 44100 or 48000 Hz"
        )
    audio.set_stream_limits(
        min_bits_per_sample=16,
        max_bits_per_sample=16,
        min_channels=min(channels for _, channels in compatible),
        max_channels=max(channels for _, channels in compatible),
        min_sample_rate=min(rate for rate, _ in compatible),
        max_sample_rate=max(rate for rate, _ in compatible),
    )(config)
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await speaker.register_speaker(var, config)
    cg.add(var.set_output_speaker(await cg.get_variable(config[CONF_OUTPUT_SPEAKER])))
    cg.add(
        var.set_preference_key(
            fnv1_hash(f"graphic_equalizer.{config[CONF_ID].id}.presets.v1")
        )
    )
    cg.add(var.set_auto_headroom(config[CONF_AUTO_HEADROOM]))
    cg.add(var.select_preset(config[CONF_PRESET]))
    if CONF_PREAMP in config:
        cg.add(var.set_preamp(config[CONF_PREAMP]))
    for band, gain in config.get(CONF_BANDS, {}).items():
        cg.add(var.set_band_gain(BANDS[band], gain))
    cg.add(var.set_enabled(config[CONF_ENABLED]))
    for key, args in (
        (CONF_ON_GAIN, [(cg.uint8, "band"), (cg.float_, "gain")]),
        (CONF_ON_PRESET, [(cg.uint8, "preset")]),
        (CONF_ON_ENABLED, [(cg.bool_, "enabled")]),
    ):
        for conf in config.get(key, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, args, conf)
