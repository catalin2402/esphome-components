from esphome import pins
import esphome.codegen as cg
from esphome.components import ota
from esphome.components.esp32 import (
    add_idf_component,
    add_idf_sdkconfig_option,
    include_builtin_idf_component,
)
from esphome.components.esp32.const import VARIANT_ESP32S3
from esphome.components.esp32 import get_esp32_variant
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import HexInt

DEPENDENCIES = ["esp32"]
CODEOWNERS = []
MULTI_CONF = False

CONF_BCLK_PIN = "bclk_pin"
CONF_LRCLK_PIN = "lrclk_pin"
CONF_DAC_DOUT_PIN = "dac_dout_pin"
CONF_SAMPLE_RATE = "sample_rate"
CONF_OUTPUT_GAIN_DB = "output_gain_db"
CONF_MANUFACTURER = "manufacturer"
CONF_PRODUCT_NAME = "product_name"
CONF_SERIAL_NUMBER = "serial_number"
CONF_USB_VID = "usb_vid"
CONF_USB_PID = "usb_pid"

usb_audio_bridge_ns = cg.esphome_ns.namespace("usb_audio_bridge")
USBAudioBridge = usb_audio_bridge_ns.class_("USBAudioBridge", cg.Component)
UAC_IDF_COMPONENT_VERSION = "1.3.1"


def _usb_string(value):
    value = cv.string_strict(value)
    if not value:
        raise cv.Invalid("USB descriptor strings must not be empty")
    if len(value) > 20:
        raise cv.Invalid("USB descriptor strings must be 20 characters or fewer")
    return value


def _validate_variant(config):
    if get_esp32_variant() != VARIANT_ESP32S3:
        raise cv.Invalid("usb_audio_bridge currently supports only ESP32-S3")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(USBAudioBridge),
            cv.Required(CONF_BCLK_PIN): pins.internal_gpio_output_pin_number,
            cv.Required(CONF_LRCLK_PIN): pins.internal_gpio_output_pin_number,
            cv.Required(CONF_DAC_DOUT_PIN): pins.internal_gpio_output_pin_number,
            cv.Optional(CONF_SAMPLE_RATE, default=48000): cv.one_of(48000, int=True),
            cv.Optional(CONF_OUTPUT_GAIN_DB, default=0.0): cv.float_range(min=-60.0, max=0.0),
            cv.Optional(CONF_MANUFACTURER, default="ASUS"): _usb_string,
            cv.Optional(CONF_PRODUCT_NAME, default="Asus Strix F700"): _usb_string,
            cv.Optional(CONF_SERIAL_NUMBER, default="STRIXF7001"): _usb_string,
            cv.Optional(CONF_USB_VID, default=0x303A): cv.int_range(min=0, max=0xFFFF),
            cv.Optional(CONF_USB_PID, default=0x8000): cv.int_range(min=0, max=0xFFFF),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate_variant,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Suspend I2S output while any configured OTA platform is writing flash.
    ota.request_ota_state_listeners()

    cg.add(var.set_bclk_pin(config[CONF_BCLK_PIN]))
    cg.add(var.set_lrclk_pin(config[CONF_LRCLK_PIN]))
    cg.add(var.set_dac_dout_pin(config[CONF_DAC_DOUT_PIN]))
    cg.add(var.set_sample_rate(config[CONF_SAMPLE_RATE]))
    cg.add(var.set_output_gain_db(config[CONF_OUTPUT_GAIN_DB]))

    # Pull the unmodified component from the Espressif Component Registry.
    add_idf_component(
        name="espressif/usb_device_uac", ref=UAC_IDF_COMPONENT_VERSION
    )

    include_builtin_idf_component("esp_driver_i2s")

    add_idf_sdkconfig_option("CONFIG_USB_DEVICE_UAC_AS_PART", False)
    add_idf_sdkconfig_option("CONFIG_UAC_TUSB_VID", HexInt(config[CONF_USB_VID]))
    add_idf_sdkconfig_option("CONFIG_UAC_TUSB_PID", HexInt(config[CONF_USB_PID]))
    add_idf_sdkconfig_option("CONFIG_UAC_TUSB_MANUFACTURER", config[CONF_MANUFACTURER])
    add_idf_sdkconfig_option("CONFIG_UAC_TUSB_PRODUCT", config[CONF_PRODUCT_NAME])
    add_idf_sdkconfig_option("CONFIG_UAC_TUSB_SERIAL_NUM", config[CONF_SERIAL_NUMBER])
    add_idf_sdkconfig_option("CONFIG_UAC_SPEAKER_CHANNEL_NUM", 2)
    add_idf_sdkconfig_option("CONFIG_UAC_MIC_CHANNEL_NUM", 0)
    add_idf_sdkconfig_option("CONFIG_UAC_SAMPLE_RATE", config[CONF_SAMPLE_RATE])
    add_idf_sdkconfig_option("CONFIG_UAC_BIT_RESOLUTION_16", True)
    add_idf_sdkconfig_option("CONFIG_UAC_SUPPORT_MACOS", False)
    add_idf_sdkconfig_option("CONFIG_TINYUSB_RHPORT_FS", True)
    add_idf_sdkconfig_option("CONFIG_I2S_ISR_IRAM_SAFE", True)
