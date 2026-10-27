"""USB audio device with playback and microphone capture."""

from esphome import automation
import esphome.codegen as cg
from esphome.components import audio, microphone, speaker
from esphome.components.esp32 import (
    add_idf_component,
    get_esp32_variant,
    include_builtin_idf_component,
)
import esphome.config_validation as cv
from esphome.const import (
    CONF_BITS_PER_SAMPLE,
    CONF_CHANNELS,
    CONF_ID,
    CONF_MICROPHONE,
    CONF_SAMPLE_RATE,
    CONF_SPEAKER,
    CONF_TRIGGER_ID,
)
from esphome.core import CORE
import esphome.final_validate as fv

CONF_ACTIVITY_THRESHOLD = "activity_threshold"
CONF_ACTIVITY_TIMEOUT = "activity_timeout"
CONF_CAPTURE = "capture"
CONF_MANUFACTURER = "manufacturer"
CONF_PLAYBACK = "playback"
CONF_PRODUCT = "product"
CONF_PRODUCT_ID = "product_id"
CONF_SERIAL = "serial"
CONF_VENDOR_ID = "vendor_id"
CONF_USB_AUDIO_DEVICE_ID = "usb_audio_device_id"
CONF_ON_CONNECTED = "on_connected"
CONF_ON_DISCONNECTED = "on_disconnected"
CONF_ON_PLAYBACK_STARTED = "on_playback_started"
CONF_ON_PLAYBACK_STOPPED = "on_playback_stopped"
CONF_ON_CAPTURE_STARTED = "on_capture_started"
CONF_ON_CAPTURE_STOPPED = "on_capture_stopped"
CONF_ON_PLAYBACK_ACTIVITY = "on_playback_activity"
CONF_ON_MICROPHONE_MUTE = "on_microphone_mute"

AUTO_LOAD = ["audio", "speaker"]
CODEOWNERS = ["@catalin2402"]
CONFLICTS_WITH = ["usb_host", "usb_audio", "tinyusb", "usb_cdc_acm"]
DEPENDENCIES = ["esp32"]
usb_audio_device_ns = cg.esphome_ns.namespace("usb_audio_device")
USBAudioDevice = usb_audio_device_ns.class_("USBAudioDevice", cg.Component)

ConnectedTrigger = usb_audio_device_ns.class_(
    "ConnectedTrigger", automation.Trigger.template()
)
DisconnectedTrigger = usb_audio_device_ns.class_(
    "DisconnectedTrigger", automation.Trigger.template()
)
PlaybackStartedTrigger = usb_audio_device_ns.class_(
    "PlaybackStartedTrigger", automation.Trigger.template()
)
PlaybackStoppedTrigger = usb_audio_device_ns.class_(
    "PlaybackStoppedTrigger", automation.Trigger.template()
)
CaptureStartedTrigger = usb_audio_device_ns.class_(
    "CaptureStartedTrigger", automation.Trigger.template()
)
CaptureStoppedTrigger = usb_audio_device_ns.class_(
    "CaptureStoppedTrigger", automation.Trigger.template()
)
PlaybackActivityTrigger = usb_audio_device_ns.class_(
    "PlaybackActivityTrigger", automation.Trigger.template(cg.bool_)
)
MicrophoneMuteTrigger = usb_audio_device_ns.class_(
    "MicrophoneMuteTrigger", automation.Trigger.template(cg.bool_)
)

TRIGGER_TYPES = {
    CONF_ON_CONNECTED: ConnectedTrigger,
    CONF_ON_DISCONNECTED: DisconnectedTrigger,
    CONF_ON_PLAYBACK_STARTED: PlaybackStartedTrigger,
    CONF_ON_PLAYBACK_STOPPED: PlaybackStoppedTrigger,
    CONF_ON_CAPTURE_STARTED: CaptureStartedTrigger,
    CONF_ON_CAPTURE_STOPPED: CaptureStoppedTrigger,
    CONF_ON_PLAYBACK_ACTIVITY: PlaybackActivityTrigger,
    CONF_ON_MICROPHONE_MUTE: MicrophoneMuteTrigger,
}

STREAM = cv.Schema(
    {
        cv.Optional(CONF_SAMPLE_RATE, default=48000): cv.one_of(48000, int=True),
        cv.Optional(CONF_BITS_PER_SAMPLE, default=16): cv.one_of(16, int=True),
    }
)


def validate_platform(config):
    if CORE.using_arduino or get_esp32_variant() != "ESP32S3":
        raise cv.Invalid("usb_audio_device currently requires ESP32-S3 and ESP-IDF")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(USBAudioDevice),
            cv.Optional(CONF_MANUFACTURER, default="ESPHome"): cv.string_strict,
            cv.Optional(CONF_PRODUCT, default="USB Headset"): cv.string_strict,
            cv.Optional(CONF_SERIAL, default="ESPHOME-UAC-1"): cv.string_strict,
            cv.Optional(CONF_VENDOR_ID, default=0x303A): cv.hex_uint16_t,
            cv.Optional(CONF_PRODUCT_ID, default=0x8001): cv.hex_uint16_t,
            cv.Optional(CONF_ACTIVITY_THRESHOLD, default=64): cv.int_range(
                min=1, max=32767
            ),
            cv.Optional(
                CONF_ACTIVITY_TIMEOUT, default="750ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_PLAYBACK, default={}): STREAM.extend(
                {
                    cv.Optional(CONF_SPEAKER): cv.use_id(speaker.Speaker),
                    cv.Optional(CONF_CHANNELS, default=2): cv.one_of(2, int=True),
                }
            ),
            cv.Optional(CONF_CAPTURE): STREAM.extend(
                {
                    cv.Required(CONF_MICROPHONE): cv.use_id(microphone.Microphone),
                    cv.Optional(CONF_CHANNELS, default=1): cv.one_of(1, int=True),
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(
        {
            cv.Optional(key): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(trigger)}
            )
            for key, trigger in TRIGGER_TYPES.items()
        }
    ),
    validate_platform,
)


def final_validate(config):
    full = fv.full_config.get()
    if full.get("logger", {}).get("hardware_uart") in ("USB_CDC", "USB_SERIAL_JTAG"):
        raise cv.Invalid(
            "Use logger.hardware_uart: UART0; USB belongs to usb_audio_device"
        )
    sources = [
        item
        for item in full.get("media_source", [])
        if item.get("platform") == "usb_audio_device"
        and item[CONF_USB_AUDIO_DEVICE_ID] == config[CONF_ID]
    ]
    if len(sources) + int(CONF_SPEAKER in config[CONF_PLAYBACK]) != 1:
        raise cv.Invalid(
            "Choose exactly one USB playback destination: playback.speaker or one usb_audio_device media_source"
        )
    if CONF_CAPTURE in config:
        audio.final_validate_audio_schema(
            "usb_audio_device capture",
            audio_device=CONF_MICROPHONE,
            bits_per_sample=16,
            channels=1,
            sample_rate=48000,
            audio_device_issue=True,
        )(config[CONF_CAPTURE])
    if CONF_SPEAKER in config[CONF_PLAYBACK]:
        audio.final_validate_audio_schema(
            "usb_audio_device playback",
            audio_device=CONF_SPEAKER,
            bits_per_sample=16,
            channels=2,
            sample_rate=48000,
            audio_device_issue=True,
        )(config[CONF_PLAYBACK])
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    if CORE.config.get("ota"):
        from esphome.components.ota import request_ota_state_listeners

        request_ota_state_listeners()
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    for key in TRIGGER_TYPES:
        args = []
        if key == CONF_ON_PLAYBACK_ACTIVITY:
            args = [(cg.bool_, "active")]
        elif key == CONF_ON_MICROPHONE_MUTE:
            args = [(cg.bool_, "muted")]
        for conf in config.get(key, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, args, conf)
    if CONF_SPEAKER in config[CONF_PLAYBACK]:
        cg.add(
            var.set_speaker(await cg.get_variable(config[CONF_PLAYBACK][CONF_SPEAKER]))
        )
    if CONF_CAPTURE in config:
        cg.add_define("USE_USB_AUDIO_DEVICE_MICROPHONE")
        cg.add(
            var.set_microphone(
                await cg.get_variable(config[CONF_CAPTURE][CONF_MICROPHONE])
            )
        )
    # TinyUSB is built separately, so descriptor settings must be build-wide.
    cg.add_build_flag(f"-DUAD_MIC_CHANNEL_NUM={int(CONF_CAPTURE in config)}")
    cg.add(var.set_activity_threshold(config[CONF_ACTIVITY_THRESHOLD]))
    cg.add(var.set_activity_timeout(config[CONF_ACTIVITY_TIMEOUT].total_milliseconds))
    # Use the full configured name for both iProduct and the audio function
    # string, since hosts differ in which descriptor they display.
    display_name = " ".join(
        value.strip()
        for value in (config[CONF_MANUFACTURER], config[CONF_PRODUCT])
        if value.strip()
    )
    cg.add(var.set_product(display_name))
    for field in (CONF_MANUFACTURER, CONF_SERIAL, CONF_VENDOR_ID, CONF_PRODUCT_ID):
        cg.add(getattr(var, f"set_{field}")(config[field]))
    add_idf_component(name="espressif/tinyusb", ref="0.19.0~3")
    # TinyUSB supports an explicit config header. ESPHome emits -D build flags
    # project-wide, so its IDF library and our C sources see identical settings.
    # Angle brackets avoid nested quote escaping in IDF's requirements pass.
    # Point at the generated copy: all translation units must include the same
    # physical headers, not both source-cache and generated copies of enums.
    config_header = (
        CORE.relative_build_path(
            "src/esphome/components/usb_audio_device/tusb_config.h"
        )
        .resolve()
        .as_posix()
    )
    cg.add_build_flag(f"-DCFG_TUSB_CONFIG_FILE=<{config_header}>")
    for component in ("usb", "esp_timer", "esp_ringbuf", "esp_netif"):
        include_builtin_idf_component(component)


for action, method in (
    ("microphone_mute", "set_microphone_muted(true)"),
    ("microphone_unmute", "set_microphone_muted(false)"),
):
    automation.register_apply_action(
        f"usb_audio_device.{action}",
        automation.maybe_simple_id({cv.GenerateID(): cv.use_id(USBAudioDevice)}),
        automation.ApplyCall(method),
    )

USB_AUDIO_ACTION_SCHEMA = automation.maybe_simple_id(
    {cv.GenerateID(): cv.use_id(USBAudioDevice)}
)
for name, call in (
    ("is_connected", "is_connected()"),
    ("is_playback_active", "is_playback_active()"),
    ("is_capture_active", "is_capture_active()"),
    ("is_microphone_muted", "is_microphone_muted()"),
):
    automation.register_apply_condition(
        f"usb_audio_device.{name}", USB_AUDIO_ACTION_SCHEMA, call
    )
