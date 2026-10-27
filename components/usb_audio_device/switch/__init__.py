"""Microphone Mute switch."""

import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
import esphome.final_validate as fv

from .. import CONF_USB_AUDIO_DEVICE_ID, USBAudioDevice, usb_audio_device_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["usb_audio_device"]

USBMicrophoneMuteSwitch = usb_audio_device_ns.class_(
    "USBMicrophoneMuteSwitch", switch.Switch, cg.Component
)
CONFIG_SCHEMA = (
    switch.switch_schema(
        USBMicrophoneMuteSwitch,
        icon="mdi:microphone-off",
        default_restore_mode="DISABLED",
        block_inverted=True,
    )
    .extend({cv.GenerateID(CONF_USB_AUDIO_DEVICE_ID): cv.use_id(USBAudioDevice)})
    .extend(cv.COMPONENT_SCHEMA)
)


def final_validate(config):
    matching = [
        item
        for item in fv.full_config.get().get("switch", [])
        if item.get("platform") == "usb_audio_device"
        and item[CONF_USB_AUDIO_DEVICE_ID] == config[CONF_USB_AUDIO_DEVICE_ID]
    ]
    if len(matching) > 1:
        raise cv.Invalid("Only one microphone mute switch may control each parent")
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(await cg.get_variable(config[CONF_USB_AUDIO_DEVICE_ID])))
