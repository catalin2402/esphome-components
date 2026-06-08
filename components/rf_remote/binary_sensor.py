import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, remote_base

from . import RFRemoteBinarySensor
from .common import (
    CONF_RECEIVER_ID,
    CONF_REMOTE_ID,
    CONF_REMOTE_BUTTON,
    CONF_RESET_TIME,
    validate_remote_id,
)

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(
        RFRemoteBinarySensor,
    )
    .extend(
        {
            cv.GenerateID(CONF_RECEIVER_ID): cv.use_id(remote_base.RemoteReceiverBase),
            cv.Required(CONF_REMOTE_ID): validate_remote_id,
            cv.Required(CONF_REMOTE_BUTTON): cv.int_range(min=0, max=255),
            cv.Optional(
                CONF_RESET_TIME, default="100ms"
            ): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    cg.add(var.set_remote_id(int(config[CONF_REMOTE_ID], 16)))
    cg.add(var.set_remote_button(config[CONF_REMOTE_BUTTON]))
    cg.add(var.set_reset_time(config[CONF_RESET_TIME]))

    receiver = await cg.get_variable(config[CONF_RECEIVER_ID])
    cg.add(receiver.register_listener(var))
