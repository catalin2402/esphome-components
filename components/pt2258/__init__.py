from esphome import automation
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import (
    CONF_CHANNEL,
    CONF_ID,
    CONF_OFFSET,
    CONF_TRIGGER_ID,
    CONF_VOLUME,
)

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["i2c"]
MULTI_CONF = True

CONF_DEFAULT_VOLUME = "default_volume"
CONF_ON_VOLUME = "on_volume"
CONF_ON_MUTE = "on_mute"
CONF_MUTE = "mute"
CONF_MIN = "min"
CONF_MAX = "max"

pt2258_ns = cg.esphome_ns.namespace("pt2258")
PT2258 = pt2258_ns.class_("PT2258", cg.Component, i2c.I2CDevice)
CONF_PT2258_ID = "pt2258_id"

ResendDataAction = pt2258_ns.class_("ResendDataAction", automation.Action)
SetMuteAction = pt2258_ns.class_("SetMuteAction", automation.Action)
ToggleMuteAction = pt2258_ns.class_("ToggleMuteAction", automation.Action)
SetMasterVolumeAction = pt2258_ns.class_("SetMasterVolumeAction", automation.Action)
SetChannelVolumeAction = pt2258_ns.class_("SetChannelVolumeAction", automation.Action)
SetChannelOffsetAction = pt2258_ns.class_("SetChannelOffsetAction", automation.Action)
IsMutedCondition = pt2258_ns.class_("IsMutedCondition", automation.Condition)
VolumeInRangeCondition = pt2258_ns.class_(
    "VolumeInRangeCondition", automation.Condition
)
VolumeTrigger = pt2258_ns.class_(
    "VolumeTrigger", automation.Trigger.template(cg.int_, cg.int_)
)
MuteTrigger = pt2258_ns.class_("MuteTrigger", automation.Trigger.template(cg.bool_))

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PT2258),
            cv.Optional(CONF_DEFAULT_VOLUME, default=0): cv.int_range(min=0, max=79),
            cv.Optional(CONF_ON_VOLUME): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(VolumeTrigger),
                }
            ),
            cv.Optional(CONF_ON_MUTE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(MuteTrigger),
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x44))
)

OPERATION_BASE_SCHEMA = cv.Schema({cv.GenerateID(): cv.use_id(PT2258)})


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_default_volume(config[CONF_DEFAULT_VOLUME]))
    for conf in config.get(CONF_ON_VOLUME, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(
            trigger, [(cg.int_, CONF_CHANNEL), (cg.int_, CONF_VOLUME)], conf
        )
    for conf in config.get(CONF_ON_MUTE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "muted")], conf)


@automation.register_action(
    "pt2258.resend_data",
    ResendDataAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def pt2258_resend_data_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "pt2258.mute",
    SetMuteAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def pt2258_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    cg.add(var.set_mute(True))
    return var


@automation.register_action(
    "pt2258.unmute",
    SetMuteAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def pt2258_unmute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    cg.add(var.set_mute(False))
    return var


@automation.register_action(
    "pt2258.set_mute",
    SetMuteAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_MUTE): cv.templatable(cv.boolean),
        }
    ),
    synchronous=True,
)
async def pt2258_set_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_MUTE], args, cg.bool_)
    cg.add(var.set_mute(value))
    return var


@automation.register_action(
    "pt2258.toggle_mute",
    ToggleMuteAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def pt2258_toggle_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "pt2258.set_master_volume",
    SetMasterVolumeAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_VOLUME): cv.templatable(cv.int_range(min=0, max=79)),
        }
    ),
    synchronous=True,
)
async def pt2258_set_master_volume_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_VOLUME], args, cg.int_)
    cg.add(var.set_volume(value))
    return var


@automation.register_action(
    "pt2258.set_channel_volume",
    SetChannelVolumeAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
            cv.Required(CONF_VOLUME): cv.templatable(cv.int_range(min=0, max=79)),
        }
    ),
    synchronous=True,
)
async def pt2258_set_channel_volume_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_CHANNEL], args, cg.int_)
    cg.add(var.set_channel(value))
    value = await cg.templatable(config[CONF_VOLUME], args, cg.int_)
    cg.add(var.set_volume(value))
    return var


@automation.register_action(
    "pt2258.set_channel_offset",
    SetChannelOffsetAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
            cv.Required(CONF_OFFSET): cv.templatable(cv.int_range(min=-79, max=79)),
        }
    ),
    synchronous=True,
)
async def pt2258_set_channel_offset_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_CHANNEL], args, cg.int_)
    cg.add(var.set_channel(value))
    value = await cg.templatable(config[CONF_OFFSET], args, cg.int_)
    cg.add(var.set_offset(value))
    return var


@automation.register_condition(
    "pt2258.is_muted",
    IsMutedCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def pt2258_is_muted_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "pt2258.volume_in_range",
    VolumeInRangeCondition,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Optional(CONF_CHANNEL, default=0): cv.templatable(
                cv.int_range(min=0, max=6)
            ),
            cv.Optional(CONF_MIN, default=0): cv.templatable(
                cv.int_range(min=0, max=79)
            ),
            cv.Optional(CONF_MAX, default=79): cv.templatable(
                cv.int_range(min=0, max=79)
            ),
        }
    ),
)
async def pt2258_volume_in_range_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(condition_id, template_arg, parent)
    value = await cg.templatable(config[CONF_CHANNEL], args, cg.int_)
    cg.add(var.set_channel(value))
    value = await cg.templatable(config[CONF_MIN], args, cg.int_)
    cg.add(var.set_min(value))
    value = await cg.templatable(config[CONF_MAX], args, cg.int_)
    cg.add(var.set_max(value))
    return var
