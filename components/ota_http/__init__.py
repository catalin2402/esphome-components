import urllib.parse as urlparse

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import (
    CONF_ID,
    CONF_URL,
)
from esphome.core import Lambda, CORE

DEPENDENCIES = ["network"]
AUTO_LOAD = ["md5"]

ota_http_ns = cg.esphome_ns.namespace("ota_http")
OtaHttpComponent = ota_http_ns.class_("OtaHttpComponent", cg.Component)

OtaHttpFlashAction = ota_http_ns.class_("OtaHttpFlashAction", automation.Action)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(OtaHttpComponent),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    if CORE.is_esp8266:
        cg.add_library("ESP8266HTTPClient", None)
        cg.add_library("ESP8266httpUpdate", None)

    await cg.register_component(var, config)


OTA_HTTP_FLASH_ACTION_SCHEMA = automation.maybe_conf(CONF_URL)


@automation.register_action(
    "ota_http.flash",
    OtaHttpFlashAction,
    automation.maybe_simple_id(
        cv.Schema(
            {
                cv.Required(CONF_ID): cv.use_id(OtaHttpComponent),
                cv.Required(CONF_URL): cv.templatable(cv.string),
            }
        )
    ),
)
async def ota_http_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)

    template_ = await cg.templatable(config[CONF_URL], args, cg.std_string)
    cg.add(var.set_url(template_))
    return var
