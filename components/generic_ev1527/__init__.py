import esphome.codegen as cg
from esphome.components import remote_base
import esphome.config_validation as cv
from esphome.const import CONF_DATA, CONF_ID

DOMAIN = "generic_ev1527"
CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["remote_receiver"]

generic_ev1527_ns = cg.esphome_ns.namespace(DOMAIN)
GenericEV1527Data = generic_ev1527_ns.struct("GenericEV1527Data")
GenericEV1527BinarySensor = generic_ev1527_ns.class_(
    "GenericEV1527BinarySensor", remote_base.RemoteReceiverBinarySensorBase
)
GenericEV1527Dumper = generic_ev1527_ns.class_(
    "GenericEV1527Dumper", remote_base.RemoteReceiverDumperBase
)

PROTOCOL_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.All(cv.hex_uint32_t, cv.Range(max=0xFFFFF)),
        cv.Required(CONF_DATA): cv.int_range(min=0, max=0x0F),
    }
)


@remote_base.register_binary_sensor(DOMAIN, GenericEV1527BinarySensor, PROTOCOL_SCHEMA)
def generic_ev1527_binary_sensor(var, config):
    cg.add(
        var.set_data(
            cg.StructInitializer(
                GenericEV1527Data,
                ("id", config[CONF_ID]),
                ("data", config[CONF_DATA]),
            )
        )
    )


@remote_base.register_dumper(DOMAIN, GenericEV1527Dumper)
def generic_ev1527_dumper(var, config):
    pass


# An empty top-level block loads the protocol registrations before validation.
CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    pass
