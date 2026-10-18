from esphome.components import binary_sensor, remote_base
import esphome.config_validation as cv
from esphome.const import CONF_TYPE, DEVICE_CLASS_BATTERY

from .. import CONF_XIRON_ID, XironBase, register_xiron, xiron_ns

DEPENDENCIES = ["remote_receiver"]
XironBatterySensor = xiron_ns.class_(
    "XironBatterySensor", XironBase, binary_sensor.BinarySensor
)

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(
        XironBatterySensor, device_class=DEVICE_CLASS_BATTERY
    )
    .extend(
        {
            cv.Required(CONF_TYPE): cv.one_of("battery", lower=True),
            cv.Optional(CONF_XIRON_ID): cv.uint16_t,
        }
    )
    .extend(remote_base.REMOTE_LISTENER_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await register_xiron(var, config)
