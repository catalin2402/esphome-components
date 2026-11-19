import esphome.codegen as cg
from esphome.components import remote_base
import esphome.config_validation as cv
from esphome.types import ConfigType

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


async def register_auriol(var: cg.MockObj, config: ConfigType) -> None:
    await cg.register_component(var, config)
    await remote_base.register_listener(var, config)
    cg.add_optional(var.set_auriol_id, config.get(CONF_AURIOL_ID))


@remote_base.register_dumper(DOMAIN, AuriolDumper)
def auriol_dumper(var: cg.MockObj, config: ConfigType) -> None:
    pass


CONFIG_SCHEMA = cv.Schema({})


async def to_code(config: ConfigType) -> None:
    pass
