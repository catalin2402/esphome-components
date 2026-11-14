import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch, remote_base
from esphome.const import CONF_ADDRESS, CONF_CHANNEL

from .. import unitec_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["unitec", "remote_transmitter"]

UnitecSwitch = unitec_ns.class_(
    "UnitecSwitch", switch.Switch, cg.Component, remote_base.RemoteTransmittable
)

CONFIG_SCHEMA = (
    switch.switch_schema(UnitecSwitch)
    .extend({
            cv.Required(CONF_ADDRESS): cv.hex_uint16_t,
            cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=4),
        })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(remote_base.REMOTE_TRANSMITTABLE_SCHEMA)
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    await remote_base.register_transmittable(var, config)
    for key in (CONF_ADDRESS, CONF_CHANNEL):
        cg.add(getattr(var, f"set_{key}")(config[key]))
