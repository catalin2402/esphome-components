"""USB audio playback source for speaker_source media players."""

import esphome.codegen as cg
from esphome.components import media_source
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import CORE
from esphome.types import ConfigType

from .. import CONF_TINYUSB_AUDIO_ID, TinyUSBAudio, tinyusb_audio_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["tinyusb_audio"]

TinyUSBAudioSource = tinyusb_audio_ns.class_(
    "TinyUSBAudioSource",
    cg.Component,
    media_source.MediaSource,
    cg.Parented.template(TinyUSBAudio),
)

CONFIG_SCHEMA = (
    media_source.media_source_schema(TinyUSBAudioSource)
    .extend({cv.GenerateID(CONF_TINYUSB_AUDIO_ID): cv.use_id(TinyUSBAudio)})
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await media_source.register_media_source(var, config)
    await cg.register_parented(var, config[CONF_TINYUSB_AUDIO_ID])
    if keyboard := CORE.config.get("tinyusb_keyboard"):
        await cg.add_variable(var.set_keyboard, keyboard[CONF_ID])
