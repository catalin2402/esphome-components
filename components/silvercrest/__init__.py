import esphome.codegen as cg
from esphome.components import remote_base
import esphome.config_validation as cv
from esphome.const import CONF_CHANNEL, CONF_ID, CONF_STATE

CODEOWNERS = ["@catalin2402"]
AUTO_LOAD = ["remote_base"]
DEPENDENCIES = ["remote_transmitter"]

CONF_SILVERCREST_ID = "silvercrest"
CONF_REPEAT_TIMES = "repeat_times"
silvercrest_ns = cg.esphome_ns.namespace("silvercrest")
Silvercrest = silvercrest_ns.class_(
    "Silvercrest", cg.Component, remote_base.RemoteTransmittable
)
Channel = silvercrest_ns.enum("Channel")
CHANNELS = {name: getattr(Channel, name) for name in ("A", "B", "C", "D", "MASTER")}
SilvercrestData = silvercrest_ns.struct("SilvercrestData")
SilvercrestBinarySensor = silvercrest_ns.class_(
    "SilvercrestBinarySensor", remote_base.RemoteReceiverBinarySensorBase
)
SilvercrestDumper = silvercrest_ns.class_(
    "SilvercrestDumper", remote_base.RemoteReceiverDumperBase
)
SilvercrestAction = silvercrest_ns.class_(
    "SilvercrestAction", remote_base.RemoteTransmitterActionBase
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
def silvercrest_binary_sensor(var, config):
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
def silvercrest_dumper(var, config):
    pass


@remote_base.register_action(
    "silvercrest",
    SilvercrestAction,
    PROTOCOL_SCHEMA.extend(
        {
            cv.Optional(CONF_REPEAT_TIMES, default=5): cv.int_range(min=1, max=255),
        }
    ),
)
async def silvercrest_action(var, config, args):
    for key, type_ in (
        (CONF_CHANNEL, Channel),
        (CONF_STATE, cg.bool_),
        (CONF_REPEAT_TIMES, cg.uint8),
    ):
        template = await cg.templatable(config[key], args, type_)
        cg.add(getattr(var, f"set_{key}")(template))


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Silvercrest),
            cv.Optional(CONF_REPEAT_TIMES, default=5): cv.int_range(min=1, max=255),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(remote_base.REMOTE_TRANSMITTABLE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await remote_base.register_transmittable(var, config)
    cg.add(var.set_repeat_times(config[CONF_REPEAT_TIMES]))
