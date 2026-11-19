import esphome.codegen as cg
from esphome.components import remote_base, switch
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS, CONF_CHANNEL
from esphome.types import ConfigType

from .. import unitec_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["unitec", "remote_transmitter"]

UnitecSwitch = unitec_ns.class_(
    "UnitecSwitch", switch.Switch, cg.Component, remote_base.RemoteTransmittable
)

CONFIG_SCHEMA = (
    switch.switch_schema(UnitecSwitch)
    .extend(
        {
            cv.Required(CONF_ADDRESS): cv.hex_uint16_t,
            cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=4),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(remote_base.REMOTE_TRANSMITTABLE_SCHEMA)
)


async def to_code(config: ConfigType) -> None:
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    await remote_base.register_transmittable(var, config)
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_channel(config[CONF_CHANNEL]))
