from esphome.components import binary_sensor, remote_base
import esphome.config_validation as cv
from esphome.const import CONF_TYPE, DEVICE_CLASS_BATTERY

from .. import CONF_AURIOL_ID, Auriol4LD5661Base, auriol_4ld5661_ns, register_auriol

DEPENDENCIES = ["remote_receiver"]
AuriolBatterySensor = auriol_4ld5661_ns.class_(
    "AuriolBatterySensor", Auriol4LD5661Base, binary_sensor.BinarySensor
)

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(
        AuriolBatterySensor, device_class=DEVICE_CLASS_BATTERY
    )
    .extend(
        {
            cv.Required(CONF_TYPE): cv.one_of("battery", lower=True),
            cv.Optional(CONF_AURIOL_ID): cv.uint8_t,
        }
    )
    .extend(remote_base.REMOTE_LISTENER_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await register_auriol(var, config)
