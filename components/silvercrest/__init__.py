import esphome.codegen as cg
from esphome.components import remote_base
import esphome.config_validation as cv
from esphome.const import CONF_CHANNEL, CONF_ID, CONF_STATE
from esphome.types import ConfigType

CODEOWNERS = ["@catalin2402"]
AUTO_LOAD = ["remote_base"]
DOMAIN = "silvercrest"

CONF_TRANSMITTER_ID = "transmitter_id"
CONF_SILVERCREST_ID = "silvercrest"
CONF_REPEAT_TIMES = "repeat_times"
silvercrest_ns = cg.esphome_ns.namespace(DOMAIN)
Silvercrest = silvercrest_ns.class_(
    "Silvercrest", cg.Component, remote_base.RemoteTransmittable
)
Channel = silvercrest_ns.enum("Channel")
CHANNELS = Channel.options("A", "B", "C", "D", "MASTER")
SilvercrestData = silvercrest_ns.struct("SilvercrestData")
SilvercrestBinarySensor = silvercrest_ns.class_(
    "SilvercrestBinarySensor", remote_base.RemoteReceiverBinarySensorBase
)
SilvercrestDumper = silvercrest_ns.class_(
    "SilvercrestDumper", remote_base.RemoteReceiverDumperBase
)

DUMP_PUBLISHER = cg.HubFunction(
    "dump",
    cg.void,
    [(cg.const_char_ptr, "TAG")],
    define="USE_SILVERCREST_DUMP_PUBLISHER",
)


PROTOCOL_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_CHANNEL): cv.enum(CHANNELS, upper=True),
        cv.Required(CONF_STATE): cv.boolean,
    }
)


@remote_base.register_binary_sensor(
    "silvercrest", SilvercrestBinarySensor, PROTOCOL_SCHEMA
)
def silvercrest_binary_sensor(var: cg.MockObj, config: ConfigType) -> None:
    cg.add(
        var.set_data(
            cg.StructInitializer(
                SilvercrestData,
                ("channel", config[CONF_CHANNEL]),
                ("state", config[CONF_STATE]),
            )
        )
    )


@remote_base.register_dumper("silvercrest", SilvercrestDumper)
def silvercrest_dumper(var: cg.MockObj, config: ConfigType) -> None:
    pass


remote_base.register_protocol_action(
    DOMAIN,
    "Silvercrest",
    PROTOCOL_SCHEMA.extend(
        {cv.Optional(CONF_REPEAT_TIMES, default=5): cv.int_range(min=1, max=255)}
    ),
    (CONF_CHANNEL, "channel", Channel),
    (CONF_STATE, "state", cg.bool_),
    (CONF_REPEAT_TIMES, "repeat_times", cg.uint8),
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Silvercrest),
        cv.OnlyWith(CONF_TRANSMITTER_ID, "remote_transmitter"): cv.use_id(
            remote_base.RemoteTransmitterBase
        ),
        cv.Optional(CONF_REPEAT_TIMES, default=5): cv.int_range(min=1, max=255),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if CONF_TRANSMITTER_ID in config:
        await remote_base.register_transmittable(var, config)
    cg.add(var.set_repeat_times(config[CONF_REPEAT_TIMES]))
