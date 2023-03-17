import esphome.codegen as cg
from esphome import pins
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_CHANNEL
from esphome import automation

MULTI_CONF = True

cd405x_ns = cg.esphome_ns.namespace("cd405x")

CD405xComponent = cd405x_ns.class_("CD405x", cg.Component, cg.PollingComponent)
ActivateChannelAction = cd405x_ns.class_("ActivateChannelAction", automation.Action)
InhibitAction = cd405x_ns.class_("InhibitAction", automation.Action)

CONF_PIN_A = "pin_a"
CONF_PIN_B = "pin_b"
CONF_PIN_C = "pin_c"
CONF_PIN_INHIBIT = "pin_inhibit"
CONF_INHIBIT = "inhibit"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(CD405xComponent),
        cv.Required(CONF_PIN_A): pins.gpio_output_pin_schema,
        cv.Required(CONF_PIN_B): pins.gpio_output_pin_schema,
        cv.Optional(CONF_PIN_C): pins.gpio_output_pin_schema,
        cv.Optional(CONF_PIN_INHIBIT): pins.gpio_output_pin_schema,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    pin_a = await cg.gpio_pin_expression(config[CONF_PIN_A])
    cg.add(var.set_pin_a(pin_a))
    pin_b = await cg.gpio_pin_expression(config[CONF_PIN_B])
    cg.add(var.set_pin_b(pin_b))
    if CONF_PIN_C in config:
        pin_c = await cg.gpio_pin_expression(config[CONF_PIN_C])
        cg.add(var.set_pin_c(pin_c))
    if CONF_PIN_INHIBIT in config:
        pin_inhibit = await cg.gpio_pin_expression(config[CONF_PIN_INHIBIT])
        cg.add(var.set_pin_inhibit(pin_inhibit))


@automation.register_action(
    "cd405x.activate_channel",
    ActivateChannelAction,
    automation.maybe_simple_id(
        cv.Schema(
            {
                cv.Required(CONF_ID): cv.use_id(CD405xComponent),
                cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=0, max=7)),
            }
        )
    ),
)
async def cd405x_activate_channel_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_CHANNEL], args, cg.size_t)
    cg.add(var.set_channel(template_))
    return var


@automation.register_action(
    "cd405x.inhibit",
    InhibitAction,
    automation.maybe_simple_id(
        cv.Schema(
            {
                cv.Required(CONF_ID): cv.use_id(CD405xComponent),
                cv.Required(CONF_INHIBIT): cv.templatable(cv.boolean),
            }
        )
    ),
)
async def cd405x_inhibit_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_CHANNEL], args, cg.size_t)
    cg.add(var.set_inhibit(template_))
    return var
