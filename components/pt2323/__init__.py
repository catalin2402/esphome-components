from esphome import automation
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_CHANNEL, CONF_ID, CONF_INPUT, CONF_TRIGGER_ID

CODEOWNERS = ["@catalin2402"]
DEPENDENCIES = ["i2c"]
MULTI_CONF = True

CONF_ENHANCE = "enhance"
CONF_BOOST = "boost"
CONF_MUTE = "mute"
CONF_ON_INPUT = "on_input"
CONF_ON_ENHANCE = "on_enhance"
CONF_ON_BOOST = "on_boost"
CONF_ON_MUTE = "on_mute"
CONF_ON_CHANNEL_MUTE = "on_channel_mute"

pt2323_ns = cg.esphome_ns.namespace("pt2323")
PT2323 = pt2323_ns.class_("PT2323", cg.Component, i2c.I2CDevice)
CONF_PT2323_ID = "pt2323_id"

ResendDataAction = pt2323_ns.class_("ResendDataAction", automation.Action)
SelectInputAction = pt2323_ns.class_("SelectInputAction", automation.Action)
SetMuteAction = pt2323_ns.class_("SetMuteAction", automation.Action)
ToggleMuteAction = pt2323_ns.class_("ToggleMuteAction", automation.Action)
SetMuteChannelAction = pt2323_ns.class_("SetMuteChannelAction", automation.Action)
SetEnhanceAction = pt2323_ns.class_("SetEnhanceAction", automation.Action)
SetBoostAction = pt2323_ns.class_("SetBoostAction", automation.Action)
IsMutedCondition = pt2323_ns.class_("IsMutedCondition", automation.Condition)
SelectedInputCondition = pt2323_ns.class_(
    "SelectedInputCondition", automation.Condition
)
IsEnhancedCondition = pt2323_ns.class_("IsEnhancedCondition", automation.Condition)
IsBoostedCondition = pt2323_ns.class_("IsBoostedCondition", automation.Condition)
IsChannelMutedCondition = pt2323_ns.class_(
    "IsChannelMutedCondition", automation.Condition
)
InputTrigger = pt2323_ns.class_("InputTrigger", automation.Trigger.template(cg.int_))
EnhanceTrigger = pt2323_ns.class_(
    "EnhanceTrigger", automation.Trigger.template(cg.bool_)
)
BoostTrigger = pt2323_ns.class_("BoostTrigger", automation.Trigger.template(cg.bool_))
MuteTrigger = pt2323_ns.class_("MuteTrigger", automation.Trigger.template(cg.bool_))
ChannelMuteTrigger = pt2323_ns.class_(
    "ChannelMuteTrigger", automation.Trigger.template(cg.int_, cg.bool_)
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PT2323),
            cv.Optional(CONF_INPUT, default=0): cv.int_range(min=0, max=4),
            cv.Optional(CONF_ENHANCE, default=False): cv.boolean,
            cv.Optional(CONF_BOOST, default=False): cv.boolean,
            cv.Optional(CONF_MUTE, default=False): cv.boolean,
            cv.Optional(CONF_ON_INPUT): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(InputTrigger),
                }
            ),
            cv.Optional(CONF_ON_ENHANCE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EnhanceTrigger),
                }
            ),
            cv.Optional(CONF_ON_BOOST): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BoostTrigger),
                }
            ),
            cv.Optional(CONF_ON_MUTE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(MuteTrigger),
                }
            ),
            cv.Optional(CONF_ON_CHANNEL_MUTE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ChannelMuteTrigger),
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x4A))
)

OPERATION_BASE_SCHEMA = cv.Schema({cv.GenerateID(): cv.use_id(PT2323)})


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_input(config[CONF_INPUT]))
    cg.add(var.set_enhance(config[CONF_ENHANCE]))
    cg.add(var.set_boost(config[CONF_BOOST]))
    cg.add(var.mute_all_channels(config[CONF_MUTE]))
    for conf in config.get(CONF_ON_INPUT, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.int_, CONF_INPUT)], conf)
    for conf in config.get(CONF_ON_ENHANCE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "enabled")], conf)
    for conf in config.get(CONF_ON_BOOST, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "enabled")], conf)
    for conf in config.get(CONF_ON_MUTE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "muted")], conf)
    for conf in config.get(CONF_ON_CHANNEL_MUTE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(
            trigger, [(cg.int_, CONF_CHANNEL), (cg.bool_, "muted")], conf
        )


@automation.register_action(
    "pt2323.resend_data",
    ResendDataAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def pt2323_resend_data_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "pt2323.select_input",
    SelectInputAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_INPUT): cv.templatable(cv.int_range(min=0, max=4)),
        }
    ),
    synchronous=True,
)
async def pt2323_select_input_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_INPUT], args, cg.int_)
    cg.add(var.set_input(value))
    return var


@automation.register_action(
    "pt2323.mute",
    SetMuteAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def pt2323_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(True, args, cg.bool_)
    cg.add(var.set_mute(value))
    return var


@automation.register_action(
    "pt2323.unmute",
    SetMuteAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def pt2323_unmute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(False, args, cg.bool_)
    cg.add(var.set_mute(value))
    return var


@automation.register_action(
    "pt2323.set_mute",
    SetMuteAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_MUTE): cv.templatable(cv.boolean),
        }
    ),
    synchronous=True,
)
async def pt2323_set_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_MUTE], args, cg.bool_)
    cg.add(var.set_mute(value))
    return var


@automation.register_action(
    "pt2323.toggle_mute",
    ToggleMuteAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def pt2323_toggle_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "pt2323.mute_channel",
    SetMuteChannelAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
        }
    ),
    synchronous=True,
)
async def pt2323_mute_channel_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_CHANNEL], args, cg.int_)
    cg.add(var.set_channel(value))
    value = await cg.templatable(True, args, cg.bool_)
    cg.add(var.set_mute(value))
    return var


@automation.register_action(
    "pt2323.unmute_channel",
    SetMuteChannelAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
        }
    ),
    synchronous=True,
)
async def pt2323_unmute_channel_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_CHANNEL], args, cg.int_)
    cg.add(var.set_channel(value))
    value = await cg.templatable(False, args, cg.bool_)
    cg.add(var.set_mute(value))
    return var


@automation.register_action(
    "pt2323.set_channel_mute",
    SetMuteChannelAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
            cv.Required(CONF_MUTE): cv.templatable(cv.boolean),
        }
    ),
    synchronous=True,
)
async def pt2323_set_channel_mute_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_CHANNEL], args, cg.int_)
    cg.add(var.set_channel(value))
    value = await cg.templatable(config[CONF_MUTE], args, cg.bool_)
    cg.add(var.set_mute(value))
    return var


@automation.register_action(
    "pt2323.set_enhance",
    SetEnhanceAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_ENHANCE): cv.templatable(cv.boolean),
        }
    ),
    synchronous=True,
)
async def pt2323_set_enhance_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_ENHANCE], args, cg.bool_)
    cg.add(var.set_enhance(value))
    return var


@automation.register_action(
    "pt2323.set_boost",
    SetBoostAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_BOOST): cv.templatable(cv.boolean),
        }
    ),
    synchronous=True,
)
async def pt2323_set_boost_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_BOOST], args, cg.bool_)
    cg.add(var.set_boost(value))
    return var


@automation.register_condition(
    "pt2323.is_muted",
    IsMutedCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def pt2323_is_muted_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "pt2323.selected_input",
    SelectedInputCondition,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_INPUT): cv.templatable(cv.int_range(min=0, max=4)),
        }
    ),
)
async def pt2323_selected_input_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(condition_id, template_arg, parent)
    value = await cg.templatable(config[CONF_INPUT], args, cg.int_)
    cg.add(var.set_input(value))
    return var


@automation.register_condition(
    "pt2323.is_enhanced",
    IsEnhancedCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def pt2323_is_enhanced_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "pt2323.is_boosted",
    IsBoostedCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def pt2323_is_boosted_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "pt2323.is_channel_muted",
    IsChannelMutedCondition,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=1, max=6)),
        }
    ),
)
async def pt2323_is_channel_muted_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(condition_id, template_arg, parent)
    value = await cg.templatable(config[CONF_CHANNEL], args, cg.int_)
    cg.add(var.set_channel(value))
    return var
