import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32, media_source
from esphome.components.esp32.const import VARIANT_ESP32
from esphome.const import CONF_ID, CONF_NAME, CONF_BUFFER_SIZE
import esphome.final_validate as fv
from esphome.core import CORE

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["audio", "media_source"]
CONF_A2DP_SINK_ID = "a2dp_sink_id"
ns = cg.esphome_ns.namespace("a2dp_sink")
A2DPSink = ns.class_("A2DPSink", cg.Component, media_source.MediaSource)

CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(A2DPSink),
        cv.Required(CONF_NAME): cv.Length(min=1, max=248),
        cv.Optional("minimal_bluetooth", default=True): cv.boolean,
        cv.Optional(CONF_BUFFER_SIZE, default=32768): cv.int_range(min=8192, max=131072),
    }).extend(cv.COMPONENT_SCHEMA),
    cv.only_with_framework("esp-idf"),
    esp32.only_on_variant(supported=[VARIANT_ESP32]),
)


def final_validate(config):
    ble = fv.full_config.get().get("esp32_ble")
    if config["minimal_bluetooth"]:
        if ble is not None:
            raise cv.Invalid(
                "With esp32_ble, set minimal_bluetooth: false and esp32_ble.classic_bt: true"
            )
    elif ble is None or not ble.get("classic_bt", False):
        raise cv.Invalid(
            "minimal_bluetooth: false requires esp32_ble with classic_bt: true "
            "(ESPHome PR #21408 or a version containing that support)"
        )
    return config


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    esp32.include_builtin_idf_component("bt")
    for option, value in {
        "CONFIG_BT_ENABLED": True,
        "CONFIG_BT_BLUEDROID_ENABLED": True,
        "CONFIG_BT_CLASSIC_ENABLED": True,
        "CONFIG_BT_A2DP_ENABLE": True,
        "CONFIG_BT_A2DP_USE_EXTERNAL_CODEC": False,
        "CONFIG_BT_NIMBLE_ENABLED": False,
        "CONFIG_BT_SPP_ENABLED": False,
        "CONFIG_BT_HFP_ENABLE": False,
        "CONFIG_BT_HID_ENABLED": False,
        "CONFIG_BT_AVRCP_CT_COVER_ART_ENABLED": False,
    }.items():
        esp32.add_idf_sdkconfig_option(option, value)
    if config["minimal_bluetooth"]:
        for option, value in {
            "CONFIG_BTDM_CTRL_MODE_BR_EDR_ONLY": True,
            "CONFIG_BT_BLE_ENABLED": False,
            "CONFIG_BT_GATTC_ENABLE": False,
            "CONFIG_BT_GATTS_ENABLE": False,
            "CONFIG_BT_STACK_NO_LOG": True,
        }.items():
            esp32.add_idf_sdkconfig_option(option, value)
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_name(config[CONF_NAME]))
    cg.add(var.set_buffer_size(config[CONF_BUFFER_SIZE]))
    if not config["minimal_bluetooth"]:
        from esphome.components import esp32_ble

        cg.add_define("USE_A2DP_SHARED_BLUETOOTH")
        parent = await cg.get_variable(CORE.config["esp32_ble"][CONF_ID])
        cg.add(var.set_ble_parent(parent))
        esp32_ble.register_ble_status_event_handler(parent, var)
