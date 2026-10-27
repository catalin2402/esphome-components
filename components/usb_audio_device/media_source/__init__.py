"""USB PCM source for ESPHome's speaker_source media player."""

import esphome.codegen as cg
from esphome.components import media_source
import esphome.config_validation as cv
from esphome.const import CONF_ID

from .. import CONF_USB_AUDIO_DEVICE_ID, USBAudioDevice, usb_audio_device_ns

DEPENDENCIES = ["usb_audio_device"]
USBMediaSource = usb_audio_device_ns.class_(
    "USBMediaSource", cg.Component, media_source.MediaSource
)
CONFIG_SCHEMA = (
    media_source.media_source_schema(USBMediaSource)
    .extend(
        {
            cv.GenerateID(CONF_USB_AUDIO_DEVICE_ID): cv.use_id(USBAudioDevice),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    cg.add_define("USE_USB_AUDIO_MEDIA_SOURCE")
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await media_source.register_media_source(var, config)
    parent = await cg.get_variable(config[CONF_USB_AUDIO_DEVICE_ID])
    cg.add(var.set_usb_audio_device(parent))
    cg.add(parent.set_playback_sink(var))
