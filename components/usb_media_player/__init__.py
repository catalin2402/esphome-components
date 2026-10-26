from pathlib import Path

from esphome import automation
import esphome.codegen as cg
from esphome.components import media_source, microphone, ota
from esphome.components.esp32 import (
    add_idf_component,
    add_idf_sdkconfig_option,
    get_esp32_variant,
    include_builtin_idf_component,
)
from esphome.components.esp32.const import VARIANT_ESP32S3
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MICROPHONE, CONF_SAMPLE_RATE, CONF_TRIGGER_ID
from esphome.core import CORE, HexInt
import esphome.final_validate as fv

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["audio"]

CONF_USB_MEDIA_PLAYER_ID = "usb_media_player_id"
CONF_MICROPHONE_BUFFER_SIZE = "microphone_buffer_size"
CONF_MANUFACTURER = "manufacturer"
CONF_PRODUCT_NAME = "product_name"
CONF_SERIAL_NUMBER = "serial_number"
CONF_USB_VID = "usb_vid"
CONF_USB_PID = "usb_pid"
CONF_SUPPORT_MACOS = "support_macos"

usb_media_player_ns = cg.esphome_ns.namespace("usb_media_player")
USBMediaPlayer = usb_media_player_ns.class_("USBMediaPlayer", cg.Component)
USBMediaSource = usb_media_player_ns.class_(
    "USBMediaSource", cg.Component, media_source.MediaSource
)
MediaKey = usb_media_player_ns.enum("MediaKey", is_class=True)
MediaKeyAction = usb_media_player_ns.class_("MediaKeyAction", automation.Action)
MediaKeyPressedAction = usb_media_player_ns.class_(
    "MediaKeyPressedAction", automation.Action
)
MediaKeyReleasedAction = usb_media_player_ns.class_(
    "MediaKeyReleasedAction", automation.Action
)
ConnectedCondition = usb_media_player_ns.class_(
    "ConnectedCondition", automation.Condition
)
StreamingCondition = usb_media_player_ns.class_(
    "StreamingCondition", automation.Condition
)
MicrophoneEnabledCondition = usb_media_player_ns.class_(
    "MicrophoneEnabledCondition", automation.Condition
)
MicrophoneAction = usb_media_player_ns.class_("MicrophoneAction", automation.Action)

HUB_SCHEMA = automation.maybe_simple_id({cv.GenerateID(): cv.use_id(USBMediaPlayer)})

TRIGGERS = {
    "on_connected": usb_media_player_ns.class_(
        "ConnectedTrigger", automation.Trigger.template()
    ),
    "on_disconnected": usb_media_player_ns.class_(
        "DisconnectedTrigger", automation.Trigger.template()
    ),
    "on_stream_started": usb_media_player_ns.class_(
        "StreamStartedTrigger", automation.Trigger.template()
    ),
    "on_stream_stopped": usb_media_player_ns.class_(
        "StreamStoppedTrigger", automation.Trigger.template()
    ),
}


def usb_string(value):
    value = cv.string_strict(value)
    if not value or len(value) > 20 or not value.isascii() or not value.isprintable():
        raise cv.Invalid(
            "USB descriptor strings must contain 1-20 printable ASCII characters"
        )
    return value


def validate_config(config):
    if CORE.target_framework != "esp-idf" or get_esp32_variant() != VARIANT_ESP32S3:
        raise cv.Invalid(
            "usb_media_player requires an ESP32-S3 with the ESP-IDF framework"
        )
    if CONF_MICROPHONE_BUFFER_SIZE in config and CONF_MICROPHONE not in config:
        raise cv.Invalid("microphone_buffer_size requires a microphone")
    if config.get(CONF_MICROPHONE_BUFFER_SIZE, 0) % 2:
        raise cv.Invalid("microphone_buffer_size must be a multiple of 2")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(USBMediaPlayer),
            cv.Optional(CONF_SAMPLE_RATE, default=48000): cv.one_of(48000, int=True),
            cv.Optional(CONF_MICROPHONE): microphone.microphone_source_schema(
                min_bits_per_sample=16,
                max_bits_per_sample=16,
                min_channels=1,
                max_channels=1,
            ),
            cv.Optional(CONF_MICROPHONE_BUFFER_SIZE): cv.int_range(min=1920, max=65536),
            cv.Optional(CONF_MANUFACTURER, default="ESPHome"): usb_string,
            cv.Optional(CONF_PRODUCT_NAME, default="USB Media Player"): usb_string,
            cv.Optional(CONF_SERIAL_NUMBER, default="ESPHomeUSB1"): usb_string,
            cv.Optional(CONF_USB_VID, default=0x303A): cv.int_range(min=1, max=0xFFFF),
            cv.Optional(CONF_USB_PID, default=0x8000): cv.int_range(min=1, max=0xFFFF),
            cv.Optional(CONF_SUPPORT_MACOS, default=False): cv.boolean,
            **{
                cv.Optional(key): automation.validate_automation(
                    {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(trigger)}
                )
                for key, trigger in TRIGGERS.items()
            },
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
    validate_config,
)


def final_validate(config):
    full_config = fv.full_config.get()
    if "usb_audio_bridge" in full_config:
        raise cv.Invalid(
            "usb_media_player and usb_audio_bridge cannot share the USB controller"
        )
    sources = [
        source
        for source in full_config.get("media_source", [])
        if source.get("platform") == "usb_media_player"
    ]
    if len(sources) > 1:
        raise cv.Invalid("Only one USB playback media source is supported")
    if not sources and CONF_MICROPHONE not in config:
        raise cv.Invalid("Configure a USB playback media source, a microphone, or both")
    if CONF_MICROPHONE in config:
        mic_config = config[CONF_MICROPHONE]
        microphone.final_validate_microphone_source_schema(
            "usb_media_player", sample_rate=48000
        )(mic_config)
        path = full_config.get_path_for_id(mic_config[CONF_MICROPHONE])
        mic = full_config.get_config_for_path(path[:-1])
        if mic.get("platform") != "i2s_audio":
            raise cv.Invalid(
                "USB microphone input currently supports i2s_audio microphones only"
            )
    logger_config = full_config.get("logger", {})
    if str(logger_config.get("hardware_uart", "")) in ("USB_CDC", "USB_SERIAL_JTAG"):
        raise cv.Invalid(
            "USB audio needs the native USB controller; use logger hardware_uart: UART0"
        )
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if CONF_MICROPHONE in config:
        cg.add_define("USE_USB_MEDIA_PLAYER_MICROPHONE")
        mic = await microphone.microphone_source_to_code(config[CONF_MICROPHONE])
        cg.add(var.set_microphone(mic))
        cg.add(
            var.set_microphone_buffer_size(
                config.get(CONF_MICROPHONE_BUFFER_SIZE, 8192)
            )
        )
    ota.request_ota_state_listeners()
    include_builtin_idf_component("esp_ringbuf")
    add_idf_component(name="espressif/usb_device_uac", ref="1.3.1")
    add_idf_component(
        name="usb_audio_hid",
        path=str(Path(__file__).resolve().parent / "usb_audio_hid"),
    )
    has_playback = any(
        source.get("platform") == "usb_media_player"
        for source in CORE.config.get("media_source", [])
    )
    for key, value in {
        "CONFIG_USB_DEVICE_UAC_AS_PART": False,
        "CONFIG_UAC_TUSB_VID": HexInt(config[CONF_USB_VID]),
        "CONFIG_UAC_TUSB_PID": HexInt(config[CONF_USB_PID]),
        "CONFIG_UAC_TUSB_MANUFACTURER": config[CONF_MANUFACTURER],
        "CONFIG_UAC_TUSB_PRODUCT": config[CONF_PRODUCT_NAME],
        "CONFIG_UAC_TUSB_SERIAL_NUM": config[CONF_SERIAL_NUMBER],
        "CONFIG_UAC_SPEAKER_CHANNEL_NUM": 2 if has_playback else 0,
        "CONFIG_UAC_MIC_CHANNEL_NUM": 1 if CONF_MICROPHONE in config else 0,
        "CONFIG_UAC_SAMPLE_RATE": config[CONF_SAMPLE_RATE],
        "CONFIG_UAC_BIT_RESOLUTION_16": True,
        "CONFIG_UAC_SUPPORT_MACOS": config[CONF_SUPPORT_MACOS],
        "CONFIG_TINYUSB_RHPORT_FS": True,
    }.items():
        add_idf_sdkconfig_option(key, value)
    for key in TRIGGERS:
        for conf in config.get(key, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, [], conf)


def register_key_action(name, action_type, key):
    @automation.register_action(
        f"usb_media_player.{name}", action_type, HUB_SCHEMA, synchronous=True
    )
    async def key_to_code(config, action_id, template_arg, args):
        parent = await cg.get_variable(config[CONF_ID])
        return cg.new_Pvariable(action_id, template_arg, parent, key)


for name, key in (
    ("volume_up", MediaKey.VOLUME_UP),
    ("volume_down", MediaKey.VOLUME_DOWN),
    ("next_track", MediaKey.NEXT_TRACK),
    ("previous_track", MediaKey.PREVIOUS_TRACK),
    ("play_pause", MediaKey.PLAY_PAUSE),
    ("play", MediaKey.PLAY),
    ("pause", MediaKey.PAUSE),
    ("stop", MediaKey.STOP),
):
    register_key_action(name, MediaKeyAction, key)
    if name in ("volume_up", "volume_down"):
        register_key_action(f"{name}_pressed", MediaKeyPressedAction, key)
        register_key_action(f"{name}_released", MediaKeyReleasedAction, key)


@automation.register_condition(
    "usb_media_player.is_connected", ConnectedCondition, HUB_SCHEMA
)
async def connected_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "usb_media_player.is_streaming", StreamingCondition, HUB_SCHEMA
)
async def streaming_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "usb_media_player.microphone_enabled", MicrophoneEnabledCondition, HUB_SCHEMA
)
async def microphone_enabled_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


def register_microphone_action(name, enabled):
    @automation.register_action(
        f"usb_media_player.{name}", MicrophoneAction, HUB_SCHEMA, synchronous=True
    )
    async def microphone_to_code(config, action_id, template_arg, args):
        parent = await cg.get_variable(config[CONF_ID])
        return cg.new_Pvariable(action_id, template_arg, parent, enabled)


register_microphone_action("enable_microphone", True)
register_microphone_action("disable_microphone", False)
