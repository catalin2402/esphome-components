import esphome.codegen as cg
from esphome.components import remote_base
import esphome.config_validation as cv

DOMAIN = "xiron"
CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["remote_receiver"]

CONF_XIRON_ID = "xiron_id"
xiron_ns = cg.esphome_ns.namespace(DOMAIN)
XironBase = xiron_ns.class_(
    "XironBase", remote_base.RemoteReceiverListener, cg.Component
)
XironDumper = xiron_ns.class_("XironDumper", remote_base.RemoteReceiverDumperBase)


async def register_xiron(var, config):
    await cg.register_component(var, config)
    await remote_base.register_listener(var, config)
    if CONF_XIRON_ID in config:
        cg.add(var.set_xiron_id(config[CONF_XIRON_ID]))


@remote_base.register_dumper(DOMAIN, XironDumper)
def xiron_dumper(var, config):
    pass


CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    pass
