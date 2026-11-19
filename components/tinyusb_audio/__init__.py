"""USB Audio Class device for ESP32 native USB OTG ports."""

import esphome.codegen as cg
from esphome.components import audio, microphone, tinyusb
from esphome.components.esp32 import (
    VARIANT_ESP32P4,
    VARIANT_ESP32S2,
    VARIANT_ESP32S3,
    VARIANT_ESP32S31,
    add_idf_sdkconfig_option,
    get_esp32_variant,
    idf_version,
    only_on_variant,
)
import esphome.config_validation as cv
from esphome.const import (
    CONF_BITS_PER_SAMPLE,
    CONF_CHANNELS,
    CONF_ID,
    CONF_MICROPHONE,
    CONF_SAMPLE_RATE,
    CONF_SPEAKER,
)
from esphome.core import CORE
import esphome.final_validate as fv
from esphome.types import ConfigType

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["tinyusb"]
AUTO_LOAD = ["audio"]
CONFLICTS_WITH = ["usb_host", "usb_cdc_acm"]
DOMAIN = "tinyusb_audio"

CONF_PLAYBACK = "playback"
CONF_CAPTURE = "capture"
CONF_KEEP_ALIVE = "keep_alive"
CONF_TINYUSB_ID = "tinyusb_id"
CONF_TINYUSB_AUDIO_ID = "tinyusb_audio_id"

# The official hub exposes a descriptor setter but keeps a fixed class-provider allowlist.
if DOMAIN not in tinyusb._USB_CLASS_COMPONENTS:
    tinyusb._USB_CLASS_COMPONENTS += (DOMAIN,)

tinyusb_audio_ns = cg.esphome_ns.namespace(DOMAIN)
TinyUSBAudio = tinyusb_audio_ns.class_("TinyUSBAudio", cg.Component)

PCM_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_SAMPLE_RATE): cv.int_range(min=1000, max=192000),
        cv.Optional(CONF_BITS_PER_SAMPLE): cv.one_of(16, 24, 32, int=True),
    }
)


def _validate_version(config: ConfigType) -> ConfigType:
    cv.validate_esphome_version("2026.11.0")
    if CORE.using_arduino:
        raise cv.Invalid("tinyusb_audio requires ESP-IDF")
    if get_esp32_variant() == VARIANT_ESP32S31 and idf_version() < cv.Version(6, 1, 0):
        raise cv.Invalid("ESP32-S31 requires ESP-IDF 6.1 or newer")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TinyUSBAudio),
            cv.GenerateID(CONF_TINYUSB_ID): cv.use_id(tinyusb.TinyUSB),
            cv.Optional(CONF_PLAYBACK, default={}): PCM_SCHEMA.extend(
                {cv.Optional(CONF_KEEP_ALIVE, default=False): cv.boolean}
            ),
            cv.Optional(CONF_CAPTURE): PCM_SCHEMA.extend(
                {
                    cv.Required(CONF_MICROPHONE): cv.use_id(microphone.Microphone),
                    cv.Optional(CONF_CHANNELS, default=1): cv.one_of(1, 2, int=True),
                }
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    only_on_variant(
        supported=[
            VARIANT_ESP32S2,
            VARIANT_ESP32S3,
            VARIANT_ESP32P4,
            VARIANT_ESP32S31,
        ]
    ),
    _validate_version,
)


def _packet_bytes(config: ConfigType, channels: int) -> int:
    return (
        (config[CONF_SAMPLE_RATE] // 1000 + 1)
        * channels
        * (config[CONF_BITS_PER_SAMPLE] // 8)
    )


def _device_format_value(
    device: ConfigType, field: str, min_field: str, max_field: str
) -> int | None:
    minimum = device.get(min_field)
    return (
        minimum
        if minimum == device.get(max_field) and minimum is not None
        else device.get(field)
    )


def _device_format(device: ConfigType) -> tuple[int | None, int | None]:
    rate = _device_format_value(
        device, CONF_SAMPLE_RATE, audio.CONF_MIN_SAMPLE_RATE, audio.CONF_MAX_SAMPLE_RATE
    )
    bits = _device_format_value(
        device,
        CONF_BITS_PER_SAMPLE,
        audio.CONF_MIN_BITS_PER_SAMPLE,
        audio.CONF_MAX_BITS_PER_SAMPLE,
    )
    return rate, bits


def _infer_format(
    stream: ConfigType, formats: list[tuple[int | None, int | None]]
) -> None:
    for index, (field, default) in enumerate(
        ((CONF_SAMPLE_RATE, 48000), (CONF_BITS_PER_SAMPLE, 16))
    ):
        if field in stream:
            continue
        values = {fmt[index] for fmt in formats if fmt[index] is not None}
        if field == CONF_BITS_PER_SAMPLE:
            values.intersection_update((16, 24, 32))
        if len(values) > 1:
            raise cv.Invalid(
                f"Specify {field} when the USB playback pipelines disagree",
                path=[field],
            )
        stream[field] = values.pop() if values else default
    PCM_SCHEMA(
        {field: stream[field] for field in (CONF_SAMPLE_RATE, CONF_BITS_PER_SAMPLE)}
    )


def _final_validate(config: ConfigType) -> None:
    full_config = fv.full_config.get()
    sources = fv.platform_entries(
        domain="media_source",
        platform=DOMAIN,
        match={CONF_TINYUSB_AUDIO_ID: config[CONF_ID]},
    )
    if len(sources) != 1:
        raise cv.Invalid("Configure exactly one tinyusb_audio media source per hub")

    playback = config[CONF_PLAYBACK]
    capture = config.get(CONF_CAPTURE)
    source_id = sources[0][CONF_ID]
    pipelines = [
        player[key]
        for player in full_config.get("media_player", [])
        if player.get("platform") == "speaker_source"
        for key in ("media_pipeline", "announcement_pipeline")
        if key in player and source_id in player[key]["sources"]
    ]
    playback_formats = []
    for pipeline in pipelines:
        rate, bits = _device_format(fv.get_declaring_config(pipeline[CONF_SPEAKER]))
        playback_formats.append((pipeline.get(CONF_SAMPLE_RATE, rate), bits))
    with cv.prepend_path([CONF_PLAYBACK]):
        _infer_format(playback, playback_formats)

    if capture:
        mic = fv.get_declaring_config(capture[CONF_MICROPHONE])
        rate, bits = _device_format(mic)
        with cv.prepend_path([CONF_CAPTURE]):
            _infer_format(capture, [(rate, 16 if mic.get("pdm") else bits)])

    out_bytes = _packet_bytes(playback, 2)
    in_bytes = _packet_bytes(capture, capture[CONF_CHANNELS]) if capture else 0
    if max(out_bytes, in_bytes) > 1023:
        raise cv.Invalid("The PCM format exceeds the full-speed USB packet limit")

    # The DWC2 RX FIFO holds two packets of the largest OUT endpoint (including
    # EP0). The TX FIFOs hold EP0, feedback and capture; reserve DMA metadata too.
    variant = get_esp32_variant()
    fifo_words, endpoints = (
        (1024, 16) if variant in (VARIANT_ESP32P4, VARIANT_ESP32S31) else (256, 7)
    )
    rx_words = 16 + 2 * endpoints + 2 * ((max(64, out_bytes) + 3) // 4)
    tx_words = 16 + 1 + (in_bytes + 3) // 4
    if "tinyusb_keyboard" in full_config:
        tx_words += 3  # An eight-byte keyboard report plus its report ID.
    if rx_words + tx_words + 2 * endpoints > fifo_words:
        raise cv.Invalid(f"The PCM formats exceed the {variant} USB FIFO capacity")

    for pipeline in pipelines:
        audio.final_validate_audio_schema(
            "tinyusb_audio playback",
            audio_device=CONF_SPEAKER,
            sample_rate=playback[CONF_SAMPLE_RATE],
            channels=2,
            bits_per_sample=playback[CONF_BITS_PER_SAMPLE],
        )(pipeline)

    if capture:
        microphone.final_validate_microphone_source_schema(
            DOMAIN, sample_rate=capture[CONF_SAMPLE_RATE]
        )(
            {
                CONF_MICROPHONE: capture[CONF_MICROPHONE],
                CONF_CHANNELS: list(range(capture[CONF_CHANNELS])),
            }
        )


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    usb = await cg.get_variable(config[CONF_TINYUSB_ID])
    cg.add(usb.set_full_speed_config(tinyusb_audio_ns.CONFIGURATION_DESCRIPTOR))
    if "tinyusb_keyboard" in CORE.config:
        cg.add_define("USE_TINYUSB_AUDIO_KEYBOARD")

    playback = config[CONF_PLAYBACK]
    capture = config.get(CONF_CAPTURE)
    cg.add(var.set_playback_keep_alive(playback[CONF_KEEP_ALIVE]))
    cg.add_build_flag(f"-DTINYUSB_AUDIO_PLAYBACK_RATE={playback[CONF_SAMPLE_RATE]}")
    cg.add_build_flag(f"-DTINYUSB_AUDIO_PLAYBACK_BITS={playback[CONF_BITS_PER_SAMPLE]}")
    cg.add_build_flag(f"-DTINYUSB_AUDIO_CAPTURE={int(capture is not None)}")
    if capture:
        cg.add_build_flag(f"-DTINYUSB_AUDIO_CAPTURE_CHANNELS={capture[CONF_CHANNELS]}")
        cg.add_build_flag(f"-DTINYUSB_AUDIO_CAPTURE_RATE={capture[CONF_SAMPLE_RATE]}")
        cg.add_build_flag(
            f"-DTINYUSB_AUDIO_CAPTURE_BITS={capture[CONF_BITS_PER_SAMPLE]}"
        )
        await cg.add_variable(var.set_microphone, capture[CONF_MICROPHONE])

    # TinyUSB and the application must see the same class and endpoint sizes.
    header = (
        CORE.relative_build_path(
            "src/esphome/components/tinyusb_audio/tinyusb_audio_config.h"
        )
        .resolve()
        .as_posix()
    )
    cg.add_build_flag(f"-DCFG_TUSB_CONFIG_FILE=<{header}>")
    cg.add_build_flag("-Wl,--wrap=tinyusb_driver_install")
    add_idf_sdkconfig_option("CONFIG_TINYUSB_SUSPEND_CALLBACK", True)
    add_idf_sdkconfig_option("CONFIG_TINYUSB_RESUME_CALLBACK", True)
