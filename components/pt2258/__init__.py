import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID
from esphome import automation

DEPENDENCIES = ["i2c"]

pt2258_ns = cg.esphome_ns.namespace("pt2258")
PT2258 = pt2258_ns.class_("PT2258", cg.Component, i2c.I2CDevice)

ResendDataAction = pt2258_ns.class_("ResendDataAction", automation.Action)
SetMuteAction = pt2258_ns.class_("SetMuteAction", automation.Action)
ToggleMuteAction = pt2258_ns.class_("ToggleMuteAction", automation.Action)

CONF_DEFAULT_VOLUME = "default_volume"
CONF_MUTE = "mute"
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PT2258),
            cv.Optional(CONF_DEFAULT_VOLUME, default=0): cv.int_range(min=0, max=79),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x44))
)

OPERATION_BASE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(PT2258),
    }
)

SET_MUTE_SCHEMA = OPERATION_BASE_SCHEMA.extend(
    {
        cv.Required(CONF_MUTE): cv.templatable(cv.boolean),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    if CONF_DEFAULT_VOLUME in config:
        cg.add(var.set_default_volume(config[CONF_DEFAULT_VOLUME]))


@automation.register_action(
    "pt2258.resend_data",
    ResendDataAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA.extend),
    synchronous=True,
)
async def pt2258_resend_data_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "pt2258.set_mute",
    SetMuteAction,
    SET_MUTE_SCHEMA,
    synchronous=True,
)
async def pt2258_set_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    mute = await cg.templatable(config[CONF_MUTE], args, bool)
    cg.add(var.set_mute(mute))
    return var


@automation.register_action(
    "pt2258.toggle_mute",
    ToggleMuteAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA.extend),
    synchronous=True,
)
async def pt2258_toggle_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)
