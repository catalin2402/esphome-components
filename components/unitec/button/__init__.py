import esphome.codegen as cg
from esphome.components import button, remote_base
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS, CONF_CHANNEL, CONF_STATE
from esphome.types import ConfigType

from .. import PROTOCOL_SCHEMA, unitec_ns

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["unitec", "remote_transmitter"]

UnitecButton = unitec_ns.class_(
    "UnitecButton", button.Button, cg.Component, remote_base.RemoteTransmittable
)

CONFIG_SCHEMA = (
    button.button_schema(UnitecButton)
    .extend(PROTOCOL_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
    .extend(remote_base.REMOTE_TRANSMITTABLE_SCHEMA)
)


async def to_code(config: ConfigType) -> None:
    var = await button.new_button(config)
    await cg.register_component(var, config)
    await remote_base.register_transmittable(var, config)
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    cg.add(var.set_state(config[CONF_STATE]))
