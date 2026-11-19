import esphome.codegen as cg
from esphome.components import remote_base
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS, CONF_CHANNEL, CONF_STATE
from esphome.types import ConfigType

DOMAIN = "unitec"
CODEOWNERS = ["@catalin2402"]
AUTO_LOAD = ["remote_base"]
CONFIG_SCHEMA = cv.Schema({})

unitec_ns = cg.esphome_ns.namespace(DOMAIN)
UnitecData = unitec_ns.struct("UnitecData")
UnitecBinarySensor = unitec_ns.class_(
    "UnitecBinarySensor", remote_base.RemoteReceiverBinarySensorBase
)
UnitecDumper = unitec_ns.class_("UnitecDumper", remote_base.RemoteReceiverDumperBase)


@remote_base.register_dumper(DOMAIN, UnitecDumper)
def unitec_dumper(var: cg.MockObj, config: ConfigType) -> None:
    pass


PROTOCOL_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ADDRESS): cv.hex_uint16_t,
        cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=4),
        cv.Required(CONF_STATE): cv.boolean,
    }
)


@remote_base.register_binary_sensor(DOMAIN, UnitecBinarySensor, PROTOCOL_SCHEMA)
def unitec_binary_sensor(var: cg.MockObj, config: ConfigType) -> None:
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


remote_base.register_protocol_action(
    DOMAIN,
    "Unitec",
    PROTOCOL_SCHEMA,
    (CONF_ADDRESS, "address", cg.uint16),
    (CONF_CHANNEL, "channel", cg.uint8),
    (CONF_STATE, "state", cg.bool_),
)
