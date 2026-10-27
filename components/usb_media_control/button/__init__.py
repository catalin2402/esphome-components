"""USB HID media key buttons."""

import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_KEY

from .. import (
    CONF_USB_MEDIA_CONTROL_ID,
    MEDIA_KEYS,
    USBMediaControl,
    usb_media_control_ns,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["usb_media_control"]

USBMediaControlButton = usb_media_control_ns.class_(
    "USBMediaControlButton", button.Button
)
CONFIG_SCHEMA = button.button_schema(USBMediaControlButton).extend(
    {
        cv.GenerateID(CONF_USB_MEDIA_CONTROL_ID): cv.use_id(USBMediaControl),
        cv.Required(CONF_KEY): cv.enum(MEDIA_KEYS, upper=True),
    }
)


async def to_code(config):
    var = await button.new_button(config)
    cg.add(var.set_parent(await cg.get_variable(config[CONF_USB_MEDIA_CONTROL_ID])))
    cg.add(var.set_key(config[CONF_KEY]))
