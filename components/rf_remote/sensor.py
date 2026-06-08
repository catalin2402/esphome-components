import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, remote_base
from esphome.const import (
    STATE_CLASS_MEASUREMENT,
)

from . import RFRemoteSensor
from .common import (
    CONF_RECEIVER_ID,
    CONF_REMOTE_ID,
    CONF_RESET_TIME,
    validate_remote_id,
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        RFRemoteSensor,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(CONF_RECEIVER_ID): cv.use_id(remote_base.RemoteReceiverBase),
            cv.Required(CONF_REMOTE_ID): validate_remote_id,
            cv.Optional(
                CONF_RESET_TIME, default="50ms"
            ): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    cg.add(var.set_remote_id(int(config[CONF_REMOTE_ID], 16)))
    cg.add(var.set_reset_time(config[CONF_RESET_TIME]))

    receiver = await cg.get_variable(config[CONF_RECEIVER_ID])
    cg.add(receiver.register_listener(var))
