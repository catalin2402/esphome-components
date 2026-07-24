import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, remote_base

from . import RFRemoteBinarySensor
from .common import (
    CONF_REMOTE_ID,
    CONF_REMOTE_BUTTON,
    CONF_RESET_TIME,
    validate_remote_id,
)

CODEOWNERS = ["@catalin2402"]

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(
        RFRemoteBinarySensor,
    )
    .extend(
        {
            cv.Required(CONF_REMOTE_ID): validate_remote_id,
            cv.Required(CONF_REMOTE_BUTTON): cv.int_range(min=0, max=0x3F),
            cv.Optional(
                CONF_RESET_TIME, default="50ms"
            ): cv.positive_time_period_milliseconds,
        }
    )
    .extend(remote_base.REMOTE_LISTENER_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
    await remote_base.register_listener(var, config)

    cg.add(var.set_remote_id(config[CONF_REMOTE_ID]))
    cg.add(var.set_remote_button(config[CONF_REMOTE_BUTTON]))
    cg.add(var.set_reset_time(config[CONF_RESET_TIME]))
