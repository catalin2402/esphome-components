from esphome import automation, pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_CHANNEL, CONF_ID, CONF_TRIGGER_ID

CODEOWNERS = ["@catalin2402"]
MULTI_CONF = True

cd405x_ns = cg.esphome_ns.namespace("cd405x")
CD405xComponent = cd405x_ns.class_("CD405x", cg.Component)
SelectChannelAction = cd405x_ns.class_("SelectChannelAction", automation.Action)
InhibitAction = cd405x_ns.class_("InhibitAction", automation.Action)
ToggleInhibitAction = cd405x_ns.class_("ToggleInhibitAction", automation.Action)
SelectedChannelCondition = cd405x_ns.class_(
    "SelectedChannelCondition", automation.Condition
)
IsInhibitedCondition = cd405x_ns.class_("IsInhibitedCondition", automation.Condition)
IsEnabledCondition = cd405x_ns.class_("IsEnabledCondition", automation.Condition)
ChannelTrigger = cd405x_ns.class_(
    "ChannelTrigger", automation.Trigger.template(cg.int_)
)
InhibitTrigger = cd405x_ns.class_(
    "InhibitTrigger", automation.Trigger.template(cg.bool_)
)

CONF_PIN_A = "pin_a"
CONF_PIN_B = "pin_b"
CONF_PIN_C = "pin_c"
CONF_PIN_INHIBIT = "pin_inhibit"
CONF_INHIBIT = "inhibit"
CONF_ON_CHANNEL = "on_channel"
CONF_ON_INHIBIT = "on_inhibit"


def validate_config(config):
    if CONF_PIN_C not in config and config[CONF_CHANNEL] > 3:
        raise cv.Invalid("Channels 4-7 require pin_c", path=[CONF_CHANNEL])
    if CONF_PIN_INHIBIT not in config:
        if config[CONF_INHIBIT]:
            raise cv.Invalid("inhibit: true requires pin_inhibit", path=[CONF_INHIBIT])
        if CONF_ON_INHIBIT in config:
            raise cv.Invalid("on_inhibit requires pin_inhibit", path=[CONF_ON_INHIBIT])
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CD405xComponent),
            cv.Required(CONF_PIN_A): pins.gpio_output_pin_schema,
            cv.Required(CONF_PIN_B): pins.gpio_output_pin_schema,
            cv.Optional(CONF_PIN_C): pins.gpio_output_pin_schema,
            cv.Optional(CONF_PIN_INHIBIT): pins.gpio_output_pin_schema,
            cv.Optional(CONF_CHANNEL, default=0): cv.int_range(min=0, max=7),
            cv.Optional(CONF_INHIBIT, default=False): cv.boolean,
            cv.Optional(CONF_ON_CHANNEL): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ChannelTrigger),
                }
            ),
            cv.Optional(CONF_ON_INHIBIT): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(InhibitTrigger),
                }
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    validate_config,
)

OPERATION_BASE_SCHEMA = cv.Schema({cv.GenerateID(): cv.use_id(CD405xComponent)})
CHANNEL_SCHEMA = OPERATION_BASE_SCHEMA.extend(
    {
        cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=0, max=7)),
    }
)
INHIBIT_SCHEMA = OPERATION_BASE_SCHEMA.extend(
    {
        cv.Required(CONF_INHIBIT): cv.templatable(cv.boolean),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    for key in (CONF_PIN_A, CONF_PIN_B, CONF_PIN_C, CONF_PIN_INHIBIT):
        if key in config:
            pin = await cg.gpio_pin_expression(config[key])
            cg.add(getattr(var, f"set_{key}")(pin))
    cg.add(var.select_channel(config[CONF_CHANNEL]))
    cg.add(var.inhibit(config[CONF_INHIBIT]))
    for conf in config.get(CONF_ON_CHANNEL, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.int_, "channel")], conf)
    for conf in config.get(CONF_ON_INHIBIT, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.bool_, "inhibited")], conf)


@automation.register_action(
    "cd405x.select_channel", SelectChannelAction, CHANNEL_SCHEMA, synchronous=True
)
async def select_channel_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_CHANNEL], args, cg.int_)
    cg.add(var.set_channel(value))
    return var


@automation.register_action(
    "cd405x.inhibit", InhibitAction, INHIBIT_SCHEMA, synchronous=True
)
async def inhibit_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    value = await cg.templatable(config[CONF_INHIBIT], args, cg.bool_)
    cg.add(var.set_inhibit(value))
    return var


@automation.register_action(
    "cd405x.disable",
    InhibitAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def disable_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    cg.add(var.set_inhibit(True))
    return var


@automation.register_action(
    "cd405x.enable",
    InhibitAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def enable_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    cg.add(var.set_inhibit(False))
    return var


@automation.register_action(
    "cd405x.toggle_inhibit",
    ToggleInhibitAction,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    synchronous=True,
)
async def toggle_inhibit_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_condition(
    "cd405x.selected_channel", SelectedChannelCondition, CHANNEL_SCHEMA
)
async def selected_channel_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(condition_id, template_arg, parent)
    value = await cg.templatable(config[CONF_CHANNEL], args, cg.int_)
    cg.add(var.set_channel(value))
    return var


@automation.register_condition(
    "cd405x.is_inhibited",
    IsInhibitedCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def is_inhibited_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)


@automation.register_condition(
    "cd405x.is_enabled",
    IsEnabledCondition,
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
)
async def is_enabled_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)
