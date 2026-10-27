"""USB HID media controls on a usb_audio_device composite device."""

from esphome import automation
import esphome.codegen as cg
from esphome.components.usb_audio_device import (
    CONF_USB_AUDIO_DEVICE_ID,
    USBAudioDevice,
    usb_audio_device_ns,
)
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_KEY

CONF_USB_MEDIA_CONTROL_ID = "usb_media_control_id"

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["usb_audio_device"]
usb_media_control_ns = cg.esphome_ns.namespace("usb_media_control")
USBMediaControl = usb_media_control_ns.class_("USBMediaControl", cg.Component)
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(USBMediaControl),
        cv.GenerateID(CONF_USB_AUDIO_DEVICE_ID): cv.use_id(USBAudioDevice),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_USB_AUDIO_DEVICE_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(parent.set_media_transport(var))
    # Shared with the TinyUSB library and the composite descriptor source.
    cg.add_build_flag("-DUAD_MEDIA_CONTROL=1")


MediaKey = usb_audio_device_ns.enum("MediaKey")
MEDIA_KEYS = {
    "PLAY": MediaKey.MEDIA_KEY_PLAY,
    "PAUSE": MediaKey.MEDIA_KEY_PAUSE,
    "FAST_FORWARD": MediaKey.MEDIA_KEY_FAST_FORWARD,
    "REWIND": MediaKey.MEDIA_KEY_REWIND,
    "NEXT": MediaKey.MEDIA_KEY_NEXT,
    "PREVIOUS": MediaKey.MEDIA_KEY_PREVIOUS,
    "STOP": MediaKey.MEDIA_KEY_STOP,
    "PLAY_PAUSE": MediaKey.MEDIA_KEY_PLAY_PAUSE,
    "VOLUME_UP": MediaKey.MEDIA_KEY_VOLUME_UP,
    "VOLUME_DOWN": MediaKey.MEDIA_KEY_VOLUME_DOWN,
}

automation.register_apply_action(
    "usb_media_control.media_key",
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(USBMediaControl),
            cv.Required(CONF_KEY): cv.enum(MEDIA_KEYS, upper=True),
        }
    ),
    automation.ApplyField(CONF_KEY, "send_media_key", cg.uint16),
)

for action, method in (
    ("media_key_press", "press_media_key"),
    ("media_key_release", "release_media_key"),
):
    automation.register_apply_action(
        f"usb_media_control.{action}",
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(USBMediaControl),
                cv.Required(CONF_KEY): cv.enum(MEDIA_KEYS, upper=True),
            }
        ),
        automation.ApplyField(CONF_KEY, method, cg.uint16),
    )

automation.register_apply_action(
    "usb_media_control.media_key_release_all",
    automation.maybe_simple_id({cv.GenerateID(): cv.use_id(USBMediaControl)}),
    automation.ApplyCall("release_all_media_keys()"),
)

automation.register_apply_condition(
    "usb_media_control.is_ready",
    automation.maybe_simple_id({cv.GenerateID(): cv.use_id(USBMediaControl)}),
    "is_ready()",
)
