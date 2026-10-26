import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MICROPHONE
import esphome.final_validate as fv

from .. import CONF_USB_MEDIA_PLAYER_ID, USBMediaPlayer, usb_media_player_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["usb_media_player"]

USBMicrophoneSwitch = usb_media_player_ns.class_(
    "USBMicrophoneSwitch", switch.Switch, cg.Component
)

CONFIG_SCHEMA = (
    switch.switch_schema(
        USBMicrophoneSwitch,
        icon="mdi:microphone",
        block_inverted=True,
        default_restore_mode="ALWAYS_OFF",
    )
    .extend({cv.GenerateID(CONF_USB_MEDIA_PLAYER_ID): cv.use_id(USBMediaPlayer)})
    .extend(cv.COMPONENT_SCHEMA)
)


def final_validate(config):
    full_config = fv.full_config.get()
    path = full_config.get_path_for_id(config[CONF_USB_MEDIA_PLAYER_ID])
    if CONF_MICROPHONE not in full_config.get_config_for_path(path[:-1]):
        raise cv.Invalid(
            "The USB microphone switch requires a microphone on its usb_media_player hub"
        )
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)
    parent = await cg.get_variable(config[CONF_USB_MEDIA_PLAYER_ID])
    cg.add(var.set_parent(parent))
