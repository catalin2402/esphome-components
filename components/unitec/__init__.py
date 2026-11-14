import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import remote_base
from esphome.const import CONF_ADDRESS, CONF_CHANNEL, CONF_STATE

DOMAIN = "unitec"
CODEOWNERS = ["@catalin2402"]
AUTO_LOAD = ["remote_base"]
CONFIG_SCHEMA = cv.Schema({})

unitec_ns = cg.esphome_ns.namespace(DOMAIN)
UnitecData = unitec_ns.struct("UnitecData")
UnitecBinarySensor = unitec_ns.class_(
    "UnitecBinarySensor", remote_base.RemoteReceiverBinarySensorBase
)
UnitecDumper = unitec_ns.class_(
    "UnitecDumper", remote_base.RemoteReceiverDumperBase
)
UnitecAction = unitec_ns.class_(
    "UnitecAction", remote_base.RemoteTransmitterActionBase
)


@remote_base.register_dumper(DOMAIN, UnitecDumper)
def unitec_dumper(var, config):
    pass


PROTOCOL_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ADDRESS): cv.hex_uint16_t,
        cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=4),
        cv.Required(CONF_STATE): cv.boolean,
    }
)


@remote_base.register_binary_sensor(DOMAIN, UnitecBinarySensor, PROTOCOL_SCHEMA)
def unitec_binary_sensor(var, config):
    cg.add(
        var.set_data(
            cg.StructInitializer(
                UnitecData,
                ("address", config[CONF_ADDRESS]),
                ("channel", config[CONF_CHANNEL]),
                ("state", config[CONF_STATE]),
            )
        )
    )


@remote_base.register_action(DOMAIN, UnitecAction, PROTOCOL_SCHEMA)
async def unitec_action(var, config, args):
    for key, type_ in (
        (CONF_ADDRESS, cg.uint16),
        (CONF_CHANNEL, cg.uint8),
        (CONF_STATE, cg.bool_),
    ):
        value = await cg.templatable(config[key], args, type_)
        cg.add(getattr(var, f"set_{key}")(value))
