import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_INPUT, CONF_CHANNEL
from esphome import automation

DEPENDENCIES = ["i2c"]

pt2323_ns = cg.esphome_ns.namespace("pt2323")
PT2323 = pt2323_ns.class_("PT2323", cg.Component, i2c.I2CDevice)

CONF_PT2323_ID = "pt2323_id"
CONF_ENAHNCE = "enhance"
CONF_BOOST = "boost"
CONF_MUTE = "mute"

SetInputAction = pt2323_ns.class_("SetInputAction", automation.Action)
SetMuteAction = pt2323_ns.class_("SetMuteAction", automation.Action)
SetUnmuteAction = pt2323_ns.class_("SetUnmuteAction", automation.Action)
SetMuteChannelAction = pt2323_ns.class_("SetMuteChannelAction", automation.Action)
SetUnmuteChannelAction = pt2323_ns.class_("SetUnmuteChannelAction", automation.Action)
SetEnhanceAction = pt2323_ns.class_("SetEnhanceAction", automation.Action)
SetBoostAction = pt2323_ns.class_("SetBoostAction", automation.Action)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(PT2323),
            cv.Optional(CONF_INPUT, default=0): cv.int_range(min=0, max=4),
            cv.Optional(CONF_ENAHNCE, default=False): cv.boolean,
            cv.Optional(CONF_BOOST, default=False): cv.boolean,
            cv.Optional(CONF_MUTE, default=False): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x4A))
)

OPERATION_BASE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(PT2323),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_input(config[CONF_INPUT]))
    cg.add(var.set_enhance(config[CONF_ENAHNCE]))
    cg.add(var.set_boost(config[CONF_BOOST]))
    cg.add(var.mute_all_channels(config[CONF_MUTE]))


@automation.register_action(
    "pt2323.set_input",
    SetInputAction,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_INPUT): cv.templatable(cv.int_range(min=0, max=4)),
            }
        )
    ),
)
async def pt2323_set_input_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_INPUT], args, cg.size_t)
    cg.add(var.set_input(template_))
    return var


@automation.register_action(
    "pt2323.mute", SetMuteAction, automation.maybe_simple_id(OPERATION_BASE_SCHEMA)
)
async def pt2323_mute_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var


@automation.register_action(
    "pt2323.unmute", SetUnmuteAction, automation.maybe_simple_id(OPERATION_BASE_SCHEMA)
)
async def pt2323_unmute_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var


@automation.register_action(
    "pt2323.mute_channel",
    SetMuteChannelAction,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
            }
        )
    ),
)
async def pt2323_mute_channel_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_CHANNEL], args, cg.size_t)
    cg.add(var.set_channel(template_))
    return var


@automation.register_action(
    "pt2323.unmute_channel",
    SetUnmuteChannelAction,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
            }
        )
    ),
)
async def pt2323_unmute_channel_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_CHANNEL], args, cg.size_t)
    cg.add(var.set_channel(template_))
    return var


@automation.register_action(
    "pt2323.set_enhance",
    SetEnhanceAction,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_ENAHNCE): cv.boolean,
            }
        )
    ),
)
async def pt2323_set_enhance_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_ENAHNCE], args, cg.bool_)
    cg.add(var.set_enhance(template_))
    return var


@automation.register_action(
    "pt2323.set_boost",
    SetBoostAction,
    automation.maybe_simple_id(
        OPERATION_BASE_SCHEMA.extend(
            {
                cv.Required(CONF_BOOST): cv.boolean,
            }
        )
    ),
)
async def pt2323_set_enhance_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_BOOST], args, cg.bool_)
    cg.add(var.set_boost(template_))
    return var
