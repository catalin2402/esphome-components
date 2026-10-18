import esphome.codegen as cg
from esphome.components import remote_base
import esphome.config_validation as cv

DOMAIN = "auriol_4ld5661"
CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["remote_receiver"]

CONF_AURIOL_ID = "auriol_id"
CONF_MM_PER_TIP = "mm_per_tip"
auriol_4ld5661_ns = cg.esphome_ns.namespace(DOMAIN)
Auriol4LD5661Base = auriol_4ld5661_ns.class_(
    "Auriol4LD5661Base", remote_base.RemoteReceiverListener, cg.Component
)
AuriolDumper = auriol_4ld5661_ns.class_(
    "AuriolDumper", remote_base.RemoteReceiverDumperBase
)


async def register_auriol(var, config):
    await cg.register_component(var, config)
    await remote_base.register_listener(var, config)
    if CONF_AURIOL_ID in config:
        cg.add(var.set_auriol_id(config[CONF_AURIOL_ID]))


@remote_base.register_dumper(DOMAIN, AuriolDumper)
def auriol_dumper(var, config):
    pass


CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    pass
