"""USB audio playback source for speaker_source media players."""

import esphome.codegen as cg
from esphome.components import media_source
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import CORE
from esphome.types import ConfigType

from .. import CONF_TINYUSB_AUDIO_ID, TinyUSBAudio, tinyusb_audio_ns

DEPENDENCIES = ["tinyusb_audio"]

TinyUSBAudioSource = tinyusb_audio_ns.class_(
    "TinyUSBAudioSource", cg.Component, media_source.MediaSource
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
    cg.add(var.set_parent(await cg.get_variable(config[CONF_TINYUSB_AUDIO_ID])))
    if keyboard := CORE.config.get("tinyusb_keyboard"):
        cg.add(var.set_keyboard(await cg.get_variable(keyboard[CONF_ID])))
