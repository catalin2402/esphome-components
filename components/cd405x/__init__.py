from esphome import automation, pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_CHANNEL, CONF_ID
from esphome.types import ConfigType

CODEOWNERS = ["@catalin2402"]
DOMAIN = "cd405x"
MULTI_CONF = True

cd405x_ns = cg.esphome_ns.namespace(DOMAIN)
CD405xComponent = cd405x_ns.class_("CD405x", cg.Component)
CONF_PIN_A = "pin_a"
CONF_PIN_B = "pin_b"
CONF_PIN_C = "pin_c"
CONF_PIN_INHIBIT = "pin_inhibit"
CONF_INHIBIT = "inhibit"
CONF_ON_CHANNEL = "on_channel"
CONF_ON_INHIBIT = "on_inhibit"


def validate_config(config: ConfigType) -> ConfigType:
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
            cv.Optional(CONF_ON_CHANNEL): automation.validate_automation({}),
            cv.Optional(CONF_ON_INHIBIT): automation.validate_automation({}),
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


_CALLBACK_AUTOMATIONS = (
    automation.CallbackAutomation(
        CONF_ON_CHANNEL, "add_on_channel_callback", [(cg.int_, "channel")]
    ),
    automation.CallbackAutomation(
        CONF_ON_INHIBIT, "add_on_inhibit_callback", [(cg.bool_, "inhibited")]
    ),
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cg.add_pin(var.set_pin_a, config[CONF_PIN_A])
    await cg.add_pin(var.set_pin_b, config[CONF_PIN_B])
    await cg.add_pin(var.set_pin_c, config.get(CONF_PIN_C))
    await cg.add_pin(var.set_pin_inhibit, config.get(CONF_PIN_INHIBIT))
    cg.add(var.select_channel(config[CONF_CHANNEL]))
    cg.add(var.inhibit(config[CONF_INHIBIT]))
    await automation.build_callback_automations(var, config, _CALLBACK_AUTOMATIONS)


automation.register_apply_action(
    "cd405x.select_channel",
    CHANNEL_SCHEMA,
    automation.ApplyField(CONF_CHANNEL, "select_channel", cg.int_),
)

automation.register_apply_action(
    "cd405x.inhibit",
    INHIBIT_SCHEMA,
    automation.ApplyField(CONF_INHIBIT, "inhibit", cg.bool_),
)

automation.register_apply_action(
    "cd405x.disable",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("inhibit(true)"),
)

automation.register_apply_action(
    "cd405x.enable",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("inhibit(false)"),
)

automation.register_apply_action(
    "cd405x.toggle_inhibit",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    automation.ApplyCall("toggle_inhibit()"),
)

automation.register_apply_condition(
    "cd405x.selected_channel",
    CHANNEL_SCHEMA,
    automation.ApplyCall("get_channel() == {}", ((CONF_CHANNEL, cg.int_),)),
)

automation.register_apply_condition(
    "cd405x.is_inhibited",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    "is_inhibited()",
)

automation.register_apply_condition(
    "cd405x.is_enabled",
    automation.maybe_simple_id(OPERATION_BASE_SCHEMA),
    "is_inhibited() == false",
)
